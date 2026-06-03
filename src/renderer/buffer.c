#include "buffer.h"

void setVertexBindingDescription(vk_context *vko) {
    vko->bindingDesc = (VkVertexInputBindingDescription) {0};
    
    vko->bindingDesc.binding = 0; // vertex buffer binding
    vko->bindingDesc.stride = sizeof(Vertex);
    vko->bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // other option = instance
}

void setVertexAttributeDescriptions(vk_context *vko) {
    // attr 0: vec2 inPos
    vko->attrDescs[0].binding = 0; // 0th vertex buffer
    vko->attrDescs[0].location = 0;
    vko->attrDescs[0].format = VK_FORMAT_R32G32_SFLOAT; // s stands for signed
    vko->attrDescs[0].offset = offsetof(Vertex, pos);

    // attr 1: vec3 color
    // vko->attrDescs[1].binding = 0;
    // vko->attrDescs[1].location = 1;
    // vko->attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    // vko->attrDescs[1].offset = offsetof(Vertex, color);
}

uint32_t findMemoryType(vk_context *vko, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vko->physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    fprintf(stderr, "Failed to find suitable memory\n");
    exit(1);
}

// create buffer + allocates memory, does not map/upload data
void createBuffer(vk_context *vko, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory) {
    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size; // for now, 3 vertices
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (pBuffer == NULL) {
        printf("bad buffer pointer\n");
        exit(1);
    }

    if (vkCreateBuffer(vko->device, &bufferInfo, NULL, pBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vertex buffer\n");
        exit(1);
    }

    // allocate memory for buffer
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vko->device, *pBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(vko, memRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(vko->device, &allocInfo, NULL, pBufferMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate memory for vertex buffer\n");
        exit(1);
    }

    vkBindBufferMemory(vko->device, *pBuffer, *pBufferMemory, 0);
}

VkCommandBuffer beginSingleTimeCommands(vk_context *vko) {
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = vko->commandPool; // may want to create separate command pool for short lived commands
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vko->device, &allocInfo, &commandBuffer);

    // begin recording copy command
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // only using buffer once, may help give vulkan optimizations

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(vk_context *vko, VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer); // end recording

    // must submit command
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(vko->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vko->graphicsQueue); // BAD! do not do this.
    // printf("(queue wait single time cmd) elapsed ms: %f\n", elapsed_ms);

    // one time use, thus immediately free buffer

    vkFreeCommandBuffers(vko->device, vko->commandPool, 1, &commandBuffer);
}

void copyBuffer(vk_context *vko, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    // must create, record, and submit a copy buffer command
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vko);

    VkBufferCopy copyRegion = {0};
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    endSingleTimeCommands(vko, commandBuffer);
}
