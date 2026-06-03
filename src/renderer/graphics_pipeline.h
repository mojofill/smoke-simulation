#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include "vk_types.h"
#include "swapchain.h"
#include <assert.h>

void createPipeline(
    vk_context *vko,
    const char *vert_path,
    const char *frag_path,
    VkPrimitiveTopology topology,
    VkVertexInputBindingDescription *bindingDescs,
    int bindingDescriptionCount,
    VkVertexInputAttributeDescription *attrDescs,
    int attrDesciptionCount,
    int depthTestEnable,
    int depthWriteEnable,
    VkCullModeFlagBits cullMode,
    VkPipeline *destPipeline
);
void createGraphicsPipeline(vk_context *vko);
VkShaderModule load_shader(VkDevice device, const char* path);

#endif