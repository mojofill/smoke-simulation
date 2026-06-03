#include "compute_pipeline.h"

void createComputePipeline(vk_context *vko, VkPipeline *pComputePipeline, VkPipelineLayout *pComputePipelineLayout, VkDescriptorSetLayout *pDescriptorSetLayout, const char *compute_path) {
    VkPipelineLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1; // how many descriptor set layouts there are
    layoutInfo.pSetLayouts = pDescriptorSetLayout; // use &vko->descriptorSetLayout

    VkPushConstantRange pushConstantRange = {0};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    layoutInfo.pushConstantRangeCount = 1; // not push constants for now
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(vko->device, &layoutInfo, NULL, pComputePipelineLayout) != VK_SUCCESS) {
        printf("failed to create compute pipeline layout\n");
        exit(1);
    }

    VkShaderModule module = load_shader(vko->device, compute_path);

    VkPipelineShaderStageCreateInfo shaderStageInfo = {0};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = module;
    shaderStageInfo.pName = "main";
    
    VkComputePipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = *pComputePipelineLayout;
    pipelineInfo.stage  = shaderStageInfo;

    if (vkCreateComputePipelines(vko->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pComputePipeline) != VK_SUCCESS) {
        printf("failed to create compute pipeline\n");
        exit(1);
    }

    vkDestroyShaderModule(vko->device, module, NULL);
}

void createImageMemoryBarrier(VkImage image, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer commandBuffer) {
    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL; // more expensive to keep switching back & forth from general to optimal
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    // synchronization settings
    
    if (srcStage == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT && dstStage == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else if (srcStage == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT && dstStage == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStage,
        dstStage,
        0, 0, NULL, 0, NULL, 1, &barrier
    );
}
