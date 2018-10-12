/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
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

#include <Corrade/Utility/Assert.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Magnum.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <MagnumExternal/Vulkan/flextVk.h>
#include <MagnumExternal/Vulkan/flextVkGlobal.h>

#include "spirv.h"

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
        auto o = [](UnsignedShort length, UnsignedShort opcode) {
            return UnsignedInt(length << 16 | opcode);
        };
        auto s = [](const char(&s)[5]) {
            return UnsignedInt(s[3] << 24 | s[2] << 16 | s[1] << 8 | s[0]);
        };
        const UnsignedInt code[]{
            /* I am the generator, I have 66 IDs max, instruction schema is 0 */
            SpvMagicNumber, SpvVersion, 0, 66, 0,

            o(2, SpvOpCapability), SpvCapabilityShader,
            o(3, SpvOpMemoryModel), SpvAddressingModelLogical, SpvMemoryModelGLSL450,

            /* Function %1 is vertex shader and has %12, %13 as input and %15, %16 as output */
            o(8, SpvOpEntryPoint), SpvExecutionModelVertex, 1, s("ver\0"), 12, 13, 15, 16,

            /* Function %2 is fragment shader and has %5 as input and %6 as output */
            o(6, SpvOpEntryPoint), SpvExecutionModelFragment, 2, s("fra\0"), 5, 6,

            /* Input/output layouts */
            o(4, SpvOpDecorate), 12, SpvDecorationLocation, 0, /* in position = 0 */
            o(4, SpvOpDecorate), 13, SpvDecorationLocation, 1, /* in color = 1 */
            o(4, SpvOpDecorate), 15, SpvDecorationBuiltIn, SpvBuiltInPosition, /* out position = gl_Position */
            o(4, SpvOpDecorate), 16, SpvDecorationLocation, 0, /* out color = 0 */
            o(4, SpvOpDecorate), 5, SpvDecorationLocation, 0, /* in color = 0 */
            o(4, SpvOpDecorate), 6, SpvDecorationLocation, 0, /* out color = 0 */

            /* Types */
            o(2, SpvOpTypeVoid), 7,                 /* %7 = void */
            o(3, SpvOpTypeFunction), 8, 7,          /* %8 = void () */
            o(3, SpvOpTypeFloat), 9, 32,            /* %9 = float */
            o(4, SpvOpTypeVector), 10, 9, 4,        /* %10 = vec4 */

            o(4, SpvOpTypePointer), 11, SpvStorageClassInput, 10, /* %11 = in vec4* */
            o(4, SpvOpVariable), 11, 12, SpvStorageClassInput, /* %12 = in position */
            o(4, SpvOpVariable), 11, 13, SpvStorageClassInput, /* %13 = in color */

            o(4, SpvOpTypePointer), 14, SpvStorageClassOutput, 10, /* %14 = out vec4* */
            o(4, SpvOpVariable), 14, 15, SpvStorageClassOutput, /* %15 = out position */
            o(4, SpvOpVariable), 14, 16, SpvStorageClassOutput, /* %16 = out color */

            o(4, SpvOpVariable), 11, 5, SpvStorageClassInput, /* %5 = frag in color */
            o(4, SpvOpVariable), 14, 6, SpvStorageClassOutput, /* %6 = frag out color */

            /* %1 = void ver() */
            o(5, SpvOpFunction), 7, 1, SpvFunctionControlMaskNone, 8,
            o(2, SpvOpLabel), 33,
            o(4, SpvOpLoad), 10, 30, 12,    /* %30 = load position as vec4 */
            o(4, SpvOpLoad), 10, 31, 13,    /* %31 = load color as vec4 */
            o(3, SpvOpStore), 15, 30,       /* store position from %30 */
            o(3, SpvOpStore), 16, 31,       /* store color from %31 */
            o(1, SpvOpReturn),
            o(1, SpvOpFunctionEnd),

            /* %2 = void fra() */
            o(5, SpvOpFunction), 7, 2, SpvFunctionControlMaskNone, 8,
            o(2, SpvOpLabel), 34,
            o(4, SpvOpLoad), 10, 32, 5,     /* %32 = load color as vec4 */
            o(3, SpvOpStore), 6, 32,        /* store color from %32 */
            o(1, SpvOpReturn),
            o(1, SpvOpFunctionEnd)
        };

        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = sizeof(code);
        info.pCode = code;
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
