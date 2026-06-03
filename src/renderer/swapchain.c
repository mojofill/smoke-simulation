#include "swapchain.h"

// Capabilities, formats, and present modes
void createSwapchainSupport(vk_context *vko) {
    // get surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vko->physicalDevice, vko->surface, &vko->surfaceCapabilities);

    // pick surface format
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vko->physicalDevice, vko->surface, &formatCount, NULL);
    VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vko->physicalDevice, vko->surface, &formatCount, formats);

    vko->surfaceFormat = formats[0];

    free(formats);

    // pick present mode
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vko->physicalDevice, vko->surface, &presentModeCount, NULL);
    VkSurfacePresentModeKHR *presentModes = malloc(presentModeCount * sizeof(VkSurfacePresentModeKHR));
    
    // pick mailbox as present mode if available, otherwise fifo (always available)
    vko->presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i].presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            vko->presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    free(presentModes);
}

void createSwapchain(vk_context *vko) {
    createSwapchainSupport(vko);

    // create swapchain
    VkSwapchainCreateInfoKHR swapchainInfo = {0};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = vko->surface;
    swapchainInfo.presentMode = vko->presentMode;
    swapchainInfo.imageExtent = vko->surfaceCapabilities.currentExtent; // current window size
    swapchainInfo.imageColorSpace = vko->surfaceFormat.colorSpace;
    swapchainInfo.imageFormat = vko->surfaceFormat.format;
    swapchainInfo.minImageCount = vko->surfaceCapabilities.minImageCount + 1; // one extra buffer - triple rendering
    if (vko->surfaceCapabilities.maxImageCount > 0 && swapchainInfo.minImageCount > vko->surfaceCapabilities.maxImageCount) {
        // if not device doesnt support, revert to max image count
        swapchainInfo.minImageCount = vko->surfaceCapabilities.maxImageCount;
    }
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // swapchain complies with render pass rules
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // images cannot be accessed concurrently
    swapchainInfo.preTransform = vko->surfaceCapabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.clipped = VK_TRUE; // pixels outside of scissor/window are not rendered
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    vkCreateSwapchainKHR(vko->device, &swapchainInfo, NULL, &vko->swapchain);

    vkGetSwapchainImagesKHR(vko->device, vko->swapchain, &vko->swapchainImageCount, NULL);
    vko->swapchainImages = malloc(sizeof(VkImage) * vko->swapchainImageCount);
    vkGetSwapchainImagesKHR(vko->device, vko->swapchain, &vko->swapchainImageCount, vko->swapchainImages);
}

void recreateSwapchain(vk_context *vko) {
    // pause until framebuffer is not minimized
    int width = 0, height = 0;
    glfwGetFramebufferSize(vko->window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(vko->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(vko->device);

    cleanupSwapchain(vko);

    createSwapchain(vko);
    createImageViews(vko);
    createFramebuffers(vko);
}

void cleanupSwapchain(vk_context *vko) {
    for (uint32_t i = 0; i < vko->swapchainImageCount; i++) {
        vkDestroyImageView(vko->device, vko->swapchainImageViews[i], NULL);
        vkDestroyFramebuffer(vko->device, vko->swapchainFramebuffers[i], NULL);
    }
    free(vko->swapchainImages);
    free(vko->swapchainFramebuffers);
    vkDestroySwapchainKHR(vko->device, vko->swapchain, NULL);
}

void createImageViews(vk_context *vko) {
    vko->swapchainImageViews = malloc(sizeof(VkImageView) * vko->swapchainImageCount);
    
    for (uint32_t i = 0; i < vko->swapchainImageCount; i++) {
        VkImageViewCreateInfo viewInfo = {0};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = vko->swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = vko->surfaceFormat.format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // rgba -> rgba. pure identity, no alteration
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseMipLevel = 0;

        if (vkCreateImageView(vko->device, &viewInfo, NULL, &vko->swapchainImageViews[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create swapchain image view %d\n", i);
            exit(1);
        }
    }
}

void createFramebuffers(vk_context *vko) {
    vko->swapchainFramebuffers = malloc(sizeof(VkFramebuffer) * vko->swapchainImageCount);
    
    for (uint32_t i = 0; i < vko->swapchainImageCount; i++) {
        // When using depth buffers, need two attachments
        // For now just one is enough
        // VkImageView attachments[2] = { vko->swapchainImageViews[i], vko->depthImageView}; // try manual array decay after
        VkImageView *attachments = &vko->swapchainImageViews[i];
        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vko->renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = vko->surfaceCapabilities.currentExtent.width;
        framebufferInfo.height = vko->surfaceCapabilities.currentExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vko->device, &framebufferInfo, NULL, &vko->swapchainFramebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffer %d!\n", i);
            exit(1);
        }
    }
}
