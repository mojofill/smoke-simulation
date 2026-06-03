#include "renderer.h"

static void initWindow(vk_context *vko) {
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        exit(1);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // necessary to use Vulkan
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); 

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    vko->width = mode->width;
    vko->height = mode->height;
    vko->window = glfwCreateWindow(vko->width, vko->height, "Smoke Simulation", monitor, NULL);
    if (!vko->window) {
        printf("Failed to create GLFW window\n");
        exit(1);
    }

    glfwSetWindowPos(vko->window, 0, 0);
    glfwSetWindowUserPointer(vko->window, vko); // easy usage of god object within GLFW
    glfwSetFramebufferSizeCallback(vko->window, framebufferResizeCallback);
    glfwSetInputMode(vko->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

static void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    vk_context *vko = glfwGetWindowUserPointer(window);
    vko->framebufferResized = 1; // flag to update window in main loop
}

static void createInstance(vk_context *vko) {
    VkApplicationInfo appInfo = {0}; 
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Smoke Simulation";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    uint32_t glfwExtensionCount;
    
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    uint32_t extensionCount = glfwExtensionCount;
    #ifdef __APPLE__
        extensionCount++;
    #endif
    const char **extensions = malloc(sizeof(char*) * extensionCount);

    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        extensions[i] = glfwExtensions[i];
    }

    #ifdef __APPLE__
        extensions[glfwExtensionCount] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    #endif

    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    // validation layers

    const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties* availableLayers =
        malloc(sizeof(VkLayerProperties) * layerCount);

    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    int validationLayerFound = 0;

    for (uint32_t i = 0; i < layerCount; i++) {
        if (strcmp(availableLayers[i].layerName,
                "VK_LAYER_KHRONOS_validation") == 0) {
            validationLayerFound = 1;
            break;
        }
    }

    if (validationLayerFound) {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = layers;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = NULL;
    }

    free(availableLayers);

    #ifdef __APPLE__
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    VkResult res = vkCreateInstance(&createInfo, NULL, &vko->instance);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance failed: %d\n", res);
        exit(1);
    }    
}

static void createSurface(vk_context *vko) {
    if (glfwCreateWindowSurface(vko->instance, vko->window, NULL, &vko->surface) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create surface\n");
        exit(1);
    }
}

static void pickPhysicalDevice(vk_context *vko) {
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(vko->instance, &physicalDeviceCount, NULL);
    VkPhysicalDevice *physicalDevices = malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
    vkEnumeratePhysicalDevices(vko->instance, &physicalDeviceCount, physicalDevices);

    vko->physicalDevice = physicalDevices[0]; // Pick first physical device found
    free(physicalDevices);
}

static void findQueueFamilies(vk_context *vko) {
    vkGetPhysicalDeviceQueueFamilyProperties(vko->physicalDevice, &vko->queueFamilyCount, NULL);
    VkQueueFamilyProperties *queueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties) * vko->queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vko->physicalDevice, &vko->queueFamilyCount, queueFamilyProperties);

    vko->graphicsFamilyIndex = -1;
    vko->presentFamilyIndex = -1;

    for (uint32_t i = 0; i < vko->queueFamilyCount; i++) {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            vko->graphicsFamilyIndex = i;
        }

        VkBool32 presentSupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(vko->physicalDevice, i, vko->surface, &presentSupport);
        if (presentSupport) {
            vko->presentFamilyIndex = i;
        }

        if (vko->graphicsFamilyIndex != -1 && vko->presentFamilyIndex != -1) break;
    }

    if (vko->graphicsFamilyIndex == -1 || vko->presentFamilyIndex == -1) {
        fprintf(stderr, "Failed to find required queue families\ngraphics family: %d\npresent family: %d", vko->graphicsFamilyIndex, vko->presentFamilyIndex);
        exit(1);
    }
}

static void createLogicalDevice(vk_context *vko) {
    uint32_t extensionCount = 1;

    #ifdef __APPLE__
        extensionCount = 2;
    #endif

    const char* deviceExtensions[2];
    deviceExtensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    #ifdef __APPLE__
        deviceExtensions[1] = VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
    #endif

    // create device queue
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfos[2] = {0};
    queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[0].queueFamilyIndex = vko->graphicsFamilyIndex;
    queueCreateInfos[0].queueCount = 1;
    queueCreateInfos[0].pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {0};
    deviceFeatures.samplerAnisotropy = VK_TRUE; // must add device feature sampler anisotropy
    deviceFeatures.fillModeNonSolid = VK_TRUE;

    VkPhysicalDeviceVulkan12Features v12Features = {0};
    v12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    v12Features.separateDepthStencilLayouts = VK_TRUE;

    // create logical device
    VkDeviceCreateInfo deviceCreateInfo = {0};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &v12Features;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.enabledExtensionCount = extensionCount;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    if (vkCreateDevice(vko->physicalDevice, &deviceCreateInfo, NULL, &vko->device) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device\n");
        exit(1);
    }

    // must grab queue handle from logical device
    vkGetDeviceQueue(vko->device, vko->graphicsFamilyIndex, 0, &vko->graphicsQueue);
}

