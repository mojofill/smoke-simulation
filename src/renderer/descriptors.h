#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "vk_types.h"

// NOT USING THIS!!!
typedef struct UniformBufferObject {
    int width; // this is needed for checkerboard pattern!
    int height;
    float density;
    float dt;
    float weightSOR;
} UniformBufferObject;

typedef struct PushConstants {
    int ping;
} PushConstants;

void createDescriptorSetLayout(vk_context *vko); // defines memory layout for vulkan
void createUniformBuffer(vk_context *vko);
void createDescriptorPool(vk_context *vko); // where uniforms are allocated
void createDescriptorSets(vk_context *vko); // where GPU resources like uniform buffers live
// other examples are storage buffers, texture samplers, etc

#endif