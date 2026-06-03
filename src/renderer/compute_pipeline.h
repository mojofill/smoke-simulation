#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include "vk_types.h"
#include "descriptors.h"
#include "graphics_pipeline.h"

void createComputePipeline(
    vk_context *vko,
    VkPipeline *pComputePipeline,
    VkPipelineLayout *pComputePipelineLayout,
    VkDescriptorSetLayout *pDescriptorSetLayout,
    const char *compute_path
);
void createImageMemoryBarrier(
    VkImage image,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    VkCommandBuffer commandBuffer
);

#endif