static void createRenderpass(vk_context *vko) {
    // (color/depth/any type of) attachment = describes how to use image view for a specific part of the render pass
    // image view = structured way to view or access the image (raw memory)
    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = vko->surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear previous data
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // image will be used for presentation

    // image layout = physical memory arrangement (what bytes go where)

    VkAttachmentReference colorAttachmentRef = {0}; // this is how a subpass gets access to the attachment
    colorAttachmentRef.attachment = 0; // pAttachments[0] - there is an array of attachments, our attachment index is 0 b/c we only have one attachment right now
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // depth buffer attachments
    VkAttachmentDescription depthAttachment = {0};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {0};
    depthAttachmentRef.attachment = 1; // links to around line 246 - VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment} - this is what render pass "references"
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

    // create the subpass. right now, we just have one subpass. this is the vertex -> fragment stuff
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // running GRAPHICS pipeline - not a compute pipeline
    subpass.colorAttachmentCount = 1; // one SINGULAR color output, which is attachment #0. attachment = "logical" i
    subpass.pColorAttachments = &colorAttachmentRef; // array decay, for single length array just pointer to first
    
    // Uncomment this to add depth testing for 3D simulations
    // subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // think negative 1 when srcSubpass is VK_SUBPASS_EXTERNAL, and max+1 when it is dstSubpass (dst = destination)
    dependency.dstSubpass = 0; // this is for the first implicit subpass from initial layout to render pass layout
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // only when subpass 0 finished can pipeline move onto this stage
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // tells last implicit subpass to wait for subpass 0 t finish before writing to color attachment

    // VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};
    VkAttachmentDescription *attachments = &colorAttachment;

    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1; // only one, cuz no depth stencil
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(vko->device, &renderPassInfo, NULL, &vko->renderPass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass\n");
        exit(1);
    }
}

// No uniforms to put in. Populate these when adding uniforms to your program
// static void createDescriptorSetLayout(vk_context *vko)
// static void createCameraUniformBuffer(vk_context *vko) Rename this/create as many as necessary
// static void createDescriptorPool(vk_context *vko)
// static void createDescriptorSets(vk_context *vko)

