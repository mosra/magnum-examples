/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
             — Vladimír Vondruš <mosra@centrum.cz>
        2021 — Pablo Escobar <mail@rvrs.in>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/Assert.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Magnum.h>
#include <Magnum/Mesh.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/VertexFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Range.h>
#include <Magnum/ShaderTools/AbstractConverter.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Vk/Assert.h>
#include <Magnum/Vk/BufferCreateInfo.h>
#include <Magnum/Vk/CommandBuffer.h>
#include <Magnum/Vk/CommandPoolCreateInfo.h>
#include <Magnum/Vk/DeviceCreateInfo.h>
#include <Magnum/Vk/DeviceProperties.h>
#include <Magnum/Vk/Fence.h>
#include <Magnum/Vk/FramebufferCreateInfo.h>
#include <Magnum/Vk/ImageCreateInfo.h>
#include <Magnum/Vk/ImageViewCreateInfo.h>
#include <Magnum/Vk/InstanceCreateInfo.h>
#include <Magnum/Vk/Memory.h>
#include <Magnum/Vk/MeshLayout.h>
#include <Magnum/Vk/Pipeline.h>
#include <Magnum/Vk/PipelineLayoutCreateInfo.h>
#include <Magnum/Vk/Queue.h>
#include <Magnum/Vk/RasterizationPipelineCreateInfo.h>
#include <Magnum/Vk/RenderPassCreateInfo.h>
#include <Magnum/Vk/ShaderCreateInfo.h>
#include <Magnum/Vk/ShaderSet.h>
#include <MagnumExternal/Vulkan/flextVkGlobal.h>

using namespace Corrade::Containers::Literals;
using namespace Magnum;
using namespace Magnum::Math::Literals;

