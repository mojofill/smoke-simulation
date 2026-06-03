#include "graphics_pipeline.h"

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
) {
    VkShaderModule vertexShaderModule = load_shader(vko->device, vert_path);
    VkShaderModule fragmentShaderModule = load_shader(vko->device, frag_path);

    // pipeline shader stage setup
    VkPipelineShaderStageCreateInfo vertStage = {0};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertexShaderModule;
    vertStage.pName = "main"; // needs to be main - this is the void main() thing in the shader

    VkPipelineShaderStageCreateInfo fragStage = {0};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.module = fragmentShaderModule;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};

    // we want dynamic states for viewport and scissor. these can change during draw time
    VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {0};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (uint32_t)2;
    dynamicState.pDynamicStates = dynamicStates;

    // HERE is where i put the vertex buffer information. this is important. thus vertex buffers must be made BEFORE graphics pipeline
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptionCount;
    vertexInputInfo.pVertexBindingDescriptions = bindingDescs;
    vertexInputInfo.vertexAttributeDescriptionCount = attrDesciptionCount;
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs;

    // input assembly = describes how to assemble vertices (triangles, lines, points, etc)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1; // dont have to define viewport yet, else would make viewports immutable
    viewportState.scissorCount = 1; // will set up viewport + scissor at draw time, not at immutable pipeline creation time

    // rasterization = creates fragments
    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

    // uncomment this line for polygon mode
    // rasterizer.polygonMode = VK_POLYGON_MODE_LINE;

    rasterizer.lineWidth = 1.0f;
    
    rasterizer.cullMode = cullMode;
    
    // uncommet this line for no culling
    // rasterizer.cullMode = VK_CULL_MODE_NONE;

    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Turn these on if making a 3D game and using depth buffers

    // depth buffering
    VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depthTestEnable;
    depthStencil.depthWriteEnable = depthWriteEnable;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;

    // multisampling (anti aliasing)
    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE; // turned off for now
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // per framebuffer color blending settings
    // color blend = blending the rgba channels
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = // this just defines what colors fragment shader can write to
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE; // turned off for now - reduces complexity

    // global color blending settings
    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // these are where the uniforms go. should turn this into a function
    // pipeline layout = defines how shaders access resources
    // this will come in useful for future experiments
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &vko->descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    // should create an independent layout for each layout, but this works for now
    if (vko->graphicsPipelineLayout == NULL) vkCreatePipelineLayout(vko->device, &pipelineLayoutInfo, NULL, &vko->graphicsPipelineLayout);

    // now finally, put everything together into a graphics pipeline object
    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2; // vertex + fragment
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = vko->graphicsPipelineLayout;
    pipelineInfo.renderPass = vko->renderPass;
    pipelineInfo.subpass = 0; // ??? i think this is an index? this means it targets subpass 0
    // subpasses are only for render passes aye man these will make sense in the future. think subpasses either read or write to attachments in the swapchain

    if (vkCreateGraphicsPipelines(vko->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, destPipeline) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline\n");
        VkResult res = vkCreateGraphicsPipelines(vko->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, destPipeline);
        printf("pipeline result = %d\n", res);
        exit(1);
    }

    // after finished, dont need shader modules anymore
    vkDestroyShaderModule(vko->device, vertexShaderModule, NULL);
    vkDestroyShaderModule(vko->device, fragmentShaderModule, NULL);
}

VkShaderModule load_shader(VkDevice device, const char* path) {
    FILE *f;
    long len;
    char *buffer;

    f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "Failed to create shader from path at %s\n", path);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    assert(len > 0);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(len); // size of char = 1
    assert(buffer != NULL);
    fread(buffer, 1, len, f);
    fclose(f);

    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = len;
    createInfo.pCode = (uint32_t*)buffer; // pointers can be casted to anything, bc at the end of the day its all just bytes. also 32bits = 4 bytes. this type casting just splits the 1 byte char array into 4 byte 32-bit words

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        fprintf(stderr, "Failed to load shader module at path %s\n", path);
        exit(1);
    }

    free(buffer);
    return shaderModule;
}

void createGraphicsPipeline(vk_context *vko) {
    createPipeline(
        vko,
        "./src/spvs/vert.spv",
        "./src/spvs/frag.spv",
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        &vko->bindingDesc,
        1, // binding description count
        vko->attrDescs,
        1, // attribute description count
        VK_FALSE, // depth test enable
        VK_FALSE, // depth write enable
        VK_CULL_MODE_BACK_BIT, // cull mode
        &vko->graphicsPipeline
    );
}
