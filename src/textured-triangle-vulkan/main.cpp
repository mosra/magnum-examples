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
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Assert.h>
#include <Corrade/Utility/Configuration.h>
#include <Corrade/Utility/Path.h>
#include <Corrade/Utility/Resource.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Magnum.h>
#include <Magnum/Mesh.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Sampler.h>
#include <Magnum/VertexFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Range.h>
#include <Magnum/ShaderTools/AbstractConverter.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Vk/Assert.h>
#include <Magnum/Vk/BufferCreateInfo.h>
#include <Magnum/Vk/CommandBuffer.h>
#include <Magnum/Vk/CommandPoolCreateInfo.h>
#include <Magnum/Vk/DescriptorPoolCreateInfo.h>
#include <Magnum/Vk/DescriptorSet.h>
#include <Magnum/Vk/DescriptorSetLayoutCreateInfo.h>
#include <Magnum/Vk/DescriptorType.h>
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
#include <Magnum/Vk/SamplerCreateInfo.h>
#include <Magnum/Vk/ShaderCreateInfo.h>
#include <Magnum/Vk/ShaderSet.h>

using namespace Corrade::Containers::Literals;
using namespace Magnum;
using namespace Magnum::Math::Literals;

int main(int argc, char** argv) {
    /* Create an instance */
    Vk::Instance instance{Vk::InstanceCreateInfo{argc, argv}
        .setApplicationInfo("Magnum Vulkan Textured Triangle Example"_s, {})
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
        .addBinding(0, (2 + 2)*4)
        .addAttribute(0, 0, VertexFormat::Vector2, 0)
        .addAttribute(1, 0, VertexFormat::Vector2, 2*4)
    };
    {
        Vk::Buffer buffer{device, Vk::BufferCreateInfo{
            Vk::BufferUsage::VertexBuffer,
            3*(2 + 2)*4 /* Three vertices, each is pos & texture coordinate */
        }, Vk::MemoryFlag::HostVisible};

        /** @todo arrayCast for an array rvalue */
        Containers::Array<char, Vk::MemoryMapDeleter> data = buffer.dedicatedMemory().map();
        auto view = Containers::arrayCast<std::pair<Vector2, Vector2>>(data);
        view[0] = {{-0.5f, -0.5f}, {0.0f, 0.0f}}; /* Left */
        view[1] = {{ 0.5f, -0.5f}, {1.0f, 0.0f}}; /* Right */
        view[2] = {{ 0.0f,  0.5f}, {0.5f, 1.0f}}; /* Top */

        mesh.addVertexBuffer(0, std::move(buffer), 0)
            .setCount(3);
    }

    /* Load TGA importer plugin. Explicitly use StbImageImporter because that
       one is the only that can do RGB -> RGBA expansion right now. */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer =
        manager.loadAndInstantiate("StbImageImporter");
    if(!importer) return 1;

    /* Load the texture. Force expansion to RGBA because that's what Vulkan
       wants. */
    const Utility::Resource rs{"textured-triangle-data"};
    if(!importer->openData(rs.getRaw("stone.tga"))) return 2;
    importer->configuration().setValue("forceChannelCount", 4);
    Containers::Optional<Trade::ImageData2D> data = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(data);
    CORRADE_INTERNAL_ASSERT(data->format() == PixelFormat::RGBA8Unorm);

    /* A scratch buffer user for texture upload as well as a download of the
       final rendered image */
    Vk::Buffer scratch{device, Vk::BufferCreateInfo{
        Vk::BufferUsage::TransferSource|Vk::BufferUsage::TransferDestination, Math::max<std::size_t>(800*600*4, data->data().size())
    }, Vk::MemoryFlag::HostVisible};
    Utility::copy(data->data(), scratch.dedicatedMemory().map().prefix(data->data().size()));

    /* Texture image. Gets copied from the buffer later. */
    Vk::Image textureImage{device, Vk::ImageCreateInfo2D{
        Vk::ImageUsage::TransferDestination|Vk::ImageUsage::Sampled,
        PixelFormat::RGBA8Srgb, data->size(), 1
    }, Vk::MemoryFlag::HostVisible};

    /* Texture view */
    Vk::ImageView textureView{device, Vk::ImageViewCreateInfo2D{textureImage}};

    /* Uniform buffer. A single color. */
    Vk::Buffer uniformBuffer{device, Vk::BufferCreateInfo{
        Vk::BufferUsage::UniformBuffer, 4*4
    }, Vk::MemoryFlag::HostVisible};
    {
        Containers::Array<char, Vk::MemoryMapDeleter> data = uniformBuffer.dedicatedMemory().map();
        Containers::arrayCast<Color4>(data)[0] = 0xffb2b2_srgbf;
    }

    /* Framebuffer */
    Vk::ImageView color{device, Vk::ImageViewCreateInfo2D{image}};
    Vk::Framebuffer framebuffer{device, Vk::FramebufferCreateInfo{renderPass, {
        color
    }, {800, 600}}};

    /* Sampler */
    Vk::Sampler sampler{device, Vk::SamplerCreateInfo{}
        .setMinificationFilter(SamplerFilter::Linear, SamplerMipmap::Linear)
        .setMagnificationFilter(SamplerFilter::Linear)
        .setWrapping(SamplerWrapping::ClampToEdge)
    };

    /* Create the shader */
    constexpr Containers::StringView assembly = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450

               OpEntryPoint Vertex %ver "ver" %position %textureCoordinates %gl_Position %interpolatedTextureCoordinatesOut
               OpEntryPoint Fragment %fra "fra" %interpolatedTextureCoordinatesIn %fragmentColor
               OpExecutionMode %fra OriginUpperLeft

               OpDecorate %position Location 0
               OpDecorate %textureCoordinates Location 1
               OpDecorate %gl_Position BuiltIn Position
               OpDecorate %interpolatedTextureCoordinatesOut Location 0
               OpDecorate %interpolatedTextureCoordinatesIn Location 0
               OpDecorate %fragmentColor Location 0

               OpDecorate %textureData DescriptorSet 0
               OpDecorate %textureData Binding 0

               OpDecorate %colorBlock Block
               OpMemberDecorate %colorBlock 0 Offset 0
               OpDecorate %color DescriptorSet 0
               OpDecorate %color Binding 1

       %void = OpTypeVoid
    %fn_void = OpTypeFunction %void
      %float = OpTypeFloat 32
       %vec2 = OpTypeVector %float 2
       %vec4 = OpTypeVector %float 4
%ptr_in_vec2 = OpTypePointer Input %vec2
%ptr_in_vec4 = OpTypePointer Input %vec4
   %position = OpVariable %ptr_in_vec4 Input
%textureCoordinates = OpVariable %ptr_in_vec2 Input
%ptr_out_vec2 = OpTypePointer Output %vec2
%ptr_out_vec4 = OpTypePointer Output %vec4
%gl_Position = OpVariable %ptr_out_vec4 Output
%interpolatedTextureCoordinatesOut = OpVariable %ptr_out_vec2 Output
%interpolatedTextureCoordinatesIn = OpVariable %ptr_in_vec2 Input
%fragmentColor = OpVariable %ptr_out_vec4 Output

; type, dimensions, depth, arrayed, multisampled, sampled, format
    %image2D = OpTypeImage %float 2D 0 0 0 1 Unknown
  %texture2D = OpTypeSampledImage %image2D
%ptr_uniform_texture2D = OpTypePointer UniformConstant %texture2D
%textureData = OpVariable %ptr_uniform_texture2D UniformConstant

        %int = OpTypeInt 32 1
    %c_zeroi = OpConstant %int 0
%ptr_uniform_vec4 = OpTypePointer Uniform %vec4
 %colorBlock = OpTypeStruct %vec4
%ptr_uniform_colorBlock = OpTypePointer Uniform %colorBlock
      %color = OpVariable %ptr_uniform_colorBlock Uniform

        %ver = OpFunction %void None %fn_void
       %ver_ = OpLabel
          %1 = OpLoad %vec4 %position
          %2 = OpLoad %vec2 %textureCoordinates
               OpStore %gl_Position %1
               OpStore %interpolatedTextureCoordinatesOut %2
               OpReturn
               OpFunctionEnd

        %fra = OpFunction %void None %fn_void
       %fra_ = OpLabel
          %3 = OpLoad %vec2 %interpolatedTextureCoordinatesIn
          %4 = OpAccessChain %ptr_uniform_vec4 %color %c_zeroi
          %5 = OpLoad %vec4 %4
          %6 = OpLoad %texture2D %textureData
          %7 = OpImageSampleImplicitLod %vec4 %6 %3
          %8 = OpFMul %vec4 %5 %7
               OpStore %fragmentColor %8
               OpReturn
               OpFunctionEnd
)"_s;
    Vk::Shader shader{device, Vk::ShaderCreateInfo{
        *CORRADE_INTERNAL_ASSERT_EXPRESSION(CORRADE_INTERNAL_ASSERT_EXPRESSION(
            PluginManager::Manager<ShaderTools::AbstractConverter>{}
                .loadAndInstantiate("SpirvAssemblyToSpirvShaderConverter")
        )->convertDataToData({}, assembly))}};

    /* Descriptor set layout, matching the shader above */
    Vk::DescriptorSetLayout descriptorSetLayout{device, Vk::DescriptorSetLayoutCreateInfo{{
        {{0, Vk::DescriptorType::CombinedImageSampler, {sampler}}},
        {{1, Vk::DescriptorType::UniformBuffer}}
    }}};

    Vk::DescriptorPool descriptorPool{device, Vk::DescriptorPoolCreateInfo{1, {
        {Vk::DescriptorType::CombinedImageSampler, 1},
        {Vk::DescriptorType::UniformBuffer, 1}
    }}};

    Vk::DescriptorSet descriptorSet = descriptorPool.allocate(descriptorSetLayout);
    {
        VkDescriptorImageInfo textureInfo{};
        textureInfo.imageView = textureView;
        textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorBufferInfo uniformBufferInfo{};
        uniformBufferInfo.buffer = uniformBuffer;
        uniformBufferInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet writes[2]{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = descriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].pImageInfo = &textureInfo;
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descriptorSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[1].pBufferInfo = &uniformBufferInfo;
        device->UpdateDescriptorSets(device, 2, writes, 0, nullptr);
    }

    Vk::PipelineLayout pipelineLayout{device, Vk::PipelineLayoutCreateInfo{
        descriptorSetLayout
    }};

    /* Create a graphics pipeline */
    Vk::ShaderSet shaderSet;
    shaderSet
        .addShader(Vk::ShaderStage::Vertex, shader, "ver"_s)
        .addShader(Vk::ShaderStage::Fragment, shader, "fra"_s);
    Vk::Pipeline pipeline{device, Vk::RasterizationPipelineCreateInfo{shaderSet, mesh.layout(), pipelineLayout, renderPass, 0, 1}
        .setViewport({{}, {800.0f, 600.0f}})
    };

    /* Record the command buffer:
        - upload the texture to device-local memory, wrapped in barriers for
          ensuring correct layout transfer
        - render pass begin converts the framebuffer attachment from Undefined
          to ColorAttachment and clears it
        - the pipeline barrier after is needed in order to make the image data
          copied to the buffer visible in time for the host read happening
          below */
    cmd.begin()
       .pipelineBarrier(Vk::PipelineStage::TopOfPipe, Vk::PipelineStage::Transfer, {
           {Vk::Accesses{}, Vk::Access::TransferWrite, Vk::ImageLayout::Undefined, Vk::ImageLayout::TransferDestination, textureImage}
        })
       .copyBufferToImage({scratch, textureImage, Vk::ImageLayout::TransferDestination, {
           Vk::BufferImageCopy2D{0, Vk::ImageAspect::Color, 0, {{}, data->size()}}
        }})
       .pipelineBarrier(Vk::PipelineStage::Transfer, Vk::PipelineStage::FragmentShader, {
           {Vk::Access::TransferWrite, Vk::Access::ShaderRead, Vk::ImageLayout::TransferDestination, Vk::ImageLayout::ShaderReadOnly, textureImage}
        })
       .beginRenderPass(Vk::RenderPassBeginInfo{renderPass, framebuffer}
           .clearColor(0, 0x1f1f1f_srgbf)
        )
       .bindPipeline(pipeline);

    {
        const VkDescriptorSet handle = descriptorSet; /* ew */
        device->CmdBindDescriptorSets(cmd, VkPipelineBindPoint(pipeline.bindPoint()), pipelineLayout, 0, 1, &handle, 0, nullptr);
    }

    cmd.draw(mesh)
       .endRenderPass()
       .copyImageToBuffer({image, Vk::ImageLayout::TransferSource, scratch, {
            Vk::BufferImageCopy2D{0, Vk::ImageAspect::Color, 0, {{}, {800, 600}}}
        }})
       .pipelineBarrier(Vk::PipelineStage::Transfer, Vk::PipelineStage::Host, {
            {Vk::Access::TransferWrite, Vk::Access::HostRead, scratch}
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
        scratch.dedicatedMemory().mapRead()
    }, "image.png");
    Debug{} << "Saved an image to image.png";
}
