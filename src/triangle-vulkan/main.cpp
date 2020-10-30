/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>

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
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/ShaderTools/AbstractConverter.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <MagnumExternal/Vulkan/flextVk.h>
#include <MagnumExternal/Vulkan/flextVkGlobal.h>

#define MAGNUM_VK_ASSERT_OUTPUT(call) \
    do { \
        VkResult result = call; \
        if(result != VK_SUCCESS) Debug{} << result; \
        CORRADE_INTERNAL_ASSERT(result == VK_SUCCESS); \
    } while(false)

#define MAGNUM_VK_ASSERT_OUTPUT_INCOMPLETE(call) \
    do { \
        VkResult result = call; \
        if(result != VK_SUCCESS && result != VK_INCOMPLETE) Debug{} << result; \
        CORRADE_INTERNAL_ASSERT(result == VK_SUCCESS || result == VK_INCOMPLETE); \
    } while(false)

using namespace Magnum;
using namespace Magnum::Math::Literals;

int main() {
    /* Create an instance, load function pointers */
    VkInstance instance;
    {
        VkInstanceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        MAGNUM_VK_ASSERT_OUTPUT(vkCreateInstance(&info, nullptr, &instance));
    }
    flextVkInitInstance(instance, &flextVkInstance);

    /* Hardcoded memory type index (yes, I'm cheating) */
    constexpr UnsignedInt MemoryTypeIndex = 0;

    /* Create a device */
    VkPhysicalDevice physicalDevice;
    {
        UnsignedInt count = 1;
        MAGNUM_VK_ASSERT_OUTPUT_INCOMPLETE(vkEnumeratePhysicalDevices(instance, &count, &physicalDevice));
        CORRADE_INTERNAL_ASSERT(count == 1);
    } {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        Debug{} << "Using" << properties.deviceName;
        CORRADE_INTERNAL_ASSERT(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
                                properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
                                properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU);
    } {
        VkQueueFamilyProperties properties;
        UnsignedInt count = 1;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, &properties);
        CORRADE_INTERNAL_ASSERT(count == 1);
        CORRADE_INTERNAL_ASSERT(properties.queueFlags & VK_QUEUE_GRAPHICS_BIT);
    } {
        VkPhysicalDeviceMemoryProperties properties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
        CORRADE_INTERNAL_ASSERT(properties.memoryTypeCount >= 1);
        CORRADE_INTERNAL_ASSERT(properties.memoryHeapCount >= 1);
        CORRADE_INTERNAL_ASSERT(properties.memoryTypes[MemoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        CORRADE_INTERNAL_ASSERT(properties.memoryTypes[MemoryTypeIndex].heapIndex == 0);
    }
    VkDevice device;
    {
        const Float zero = 0.0f;

        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = 0; /* The first family from above */
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &zero;

        VkDeviceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.queueCreateInfoCount = 1;
        info.pQueueCreateInfos = &queueInfo;
        MAGNUM_VK_ASSERT_OUTPUT(vkCreateDevice(physicalDevice, &info, nullptr, &device));
    }

    flextVkInitDevice(device, &flextVkDevice, vkGetDeviceProcAddr);

    /* Create a queue */
    VkQueue queue;
    vkGetDeviceQueue(device, 0, 0, &queue);

    /* Allocate a command buffer */
    VkCommandPool commandPool;
    {
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        info.queueFamilyIndex = 0;
        MAGNUM_VK_ASSERT_OUTPUT(vkCreateCommandPool(device, &info, nullptr, &commandPool));
    }
    VkCommandBuffer commandBuffer;
    {
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = commandPool;
        info.commandBufferCount = 1;
        MAGNUM_VK_ASSERT_OUTPUT(vkAllocateCommandBuffers(device, &info, &commandBuffer));
    }

    /* Render pass */
    VkRenderPass renderPass;
    {
        VkAttachmentDescription color{};
        color.format = VK_FORMAT_R8G8B8A8_SRGB;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0; /* color output from the shader */
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription render{};
        render.colorAttachmentCount = 1;
        render.pColorAttachments = &colorRef;

        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &color;
        info.subpassCount = 1;
        info.pSubpasses = &render;
        MAGNUM_VK_ASSERT_OUTPUT(vkCreateRenderPass(device, &info, nullptr, &renderPass));
    }

    /* Framebuffer image */
    VkImage image;
    {
        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_SRGB;
        info.extent = {800, 600, 1};
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_LINEAR;
        info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        MAGNUM_VK_ASSERT_OUTPUT(vkCreateImage(device, &info, nullptr, &image));
    }
    VkDeviceMemory imageMemory;
    {
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(device, image, &requirements);
        CORRADE_INTERNAL_ASSERT(requirements.size == 800*600*4);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = MemoryTypeIndex; /* Assuming the type from above */
        MAGNUM_VK_ASSERT_OUTPUT(vkAllocateMemory(device, &info, nullptr, &imageMemory));
        MAGNUM_VK_ASSERT_OUTPUT(vkBindImageMemory(device, image, imageMemory, 0));
    }
    VkImageView color;
    {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = image;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_SRGB;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        MAGNUM_VK_ASSERT_OUTPUT(vkCreateImageView(device, &info, nullptr, &color));
    }

    /* Vertex buffer */
    VkBuffer buffer;
    {
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = 3*2*4*4; /* Three vertices, each is four-element pos & color */
        info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        MAGNUM_VK_ASSERT_OUTPUT(vkCreateBuffer(device, &info, nullptr, &buffer));
    }
    VkDeviceMemory bufferMemory;
    {
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(device, buffer, &requirements);
        CORRADE_INTERNAL_ASSERT(requirements.size == 3*2*4*4);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = MemoryTypeIndex; /* Assuming the type from above */
        MAGNUM_VK_ASSERT_OUTPUT(vkAllocateMemory(device, &info, nullptr, &bufferMemory));
        MAGNUM_VK_ASSERT_OUTPUT(vkBindBufferMemory(device, buffer, bufferMemory, 0));

        /* Fill the data */
        void* data;
        MAGNUM_VK_ASSERT_OUTPUT(vkMapMemory(device, bufferMemory, 0, VK_WHOLE_SIZE, 0, &data));
        auto view = Containers::arrayCast<Vector4>(Containers::arrayView(static_cast<char*>(data), 3*2*4*4));
        view[0] = {-0.5f, -0.5f, 0.0f, 1.0f}; /* Left vertex, red color */
        view[1] = 0xff0000ff_srgbaf;
        view[2] = { 0.5f, -0.5f, 0.0f, 1.0f}; /* Right vertex, green color */
        view[3] = 0x00ff00ff_srgbaf;
        view[4] = { 0.0f,  0.5f, 0.0f, 1.0f}; /* Top vertex, blue color */
        view[5] = 0x0000ffff_srgbaf;
        vkUnmapMemory(device, bufferMemory);
    }

    /* Framebuffer */
    VkFramebuffer framebuffer;
    {
        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = renderPass;
        info.attachmentCount = 1;
        info.pAttachments = &color;
        info.width = 800;
        info.height = 600;
        info.layers = 1;
        MAGNUM_VK_ASSERT_OUTPUT(vkCreateFramebuffer(device, &info, nullptr, &framebuffer));
    }

    /* Create the shader */
    VkShaderModule shader;
    {
        PluginManager::Manager<ShaderTools::AbstractConverter> manager;
        Containers::Pointer<ShaderTools::AbstractConverter> compiler = manager.loadAndInstantiate("SpirvAssemblyToSpirvShaderConverter");
        CORRADE_INTERNAL_ASSERT(compiler);

        using namespace Containers::Literals;

        const Containers::Array<char> code = compiler->convertDataToData({}, R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450

; Function %1 is vertex shader and has %12, %13 as input and %15, %16 as output
               OpEntryPoint Vertex %1 "ver" %12 %13 %gl_Position %16
; Function %2 is fragment shader and has %5 as input and %6 as output
               OpEntryPoint Fragment %2 "fra" %5 %6

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
)"_s);
        CORRADE_INTERNAL_ASSERT(code);

        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = code.size(); /* thanks Vulkan for accepting byte sizes */
        info.pCode = reinterpret_cast<const UnsignedInt*>(code.data());
        MAGNUM_VK_ASSERT_OUTPUT(vkCreateShaderModule(device, &info, nullptr, &shader));
    }

    /* Pipeline layout */
    VkPipelineLayout pipelineLayout;
    {
        VkPipelineLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.setLayoutCount = 0;
        info.pushConstantRangeCount = 0;
        MAGNUM_VK_ASSERT_OUTPUT(vkCreatePipelineLayout(device, &info, nullptr, &pipelineLayout));
    }

    /* Create a graphics pipeline */
    VkPipeline pipeline;
    {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = 2*4*4;

        VkVertexInputAttributeDescription attributes[2]{};
        attributes[0].location = 0; /* position attribute */
        attributes[0].binding = binding.binding;
        attributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[0].offset = 0;
        attributes[1].location = 1; /* color attribute */
        attributes[1].binding = binding.binding;
        attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[1].offset = 4*4;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &binding;
        vertexInputInfo.vertexAttributeDescriptionCount = 2;
        vertexInputInfo.pVertexAttributeDescriptions = attributes;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport{};
        viewport.width = 800.0f;
        viewport.height = 600.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{{}, {800, 600}};

        VkPipelineViewportStateCreateInfo viewportInfo{};
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = &viewport;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationInfo.lineWidth = 1.0f;
        /* the zero-filled defaults are good enough apparently */

        VkPipelineMultisampleStateCreateInfo multisampleInfo{};
        multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blend{};
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT|
                               VK_COLOR_COMPONENT_G_BIT|
                               VK_COLOR_COMPONENT_B_BIT|
                               VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
        colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendInfo.attachmentCount = 1;
        colorBlendInfo.pAttachments = &blend;

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = shader;
        stages[0].pName = "ver";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = shader;
        stages[1].pName = "fra";

        VkGraphicsPipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        info.stageCount = 2;
        info.pStages = stages;
        info.pVertexInputState = &vertexInputInfo;
        info.pInputAssemblyState = &inputAssemblyInfo;
        info.pViewportState = &viewportInfo;
        info.pRasterizationState = &rasterizationInfo;
        info.pMultisampleState = &multisampleInfo;
        info.pColorBlendState = &colorBlendInfo;
        info.layout = pipelineLayout;
        info.renderPass = renderPass;
        info.subpass = 0;
        MAGNUM_VK_ASSERT_OUTPUT(vkCreateGraphicsPipelines(device, nullptr, 1, &info, nullptr, &pipeline));
    }

    /* Begin recording */
    {
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        MAGNUM_VK_ASSERT_OUTPUT(vkBeginCommandBuffer(commandBuffer, &info));
    }

    /* Convert the image to the proper layout */
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    /* Begin a render pass, set up clear color */
    {
        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = renderPass;
        info.framebuffer = framebuffer;
        info.renderArea = VkRect2D{{}, {800, 600}};
        info.clearValueCount = 1;
        const Color4 color = 0x1f1f1f_srgbf;
        info.pClearValues = reinterpret_cast<const VkClearValue*>(&color);
        vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    /* Bind the pipeline */
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    /* Bind the vertex buffer */
    {
        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, &offset);
    }

    /* Draw the triangle */
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    /* End a render pass */
    vkCmdEndRenderPass(commandBuffer);

    /* End recording */
    MAGNUM_VK_ASSERT_OUTPUT(vkEndCommandBuffer(commandBuffer));

    /* Fence to wait on command buffer completeness */
    VkFence fence;
    {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        MAGNUM_VK_ASSERT_OUTPUT(vkCreateFence(device, &info, nullptr, &fence));
    }

    /* Submit the command buffer */
    {
        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &commandBuffer;
        MAGNUM_VK_ASSERT_OUTPUT(vkQueueSubmit(queue, 1, &info, fence));
    }

    /* Wait until done, 1 second max */
    MAGNUM_VK_ASSERT_OUTPUT(vkWaitForFences(device, 1, &fence, true, 1000000000ull));

    /* Read the image back */
    {
        void* data;
        MAGNUM_VK_ASSERT_OUTPUT(vkMapMemory(device, imageMemory, 0, VK_WHOLE_SIZE, 0, &data));

        ImageView2D image{PixelFormat::RGBA8Unorm, {800, 600}, Containers::arrayView(static_cast<char*>(data), 800*600*4)};
        PluginManager::Manager<Trade::AbstractImageConverter> manager;
        auto converter = manager.loadAndInstantiate("AnyImageConverter");
        CORRADE_INTERNAL_ASSERT(converter);
        converter->exportToFile(image, "image.png");
        Debug{} << "Saved an image to image.png";

        vkUnmapMemory(device, imageMemory);
    }

    /* Clean up */
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyShaderModule(device, shader, nullptr);
    vkDestroyFramebuffer(device, framebuffer, nullptr);
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, bufferMemory, nullptr);
    vkDestroyImageView(device, color, nullptr);
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, imageMemory, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}
