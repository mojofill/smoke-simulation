#include "renderer/renderer.h"
#include <stdio.h>

// #define RECORD

float randf() {
    return rand() / (float) RAND_MAX;
}

void updateUniforms(vk_context *vko) {
    UniformBufferObject ubo = {0};
    ubo.width = vko->width;
    ubo.height = vko->height;
    ubo.density = 1.0f;
    ubo.dt = 0.01;
    ubo.weightSOR = 1.5f;
    memcpy(vko->uniformBuffersMapped[0], &ubo, sizeof(UniformBufferObject));
    memcpy(vko->uniformBuffersMapped[1], &ubo, sizeof(UniformBufferObject));
}

void initializeStorageImages(vk_context *vko);

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action != GLFW_RELEASE) return;
    vk_context *vko = glfwGetWindowUserPointer(window);
    if (key == GLFW_KEY_SPACE || key == GLFW_KEY_P) {
        vko->pause = vko->pause == 0 ? 1 : 0;
    }
    else if (key == GLFW_KEY_R) {
        initializeStorageImages(vko);
    }
}

void mainLoop(vk_context *vko) {
    uint32_t currentFrame = 0;

    glfwSetKeyCallback(vko->window, keyCallback);
    updateUniforms(vko); // only gotta do this once actually

    vko->pause = 0;

    while (!glfwWindowShouldClose(vko->window)) {
        glfwPollEvents();
        if (glfwGetKey(vko->window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(vko->window, VK_TRUE);
        }

        if (!vko->pause) {
            drawFrame(vko, &currentFrame);
        }
    }
}

void initializeVertexBuffer(vk_context *vko) {
    // staging
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    // recall that order of vertices is important as well. need to make sure right hand rule works
    // y is flipped, so -1 is top and 1 is bottom
    Vertex vertices[] = {     // don't need color
        { { -1.0f, -1.0f } }, // { 1.0f, 0.0f, 0.0f } }, // top left
        { {  1.0f,  1.0f } }, // { 0.0f, 1.0f, 0.0f } }, // bottom right
        { { -1.0f,  1.0f } }, // { 0.0f, 0.0f, 1.0f } }, // bottom left
        { {  1.0f,  1.0f } }, // { 1.0f, 0.0f, 0.0f } }, // bottom right
        { { -1.0f, -1.0f } }, // { 0.0f, 1.0f, 0.0f } }, // top left
        { {  1.0f, -1.0f } }, // { 0.0f, 0.0f, 1.0f } }  // top right
    };

    // staging buffer must be host visible and host coherent so it can be copied
    createBuffer(vko, sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingMemory);

    // copy data onto staging buffer
    void *data;
    vkMapMemory(vko->device, stagingMemory, 0, sizeof(vertices), 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(vko->device, stagingMemory);

    // actual vertex buffer
    createBuffer(vko, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    &vko->vertexBuffer, &vko->vertexBufferMemory);

    // copy from staging to test buffer
    copyBuffer(vko, stagingBuffer, vko->vertexBuffer, sizeof(vertices));

    // destroy staging buffer
    vkDestroyBuffer(vko->device, stagingBuffer, NULL);
    vkFreeMemory(vko->device, stagingMemory, NULL);
}

void initializeStorageImages(vk_context *vko) {
    // create a staging image
    VkImage stagingImage;
    VkDeviceMemory stagingMemory;
    createImage(
        vko,
        vko->width,
        vko->height,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        &stagingImage,
        &stagingMemory
    );

    // recall a subresource is just a smallest part of an image we can work with in vulkan
    VkImageSubresource subresource = {0};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.arrayLayer = 0;
    subresource.mipLevel = 0;

    VkSubresourceLayout layout = {0};
    vkGetImageSubresourceLayout(vko->device, stagingImage, &subresource, &layout);

    void *data;
    vkMapMemory(vko->device, stagingMemory, 0, VK_WHOLE_SIZE, 0, &data); // not sure why vk whole size is used here, but seems pretty useful given that idk what the size of the image memory is

    int r = 550;

    for (int y = 2; y < vko->height - 2; y++) {
        char* rowStart = (char*)data + y * layout.rowPitch;

        for (int x = 2; x < vko->width - 2; x++) {
            int dx = x - vko->width / 2;
            int dy = y - vko->height / 2;
            int d2 = dx * dx + dy * dy;

            uint8_t c = (d2 <= r * r) ? 255 : 0;

            if (c == 0) continue;

            uint8_t* pixel = (uint8_t*)rowStart + x * 4;

            // if (y < vko->height/2) {
            //     pixel[0] = x < vko->width/2 ? c : c;
            //     pixel[1] = x < vko->width/2 ? 0 : c;
            //     pixel[2] = x < vko->width/2 ? c : c;
            //     pixel[3] = 255;
            // }
            // else {
            //     pixel[0] = x < vko->width/2 ? c : 0;
            //     pixel[1] = x < vko->width/2 ? c : c;
            //     pixel[2] = x < vko->width/2 ? c : c;
            //     pixel[3] = 255;
            // }
        }
    }

    for (int ping = 0; ping < 2; ping++) {
        VkCommandBuffer cmd = beginSingleTimeCommands(vko);
        copyImage(vko, cmd, VK_FORMAT_R8G8B8A8_UNORM, stagingImage, vko->smokeMapImages[ping], vko->width, vko->height);
        endSingleTimeCommands(vko, cmd);
    }

    vkUnmapMemory(vko->device, stagingMemory);
    vkDestroyImage(vko->device, stagingImage, NULL);
    vkFreeMemory(vko->device, stagingMemory, NULL);

    // next, the velocity buffers
    // velocity x buffer
    createImage(
        vko,
        vko->width + 1,
        vko->height,
        VK_FORMAT_R32_SFLOAT,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        &stagingImage,
        &stagingMemory
    );
    vkGetImageSubresourceLayout(vko->device, stagingImage, &subresource, &layout);
    vkMapMemory(vko->device, stagingMemory, 0, VK_WHOLE_SIZE, 0, &data);

    for (int y = 1; y < vko->height - 1; y++) {
        char* rowStart = (char*)data + y * layout.rowPitch;
        for (int x = 1; x < vko->width; x++) {
            float c = 1000 * (2 * randf() - 1);
            c = 0;
            if (y > 375 && y < 425) {
                // c = 100000;
            }
            memcpy(rowStart + (x * 4), &c, 4);
        }
    }

    for (int ping = 0; ping < 2; ping++) {
        VkCommandBuffer cmd = beginSingleTimeCommands(vko);
        copyImage(vko, cmd, VK_FORMAT_R32_SFLOAT, stagingImage, vko->horizontalVelocitiesImages[ping], vko->width + 1, vko->height);
        endSingleTimeCommands(vko, cmd);
    }

    vkUnmapMemory(vko->device, stagingMemory);
    vkDestroyImage(vko->device, stagingImage, NULL);
    vkFreeMemory(vko->device, stagingMemory, NULL);

    // velocity y buffer
    createImage(
        vko,
        vko->width,
        vko->height + 1,
        VK_FORMAT_R32_SFLOAT,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        &stagingImage,
        &stagingMemory
    );
    vkGetImageSubresourceLayout(vko->device, stagingImage, &subresource, &layout);
    vkMapMemory(vko->device, stagingMemory, 0, VK_WHOLE_SIZE, 0, &data);

    for (int y = 1; y < vko->height; y++) {
        char* rowStart = (char*)data + y * layout.rowPitch;
        for (int x = 1; x < vko->width - 1; x++) {
            float c = 1000 * (2 * randf() - 1);
            c = 0;
            memcpy(rowStart + (x * 4), &c, 4);
        }
    }

    for (int ping = 0; ping < 2; ping++) {
        VkCommandBuffer cmd = beginSingleTimeCommands(vko);
        copyImage(vko, cmd, VK_FORMAT_R32_SFLOAT, stagingImage, vko->verticalVelocitiesImages[ping], vko->width, vko->height + 1);
        endSingleTimeCommands(vko, cmd);
    }

    vkUnmapMemory(vko->device, stagingMemory);
    vkDestroyImage(vko->device, stagingImage, NULL);
    vkFreeMemory(vko->device, stagingMemory, NULL);

    // pressure map buffer
    createImage(
        vko,
        vko->width,
        vko->height,
        VK_FORMAT_R32_SFLOAT,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        &stagingImage,
        &stagingMemory
    );
    vkGetImageSubresourceLayout(vko->device, stagingImage, &subresource, &layout);
    vkMapMemory(vko->device, stagingMemory, 0, VK_WHOLE_SIZE, 0, &data);

    for (int y = 0; y < vko->height; y++) {
        char* rowStart = (char*)data + y * layout.rowPitch;
        for (int x = 0; x < vko->width; x++) {
            float c = 0.0f; // initialize pressure to all 0
            memcpy(rowStart + (x * 4), &c, 4);
        }
    }

    for (int ping = 0; ping < 2; ping++) {
        VkCommandBuffer cmd = beginSingleTimeCommands(vko);
        copyImage(vko, cmd, VK_FORMAT_R32_SFLOAT, stagingImage, vko->pressureMapImages[ping], vko->width, vko->height);
        endSingleTimeCommands(vko, cmd);
    }

    vkUnmapMemory(vko->device, stagingMemory);
    vkDestroyImage(vko->device, stagingImage, NULL);
    vkFreeMemory(vko->device, stagingMemory, NULL);

    // solid map
    createImage(
        vko,
        vko->width,
        vko->height,
        VK_FORMAT_R8_UINT,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        &stagingImage,
        &stagingMemory
    );
    vkGetImageSubresourceLayout(vko->device, stagingImage, &subresource, &layout);
    vkMapMemory(vko->device, stagingMemory, 0, VK_WHOLE_SIZE, 0, &data);

    for (int y = 0; y < vko->height; y++) {
        char* rowStart = (char*)data + y * layout.rowPitch;
        for (int x = 0; x < vko->width; x++) {
            uint8_t c = 0; // for now set everything to be air
            if (y <= 1 || y >= vko->height - 2 || x <= 1 || x >= vko->width - 2) c = 1;

            if ((x - (vko->width/2 - 300)) * (x - (vko->width/2 - 300)) + ((y - vko->height/2)) * (y - vko->height/2) <= 70 * 70) {
                c = 1;
            }

            memcpy(rowStart + (x * 1), &c, 1); // uint8_t is just one byte
        }
    }

    for (int ping = 0; ping < 2; ping++) {
        VkCommandBuffer cmd = beginSingleTimeCommands(vko);
        copyImage(vko, cmd, VK_FORMAT_R32_SFLOAT, stagingImage, vko->solidMapImages[ping], vko->width, vko->height);
        endSingleTimeCommands(vko, cmd);
    }

    vkUnmapMemory(vko->device, stagingMemory);
    vkDestroyImage(vko->device, stagingImage, NULL);
    vkFreeMemory(vko->device, stagingMemory, NULL);
}

void initializeRecorder(vk_context *vko) {
    #ifdef RECORD
    char buffer[256];
    snprintf(buffer, sizeof(buffer), 
        "ffmpeg -f rawvideo "
        "-pix_fmt gray "
        "-s %dx%d "
        "-r 60 "
        "-i - "
        "-c:v libx264 output.mp4",
        vko->width,
        vko->height
    );

    printf("%s\n", buffer);

    vko->ffmpeg = popen(buffer, "w");

    // also initialize staging buffer
    createImage(
        vko,
        vko->width,
        vko->height,
        vko->surfaceFormat.format,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        &vko->stagingImage,
        &vko->stagingMemory
    );
    vkMapMemory(vko->device, vko->stagingMemory, 0, VK_WHOLE_SIZE, 0, &vko->mappedStagingData);

    #endif
}

void cleanupRecorder(vk_context *vko) {
    #ifdef RECORD
    pclose(vko->ffmpeg);
    vkDestroyImage(vko->device, vko->stagingImage, NULL);
    vkFreeMemory(vko->device, vko->stagingMemory, NULL);
    #endif
}

int main() {
    vk_context vko = {0};
    srand(0);

    // GOAL FOR TOMORROW: theres no clear plan man. try to figure out what the issue is with the physics

    initRenderer(&vko);
    initializeVertexBuffer(&vko);
    initializeStorageImages(&vko);
    initializeRecorder(&vko);
    mainLoop(&vko);
    cleanupRenderer(&vko);
    cleanupRecorder(&vko);
    return 0;
}
