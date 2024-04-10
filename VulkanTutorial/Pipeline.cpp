#include "Pipeline.h"
#include <stdexcept>
#include <fstream>
#include <filesystem>

#include "structs.h"


Pipeline::Pipeline(const std::string& vertShaderPath, const std::string& fragShaderPath, bool is3D)
    :m_VerShader{vertShaderPath}, m_FragShader{fragShaderPath}, m_Is3D{is3D}
{
}

void Pipeline::Init(VkDevice logicalDevice, VkExtent2D swapChainExtent, VkDescriptorSetLayout descriptorSetLayout, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples)
{
    auto vertShader = readFile(m_VerShader);
    auto fragShader = readFile(m_FragShader);

    //create module to help transfer code to pipeline
    VkShaderModule vertShaderModule = createShaderModule(logicalDevice, vertShader);
    VkShaderModule fragShaderModule = createShaderModule(logicalDevice, fragShader);

    //PIPELINE INFO
    //Vertex Shader
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    //insert the shader code and the function name that calls it
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    vertShaderStageInfo.pSpecializationInfo = nullptr; // this can specify a constant for in your shader (nullpntr = default)

    //Fragment Shader
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    //array of shaderStagesInfo
    VkPipelineShaderStageCreateInfo vShaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    //Dynamic state
    std::vector<VkDynamicState> vDynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicInfo{};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = static_cast<uint32_t>(vDynamicStates.size());
    dynamicInfo.pDynamicStates = vDynamicStates.data();

    //vertex format info 
    VkVertexInputBindingDescription bindingDescription;
    std::array<VkVertexInputAttributeDescription, 3>attributeDescription;
    if (m_Is3D)
    {
       bindingDescription = Vertex3D::getBindDescription();
       attributeDescription = Vertex3D::getAttributeDescriptions();
    }
    else
    {
        bindingDescription   = Vertex2D::getBindDescription();
        attributeDescription = Vertex2D::getAttributeDescriptions();
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

    //Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    //viewport
    VkViewport viewPort{};
    viewPort.x = 0.0f;
    viewPort.y = 0.0f;
    viewPort.width = static_cast<float>(swapChainExtent.width);
    viewPort.height = static_cast<float>(swapChainExtent.height);
    viewPort.minDepth = 0.0f;
    viewPort.maxDepth = 1.0f;

    //Apply siccor Rect -> made same size as viewport to have full view off image
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    //setting a nonDynamic viewportState ->look at tutorial for dynamic setting
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewPort;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    //Depth
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;


    //Rastereization info
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; //if true : clamps values to the far/near planes when close to
    rasterizer.rasterizerDiscardEnable = VK_FALSE; //dissables output to frameBuffer
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;//optional when disabled
    rasterizer.depthBiasClamp = 0.0f;//optional when disabled
    rasterizer.depthBiasSlopeFactor = 0.0f;//optional when disabled

    //MultiSampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.rasterizationSamples = msaaSamples;
    multisampling.minSampleShading = 0.2f; //min fraction for sample shading; closer to one is smoother    
    multisampling.pSampleMask = nullptr;  // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    //Configuration off attachted framebuffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;      // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;      // Optional

    //Color blending
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional


    VkPushConstantRange pushConstants{};
    pushConstants.offset = 0;
    pushConstants.size = sizeof(glm::mat4);
    pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    //Pipeline Layout ->used for dynamic behaviour like passing the tranform matrix to vertexshader or texture sampler to fragment shader
    VkPipelineLayoutCreateInfo pipelineLayout{};
    pipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayout.setLayoutCount = 1;
    pipelineLayout.pSetLayouts = &descriptorSetLayout;
    pipelineLayout.pushConstantRangeCount = 1;
    pipelineLayout.pPushConstantRanges = &pushConstants;

    if (vkCreatePipelineLayout(logicalDevice, &pipelineLayout, nullptr, &m_PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    //Shader stages
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = vShaderStages;
    //Fixed Function stage
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;//must be specified if the renderpass contains it
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicInfo;
    //layout
    pipelineInfo.layout = m_PipelineLayout;
    //Render pass
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0; //index
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; //optional
    pipelineInfo.basePipelineIndex = -1; //optional

    if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("creation off grapics pipeline failed");
    }

    //destroying of shader after creation off pipline
    vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);

}

void Pipeline::Record(VkCommandBuffer commandBuffer, VkDescriptorSet discriptorSet)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

    vkCmdBindDescriptorSets(commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_PipelineLayout,
        0, 1, &discriptorSet,
        0, nullptr);
}

void Pipeline::Destroy(VkDevice logicalDevice)
{
    vkDestroyPipeline(logicalDevice, m_GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(logicalDevice, m_PipelineLayout, nullptr);
}

VkShaderModule Pipeline::createShaderModule(VkDevice logicalDevice, const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    //stored localy because after the creation off the pipeline we dont need this value anymore
    return shaderModule;
}

std::vector<char> Pipeline::readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("failed to open shader file");
    size_t fileSize{ static_cast<size_t>(file.tellg()) };
    std::vector<char> vBuffer(fileSize);

    file.seekg(0);
    file.read(vBuffer.data(), fileSize);

    file.close();

    return vBuffer;
}