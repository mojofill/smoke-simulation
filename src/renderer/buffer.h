#ifndef BUFFER_H
#define BUFFER_H

#include "vk_types.h"

uint32_t findMemoryType(vk_context *vko, uint32_t typeFilter, VkMemoryPropertyFlags properties);
void setVertexBindingDescription(vk_context *vko);
void setVertexAttributeDescriptions(vk_context *vko);
void createBuffer(vk_context *vko, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory);
VkCommandBuffer beginSingleTimeCommands(vk_context *vko);
void endSingleTimeCommands(vk_context *vko, VkCommandBuffer commandBuffer);
void copyBuffer(vk_context *vko, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

#endif