static void createSyncObjects(vk_context *vko) {
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // signal start so that main loop doesnt keep waiting for it to start

    // allocate space for the sync objects
    vko->imageAvailableSemaphores = malloc(sizeof(VkSemaphore) * vko->swapchainImageCount);
    vko->renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * vko->swapchainImageCount);
    vko->inFlightFences = malloc(sizeof(VkFence) * vko->swapchainImageCount);

    for (uint32_t i = 0; i < 2; i++) {
        if (vkCreateSemaphore(vko->device, &semaphoreInfo, NULL, &vko->imageAvailableSemaphores[i]) != VK_SUCCESS || 
            vkCreateSemaphore(vko->device, &semaphoreInfo, NULL, &vko->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vko->device, &fenceInfo, NULL, &vko->inFlightFences[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create sync objects.\n");
            exit(1);
        }
    }
}

// public rendering functions
void drawFrame(vk_context *vko, uint32_t *currentFrame) {
    // wait for previous frame to finish
    vkWaitForFences(vko->device, 1, &vko->inFlightFences[*currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vko->device, vko->swapchain, UINT64_MAX, vko->imageAvailableSemaphores[*currentFrame], VK_NULL_HANDLE, &imageIndex); // signal imageAvailableSemaphore when image is available.

    if (result == VK_ERROR_OUT_OF_DATE_KHR) { // usually happens after window resize
        recreateSwapchain(vko);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "Failed to acquire swapchain image.\n");
        exit(1);
    }

    // only reset to use fence again if we are submitting work
    vkResetFences(vko->device, 1, &vko->inFlightFences[*currentFrame]);

    // if i were recording every frame, i would need to 1. vkResetCommandBuffer(commandBuffer, 0); to clear command buffer and 2. recordCommandBuffer(commandBuffer, imageIndex);
    // but since everything is pre recorded, i dont need to

    recordCommands(vko, *currentFrame, imageIndex);

    // submit commands to queue
    VkSubmitInfo submitInfo = {0};
    // explicitly tell gpu to wait until imageAvailableSemaphore signals ready before pipeline outputs to color attachment
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; // first stage synced first semaphore.
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vko->commandBuffers[imageIndex];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vko->imageAvailableSemaphores[*currentFrame]; // wait for imageAvailable semaphore to signal done before rendering
    submitInfo.pWaitDstStageMask = waitStages; // pipeline stages are synced with wait semaphores - doesnt continue onto stage until wait semaphore signals done
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &vko->renderFinishedSemaphores[*currentFrame]; // signal renderFinished semaphore when done

    if (vkQueueSubmit(vko->graphicsQueue, 1, &submitInfo, vko->inFlightFences[*currentFrame]) != VK_SUCCESS) {
        fprintf(stderr, "Failed to submit draw command buffer to graphics queue\n");
        exit(1);
    }

    // 3. Present the rendered image
    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &vko->renderFinishedSemaphores[*currentFrame]; // wait until rendering finished
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vko->swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = NULL;

    result = vkQueuePresentKHR(vko->graphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vko->framebufferResized) {
        vko->framebufferResized = 0;
        recreateSwapchain(vko);
        // rerecord command buffers
        resetCommands(vko);
        recordCommands(vko, *currentFrame, imageIndex);
    } else if (result != VK_SUCCESS) {
        printf("Failed to present swapchain image!\n");
        exit(1);
    }

    *currentFrame = (*currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    vko->ping = vko->ping == 0 ? 1 : 0; // update ping
    
    vkQueueWaitIdle(vko->graphicsQueue);
}

static void initVulkan(vk_context *vko) {
    vko->framebufferResized = 0; // initialize framebuffer resizing flag to 0 (off)
    createInstance(vko);
    pickPhysicalDevice(vko);
    createSurface(vko);
    findQueueFamilies(vko);
    createLogicalDevice(vko);
    createSwapchain(vko);
    createImageViews(vko); // image view = vulkan interpretation of raw image source
    createCommandPool(vko);
    createUniformBuffer(vko);
    
    for (int ping = 0; ping < 2; ping++) {
        createImage( // smoke map for a fullscreen quad
            vko,
            vko->width,
            vko->height,
            VK_FORMAT_R32_SFLOAT,
            VK_IMAGE_TILING_OPTIMAL,                                  // need transfer for staging
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // all of these will have staging copies
            &vko->smokeMapImages[ping], &vko->smokeMapImageMemories[ping]
        );
        createImageView(vko, vko->smokeMapImages[ping], VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &vko->smokeMapImageViews[ping]);

        createImage( // velocities x map
            vko,
            vko->width + 1, // +1 for horizontal
            vko->height,
            VK_FORMAT_R32_SFLOAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &vko->horizontalVelocitiesImages[ping], &vko->horizontalVelocitiesImageMemories[ping]
        );
        createImageView(vko, vko->horizontalVelocitiesImages[ping], VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &vko->horizontalVelocitiesImageViews[ping]);

        createImage( // velocities y map
            vko,
            vko->width,
            vko->height + 1, // +1 for vertical
            VK_FORMAT_R32_SFLOAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &vko->verticalVelocitiesImages[ping], &vko->verticalVelocitiesImageMemories[ping]
        );
        createImageView(vko, vko->verticalVelocitiesImages[ping], VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &vko->verticalVelocitiesImageViews[ping]);

        createImage( // pressure map
            vko,
            vko->width,
            vko->height,
            VK_FORMAT_R32_SFLOAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &vko->pressureMapImages[ping], &vko->pressureMapImageMemories[ping]
        );
        createImageView(vko, vko->pressureMapImages[ping], VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &vko->pressureMapImageViews[ping]);

        createImage( // solid map
            vko,
            vko->width,
            vko->height,
            VK_FORMAT_R8_UINT, // just to know one or zero
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &vko->solidMapImages[ping], &vko->solidMapImageMemories[ping]
        );
        createImageView(vko, vko->solidMapImages[ping], VK_FORMAT_R8_UINT, VK_IMAGE_ASPECT_COLOR_BIT, &vko->solidMapImageViews[ping]);
    }

    createDescriptorSetLayout(vko);
    createDescriptorPool(vko);
    createDescriptorSets(vko);
    createRenderpass(vko);
    setVertexAttributeDescriptions(vko);
    setVertexBindingDescription(vko);
    createGraphicsPipeline(vko);
    createComputePipeline(
        vko,
        &vko->pressureSolveComputePipeline,
        &vko->pressureSolveComputePipelineLayout,
        &vko->descriptorSetLayout,
        "./src/spvs/pressureSolve.spv"
    );
    createComputePipeline(
        vko,
        &vko->advectVelocitiesComputePipeline,
        &vko->advectVelocitiesComputePipelineLayout,
        &vko->descriptorSetLayout,
        "./src/spvs/advectVelocities.spv"
    );
    createComputePipeline(
        vko,
        &vko->advectSmokeComputePipeline,
        &vko->advectSmokeComputePipelineLayout,
        &vko->descriptorSetLayout,
        "./src/spvs/advectSmoke.spv"
    );
    createComputePipeline(
        vko,
        &vko->updateVelocitiesComputePipeline,
        &vko->updateVelocitiesComputePipelineLayout,
        &vko->descriptorSetLayout,
        "./src/spvs/updateVelocities.spv"
    );
    createFramebuffers(vko);
    createCommandBuffers(vko);
    createSyncObjects(vko);
}

void cleanupRenderer(vk_context *vko) {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vko->device, vko->imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore(vko->device, vko->renderFinishedSemaphores[i], NULL);
        vkDestroyFence(vko->device, vko->inFlightFences[i], NULL);
    }
    vkDestroyCommandPool(vko->device, vko->commandPool, NULL);
    vkDestroyPipeline(vko->device, vko->graphicsPipeline, NULL);
    vkDestroyPipeline(vko->device, vko->pressureSolveComputePipeline, NULL);
    vkDestroyPipeline(vko->device, vko->advectVelocitiesComputePipeline, NULL);
    vkDestroyPipeline(vko->device, vko->advectSmokeComputePipeline, NULL);
    vkDestroyPipeline(vko->device, vko->updateVelocitiesComputePipeline, NULL);
    vkDestroyPipelineLayout(vko->device, vko->graphicsPipelineLayout, NULL);
    vkDestroyPipelineLayout(vko->device, vko->pressureSolveComputePipelineLayout, NULL);
    vkDestroyPipelineLayout(vko->device, vko->advectVelocitiesComputePipelineLayout, NULL);
    vkDestroyPipelineLayout(vko->device, vko->advectSmokeComputePipelineLayout, NULL);
    vkDestroyPipelineLayout(vko->device, vko->updateVelocitiesComputePipelineLayout, NULL);
    vkDestroyRenderPass(vko->device, vko->renderPass, NULL);
    for (int ping = 0; ping < 2; ping++) {
        vkDestroyImageView(vko->device, vko->smokeMapImageViews[ping], NULL);
        vkDestroyImage(vko->device, vko->smokeMapImages[ping], NULL);
        vkFreeMemory(vko->device, vko->smokeMapImageMemories[ping], NULL);

        vkDestroyImageView(vko->device, vko->horizontalVelocitiesImageViews[ping], NULL);
        vkDestroyImage(vko->device, vko->horizontalVelocitiesImages[ping], NULL);
        vkFreeMemory(vko->device, vko->horizontalVelocitiesImageMemories[ping], NULL);

        vkDestroyImageView(vko->device, vko->verticalVelocitiesImageViews[ping], NULL);
        vkDestroyImage(vko->device, vko->verticalVelocitiesImages[ping], NULL);
        vkFreeMemory(vko->device, vko->verticalVelocitiesImageMemories[ping], NULL);

        vkDestroyImageView(vko->device, vko->pressureMapImageViews[ping], NULL);
        vkDestroyImage(vko->device, vko->pressureMapImages[ping], NULL);
        vkFreeMemory(vko->device, vko->pressureMapImageMemories[ping], NULL);

        vkDestroyImageView(vko->device, vko->solidMapImageViews[ping], NULL);
        vkDestroyImage(vko->device, vko->solidMapImages[ping], NULL);
        vkFreeMemory(vko->device, vko->solidMapImageMemories[ping], NULL);
    }
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(vko->device, vko->uniformBuffers[i], NULL);
        vkFreeMemory(vko->device, vko->uniformBufferMemories[i], NULL);
    }
    vkDestroyDescriptorPool(vko->device, vko->descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(vko->device, vko->descriptorSetLayout, NULL);
    vkDestroyBuffer(vko->device, vko->vertexBuffer, NULL);
    vkFreeMemory(vko->device, vko->vertexBufferMemory, NULL);
    cleanupSwapchain(vko);
    vkDestroySurfaceKHR(vko->instance, vko->surface, NULL);
    vkDestroyDevice(vko->device, NULL);
    vkDestroyInstance(vko->instance, NULL);

    glfwDestroyWindow(vko->window);
    glfwTerminate();
}

void initRenderer(vk_context *vko) {
    initWindow(vko);
    initVulkan(vko);
}
