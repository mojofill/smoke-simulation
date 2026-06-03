#include "descriptors.h"
#include "buffer.h"

// should figure out a way to abstract this. ie maybe a descriptor factory, pass in arguments one by one, then
// call a "make all" function that does all these steps at once

void createDescriptorSetLayout(vk_context *vko) {
    VkDescriptorSetLayoutBinding bindings[11]; // smoke map, horizontal, vertical, pressure, and solid
    // x2 for ping pong

    for (int i = 0; i < 10; i++) {
        VkDescriptorSetLayoutBinding imageLayoutBinding = {0};
        imageLayoutBinding.binding = i;
        imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        imageLayoutBinding.descriptorCount = 1;
        imageLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[i] = imageLayoutBinding;
    }

    VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
    uboLayoutBinding.binding = 10;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[10] = uboLayoutBinding;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pBindings = bindings;
    layoutInfo.bindingCount = 11;

    if (vkCreateDescriptorSetLayout(vko->device, &layoutInfo, NULL, &vko->descriptorSetLayout) != VK_SUCCESS) {
        printf("Failed to create descriptor set layout\n");
        exit(1);
    }
}

void createUniformBuffer(vk_context *vko) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    // create one for each image in flight (ie uniform buffer count)
    for (int i = 0; i < UNIFORM_BUFFER_COUNT; i++) {
        createBuffer(vko, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vko->uniformBuffers[i], &vko->uniformBufferMemories[i]);
        vkMapMemory(vko->device, vko->uniformBufferMemories[i], 0, bufferSize, 0, &vko->uniformBuffersMapped[i]);
    }
}

void createDescriptorPool(vk_context *vko) {
    VkDescriptorPoolSize poolSizes[2]; // smoke map, horizontal, and vertical velocities

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT * 10 ; // each frame need 5 (x2 for ping pong)
    
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT * 1; // only one ubo per frame in flight

    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = (uint32_t) MAX_FRAMES_IN_FLIGHT * 2; // 2 descriptor sets per frame

    if (vkCreateDescriptorPool(vko->device, &poolInfo, NULL, &vko->descriptorPool) != VK_SUCCESS) {
        printf("failed to create descriptor pool\n");
        exit(1);
    }
}

void createDescriptorSets(vk_context *vko) {
    // need ping pong to not have race condition
    for (int ping = 0; ping < 2; ping++) {
        VkDescriptorSetAllocateInfo allocInfo = {0};

        // need identical layouts for each MAX_FRAMES_IN_FLIGHT descriptor set
        VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {0};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) layouts[i] = vko->descriptorSetLayout;

        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = vko->descriptorPool;
        allocInfo.descriptorSetCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = layouts;

        if (vkAllocateDescriptorSets(vko->device, &allocInfo, vko->descriptorSets[ping]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create descriptor sets\n");
            exit(1);
        }
        // populate descriptors
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            // write to descriptor set
            VkWriteDescriptorSet descriptorWrites[11] = {0}; // smoke map, horizontal, and vertical, pressure, and solid
            // since ping pong, times 2
            // + 1 for uniform buffer object

            int ping_opp = ping == 0 ? 1 : 0;

            VkImageView imageViews[10] = {
                // input
                vko->smokeMapImageViews[ping],
                vko->horizontalVelocitiesImageViews[ping],
                vko->verticalVelocitiesImageViews[ping],
                vko->pressureMapImageViews[ping],
                vko->solidMapImageViews[ping],

                // output
                vko->smokeMapImageViews[ping_opp],
                vko->horizontalVelocitiesImageViews[ping_opp],
                vko->verticalVelocitiesImageViews[ping_opp],
                vko->pressureMapImageViews[ping_opp],
                vko->solidMapImageViews[ping_opp]
            };
            VkDescriptorImageInfo imageInfos[10] = {0};

            for (int j = 0; j < 10; j++) {
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageInfos[j].imageView = imageViews[j];

                descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[j].dstSet = vko->descriptorSets[ping][i];
                descriptorWrites[j].dstBinding = j; // layout (binding = j) in shader
                descriptorWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                descriptorWrites[j].descriptorCount = 1;
                descriptorWrites[j].pImageInfo = &imageInfos[j];
            }

            // add uniform buffer object
            VkDescriptorBufferInfo bufferInfo = {0};
            bufferInfo.buffer = vko->uniformBuffers[ping];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            descriptorWrites[10].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[10].dstSet = vko->descriptorSets[ping][i];
            descriptorWrites[10].dstBinding = 10; // layout (binding = 10) in shader
            descriptorWrites[10].dstArrayElement = 0;
            descriptorWrites[10].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[10].descriptorCount = 1;
            descriptorWrites[10].pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(vko->device, 11, descriptorWrites, 0, NULL);
        }
    }
}
