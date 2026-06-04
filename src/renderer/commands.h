#ifndef COMMANDS_H
#define COMMANDS_H

#include "vk_types.h"
#include "compute_pipeline.h"
#include "descriptors.h" // for ubo
#include "image.h" // for copyImage (for recording)
#include <string.h> // for memcpy

void createCommandPool(vk_context *vko);
void createCommandBuffers(vk_context *vko);
void resetCommands(vk_context *vko);
void recordCommands(vk_context *vko, uint32_t currentFrame, int imageIndex);

#endif