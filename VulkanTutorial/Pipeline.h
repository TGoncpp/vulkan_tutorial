#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vector>


class Pipeline 
{
public:
	Pipeline(const std::string& vertShaderPath, const std::string& fragShaderPath, bool is3D);
	~Pipeline() = default;
	void Init(VkDevice logicalDevice, VkExtent2D swapChainExtent, VkDescriptorSetLayout descriptorSetLayout, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples);
	void Record(VkCommandBuffer commandBuffer, VkDescriptorSet discriptorSet);
	void Destroy(VkDevice logicalDevice);

	VkPipelineLayout GetPipelineLayout()const { return m_PipelineLayout; };
private:
	std::string m_VerShader;
	std::string m_FragShader;
	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;
	bool m_Is3D{ true };


	static std::vector<char> readFile(const std::string& filename);
	VkShaderModule  createShaderModule(VkDevice logicalDevice, const std::vector<char>& code);
};