#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include "vk_types.h"

void createSwapchainSupport(vk_context *vko);
void createSwapchain(vk_context *vko);
void recreateSwapchain(vk_context *vko); // during window resizes, swapchain needs to be recreated each time
void cleanupSwapchain(vk_context *vko);
void createImageViews(vk_context *vko); // swapchain consists of images (in vulkan's context, they are called image views)
void createFramebuffers(vk_context *vko); // these hold onto the image views (image views can be swapped around, hence swapchain)

#endif