int main(int argc, char** argv) {
    /* Create an instance */
    Vk::Instance instance{Vk::InstanceCreateInfo{argc, argv}
        .setApplicationInfo("Magnum Vulkan Triangle Example"_s, {})
    };

    /* Create a device with a graphics queue */
    Vk::Queue queue{NoCreate};
    Vk::Device device{instance, Vk::DeviceCreateInfo{Vk::pickDevice(instance)}
        .addQueues(Vk::QueueFlag::Graphics, {0.0f}, {queue})
    };

    /* Allocate a command buffer */
    Vk::CommandPool commandPool{device, Vk::CommandPoolCreateInfo{
        device.properties().pickQueueFamily(Vk::QueueFlag::Graphics)
    }};
    Vk::CommandBuffer cmd = commandPool.allocate();

    device.populateGlobalFunctionPointers();

    /* Render pass. We'll want to transfer the image back to the host though a
       buffer once done, so additionally set up a corresponding final image
       layout and a dependency that performs the layout transition before we
       execute the copy command. */
    Vk::RenderPass renderPass{device, Vk::RenderPassCreateInfo{}
        .setAttachments({
            Vk::AttachmentDescription{PixelFormat::RGBA8Srgb,
                Vk::AttachmentLoadOperation::Clear,
                Vk::AttachmentStoreOperation::Store,
                Vk::ImageLayout::Undefined,
                Vk::ImageLayout::TransferSource
            }
        })
        .addSubpass(Vk::SubpassDescription{}
            .setColorAttachments({
                Vk::AttachmentReference{0, Vk::ImageLayout::ColorAttachment}
            })
        )
        .setDependencies({
            Vk::SubpassDependency{
                /* An operation external to the render pass depends on the
                   first subpass */
                0, Vk::SubpassDependency::External,
                /* where transfer gets executed only after color output is
                   done */
                Vk::PipelineStage::ColorAttachmentOutput,
                Vk::PipelineStage::Transfer,
                /* and color data written are available for the transfer to
                   read */
                Vk::Access::ColorAttachmentWrite,
                Vk::Access::TransferRead
            }
        })
    };

    /* Framebuffer image. It doesn't need to be host-visible, however we'll
       copy it to the host through a buffer so enable corresponding usage. */
    Vk::Image image{device, Vk::ImageCreateInfo2D{
        Vk::ImageUsage::ColorAttachment|Vk::ImageUsage::TransferSource,
        PixelFormat::RGBA8Srgb, {800, 600}, 1
    }, Vk::MemoryFlag::DeviceLocal};

    /* Vertex buffer */
    Vk::Buffer buffer{device, Vk::BufferCreateInfo{
        Vk::BufferUsage::VertexBuffer,
        3*2*4*4 /* Three vertices, each is four-element pos & color */
    }, Vk::MemoryFlag::HostVisible};
    {
        /* Fill the data */
        /** @todo arrayCast for an array rvalue */
        Containers::Array<char, Vk::MemoryMapDeleter> data = buffer.dedicatedMemory().map();
        auto view = Containers::arrayCast<Vector4>(data);
        view[0] = {-0.5f, -0.5f, 0.0f, 1.0f}; /* Left vertex, red color */
        view[1] = 0xff0000ff_srgbaf;
        view[2] = { 0.5f, -0.5f, 0.0f, 1.0f}; /* Right vertex, green color */
        view[3] = 0x00ff00ff_srgbaf;
        view[4] = { 0.0f,  0.5f, 0.0f, 1.0f}; /* Top vertex, blue color */
        view[5] = 0x0000ffff_srgbaf;
    }

    /* Buffer to which the rendered image gets linearized */
    Vk::Buffer pixels{device, Vk::BufferCreateInfo{
        Vk::BufferUsage::TransferDestination, 800*600*4
    }, Vk::MemoryFlag::HostVisible};

    /* Framebuffer */
    Vk::ImageView color{device, Vk::ImageViewCreateInfo2D{image}};
    Vk::Framebuffer framebuffer{device, Vk::FramebufferCreateInfo{renderPass, {
        color
    }, {800, 600}}};

    /* Create the shader */
    constexpr Containers::StringView assembly = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450

; Function %1 is vertex shader and has %12, %13 as input and %15, %16 as output
               OpEntryPoint Vertex %1 "ver" %12 %13 %gl_Position %16
; Function %2 is fragment shader and has %5 as input and %6 as output
               OpEntryPoint Fragment %2 "fra" %5 %6
               OpExecutionMode %2 OriginUpperLeft

; Input/output layouts
               OpDecorate %12 Location 0
               OpDecorate %13 Location 1
               OpDecorate %gl_Position BuiltIn Position
               OpDecorate %16 Location 0
               OpDecorate %5 Location 0
               OpDecorate %6 Location 0

; Types
       %void = OpTypeVoid
          %8 = OpTypeFunction %void
      %float = OpTypeFloat 32
    %v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
         %12 = OpVariable %_ptr_Input_v4float Input
         %13 = OpVariable %_ptr_Input_v4float Input
%_ptr_Output_v4float = OpTypePointer Output %v4float
%gl_Position = OpVariable %_ptr_Output_v4float Output
         %16 = OpVariable %_ptr_Output_v4float Output
          %5 = OpVariable %_ptr_Input_v4float Input
          %6 = OpVariable %_ptr_Output_v4float Output

; %1 = void ver()
          %1 = OpFunction %void None %8
         %33 = OpLabel
         %30 = OpLoad %v4float %12
         %31 = OpLoad %v4float %13
               OpStore %gl_Position %30
               OpStore %16 %31
               OpReturn
               OpFunctionEnd

; %2 = void fra()
          %2 = OpFunction %void None %8
         %34 = OpLabel
         %32 = OpLoad %v4float %5
               OpStore %6 %32
               OpReturn
               OpFunctionEnd
)"_s;
    Vk::Shader shader{device, Vk::ShaderCreateInfo{
        CORRADE_INTERNAL_ASSERT_EXPRESSION(CORRADE_INTERNAL_ASSERT_EXPRESSION(
            PluginManager::Manager<ShaderTools::AbstractConverter>{}
                .loadAndInstantiate("SpirvAssemblyToSpirvShaderConverter")
        )->convertDataToData({}, assembly))}};

    constexpr UnsignedInt VertexBufferBinding = 0;

    /* Create a graphics pipeline */
    Vk::MeshLayout meshLayout{MeshPrimitive::Triangles};
    meshLayout
        .addBinding(VertexBufferBinding, 2*4*4)
        .addAttribute(0, VertexBufferBinding, VertexFormat::Vector4, 0)
        .addAttribute(1, VertexBufferBinding, VertexFormat::Vector4, 4*4);
    Vk::ShaderSet shaderSet;
    shaderSet
        .addShader(Vk::ShaderStage::Vertex, shader, "ver"_s)
        .addShader(Vk::ShaderStage::Fragment, shader, "fra"_s);
    Vk::PipelineLayout pipelineLayout{device, Vk::PipelineLayoutCreateInfo{}};
    Vk::Pipeline pipeline{device, Vk::RasterizationPipelineCreateInfo{
            shaderSet, meshLayout, pipelineLayout, renderPass, 0, 1}
        .setViewport({{}, {800.0f, 600.0f}})
    };

    /* Begin recording */
    cmd.begin();

    /* Begin a render pass. Converts the framebuffer attachment from Undefined
       to ColorAttachment layout and clears it. */
    cmd.beginRenderPass(Vk::RenderPassBeginInfo{renderPass, framebuffer}
           .clearColor(0, 0x1f1f1f_srgbf)
        );

    /* Bind the pipeline */
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    /* Bind the vertex buffer */
    {
        const VkDeviceSize offset = 0;
        const VkBuffer handle = buffer;
        vkCmdBindVertexBuffers(cmd, VertexBufferBinding, 1, &handle, &offset);
    }

    /* Draw the triangle */
    vkCmdDraw(cmd, 3, 1, 0, 0);

    /* End render pass */
    cmd.endRenderPass();

    /* Copy the image to a host-visible buffer. After that ensure the memory
       is available in time for the host read. */
    cmd.copyImageToBuffer({image, Vk::ImageLayout::TransferSource, pixels, {
            Vk::BufferImageCopy2D{0, Vk::ImageAspect::Color, 0, {{}, {800, 600}}}
        }})
       .pipelineBarrier(Vk::PipelineStage::Transfer, Vk::PipelineStage::Host, {
           {Vk::Access::TransferWrite, Vk::Access::HostRead, pixels}
        });

    /* End recording */
    cmd.end();

    /* Submit the command buffer and wait until done */
    queue.submit({Vk::SubmitInfo{}.setCommandBuffers({cmd})}).wait();

    /* Read the image back from the buffer */
    CORRADE_INTERNAL_ASSERT_EXPRESSION(
        PluginManager::Manager<Trade::AbstractImageConverter>{}
            .loadAndInstantiate("AnyImageConverter")
    )->exportToFile(ImageView2D{
        PixelFormat::RGBA8Unorm, {800, 600},
        pixels.dedicatedMemory().mapRead()
    }, "image.png");
    Debug{} << "Saved an image to image.png";
}
