/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
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
#include <Corrade/Containers/Optional.h>
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
#include <Magnum/Vk/Mesh.h>
#include <Magnum/Vk/Pipeline.h>
#include <Magnum/Vk/PipelineLayoutCreateInfo.h>
#include <Magnum/Vk/Queue.h>
#include <Magnum/Vk/RasterizationPipelineCreateInfo.h>
#include <Magnum/Vk/RenderPassCreateInfo.h>
#include <Magnum/Vk/ShaderCreateInfo.h>
#include <Magnum/Vk/ShaderSet.h>

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

    /* Create the triangle mesh */
    Vk::Mesh mesh{Vk::MeshLayout{MeshPrimitive::Triangles}
        .addBinding(0, 2*4*4)
        .addAttribute(0, 0, VertexFormat::Vector4, 0)
        .addAttribute(1, 0, VertexFormat::Vector4, 4*4)
    };
    {
        Vk::Buffer buffer{device, Vk::BufferCreateInfo{
            Vk::BufferUsage::VertexBuffer,
            3*2*4*4 /* Three vertices, each is four-element pos & color */
        }, Vk::MemoryFlag::HostVisible};

        /** @todo arrayCast for an array rvalue */
        Containers::Array<char, Vk::MemoryMapDeleter> data = buffer.dedicatedMemory().map();
        auto view = Containers::arrayCast<Vector4>(data);
        view[0] = {-0.5f, -0.5f, 0.0f, 1.0f}; /* Left vertex, red color */
        view[1] = 0xff0000ff_srgbaf;
        view[2] = { 0.5f, -0.5f, 0.0f, 1.0f}; /* Right vertex, green color */
        view[3] = 0x00ff00ff_srgbaf;
        view[4] = { 0.0f,  0.5f, 0.0f, 1.0f}; /* Top vertex, blue color */
        view[5] = 0x0000ffff_srgbaf;

        mesh.addVertexBuffer(0, std::move(buffer), 0)
            .setCount(3);
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

               OpEntryPoint Vertex %ver "ver" %position %color %gl_Position %interpolatedColorOut
               OpEntryPoint Fragment %fra "fra" %interpolatedColorIn %fragmentColor
               OpExecutionMode %fra OriginUpperLeft

               OpDecorate %position Location 0
               OpDecorate %color Location 1
               OpDecorate %gl_Position BuiltIn Position
               OpDecorate %interpolatedColorOut Location 0
               OpDecorate %interpolatedColorIn Location 0
               OpDecorate %fragmentColor Location 0

       %void = OpTypeVoid
    %fn_void = OpTypeFunction %void
      %float = OpTypeFloat 32
       %vec4 = OpTypeVector %float 4
%ptr_in_vec4 = OpTypePointer Input %vec4
   %position = OpVariable %ptr_in_vec4 Input
      %color = OpVariable %ptr_in_vec4 Input
%ptr_out_vec4 = OpTypePointer Output %vec4
%gl_Position = OpVariable %ptr_out_vec4 Output
%interpolatedColorOut = OpVariable %ptr_out_vec4 Output
%interpolatedColorIn = OpVariable %ptr_in_vec4 Input
%fragmentColor = OpVariable %ptr_out_vec4 Output

        %ver = OpFunction %void None %fn_void
       %ver_ = OpLabel
          %1 = OpLoad %vec4 %position
          %2 = OpLoad %vec4 %color
               OpStore %gl_Position %1
               OpStore %interpolatedColorOut %2
               OpReturn
               OpFunctionEnd

        %fra = OpFunction %void None %fn_void
       %fra_ = OpLabel
          %3 = OpLoad %vec4 %interpolatedColorIn
               OpStore %fragmentColor %3
               OpReturn
               OpFunctionEnd
)"_s;
    Vk::Shader shader{device, Vk::ShaderCreateInfo{
        *CORRADE_INTERNAL_ASSERT_EXPRESSION(CORRADE_INTERNAL_ASSERT_EXPRESSION(
            PluginManager::Manager<ShaderTools::AbstractConverter>{}
                .loadAndInstantiate("SpirvAssemblyToSpirvShaderConverter")
        )->convertDataToData({}, assembly))}};

    /* Create a graphics pipeline */
    Vk::ShaderSet shaderSet;
    shaderSet
        .addShader(Vk::ShaderStage::Vertex, shader, "ver"_s)
        .addShader(Vk::ShaderStage::Fragment, shader, "fra"_s);
    Vk::PipelineLayout pipelineLayout{device, Vk::PipelineLayoutCreateInfo{}};
    Vk::Pipeline pipeline{device, Vk::RasterizationPipelineCreateInfo{shaderSet, mesh.layout(), pipelineLayout, renderPass, 0, 1}
        .setViewport({{}, {800.0f, 600.0f}})
    };

    /* Record the command buffer:
        - render pass begin converts the framebuffer attachment from Undefined
          to ColorAttachment and clears it
        - the pipeline barrier is needed in order to make the image data copied
          to the buffer visible in time for the host read happening below */
    cmd.begin()
       .beginRenderPass(Vk::RenderPassBeginInfo{renderPass, framebuffer}
           .clearColor(0, 0x1f1f1f_srgbf)
        )
       .bindPipeline(pipeline)
       .draw(mesh)
       .endRenderPass()
       .copyImageToBuffer({image, Vk::ImageLayout::TransferSource, pixels, {
            Vk::BufferImageCopy2D{0, Vk::ImageAspect::Color, 0, {{}, {800, 600}}}
        }})
       .pipelineBarrier(Vk::PipelineStage::Transfer, Vk::PipelineStage::Host, {
            {Vk::Access::TransferWrite, Vk::Access::HostRead, pixels}
        })
       .end();

    /* Submit the command buffer and wait until done */
    queue.submit({Vk::SubmitInfo{}.setCommandBuffers({cmd})}).wait();

    /* Read the image back from the buffer */
    CORRADE_INTERNAL_ASSERT_EXPRESSION(
        PluginManager::Manager<Trade::AbstractImageConverter>{}
            .loadAndInstantiate("AnyImageConverter")
    )->convertToFile(ImageView2D{
        PixelFormat::RGBA8Unorm, {800, 600},
        pixels.dedicatedMemory().mapRead()
    }, "image.png");
    Debug{} << "Saved an image to image.png";
}
