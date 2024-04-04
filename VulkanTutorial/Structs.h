#pragma once
#include <optional> //c++ wrapper to give the option to a variable to have no value
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <array>
#define GLM_ENABLE_EXPERIMENTAL //for hash mapping
#include <glm/gtx/hash.hpp>

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t>presentFamily;

	bool isComplete()const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails 
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texcoord;

	static VkVertexInputBindingDescription getBindDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding   = 0;
		bindingDescription.stride    = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static const int attributeNum{ 3 };
	static std::array<VkVertexInputAttributeDescription, attributeNum> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, attributeNum> attributeDescriptions{};
		attributeDescriptions[0].binding  = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset   = offsetof(Vertex, pos); //offsetof gives the number off bytes between start off the struct and start off specified member
		
		attributeDescriptions[1].binding  = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset   = offsetof(Vertex, color); 

		attributeDescriptions[2].binding  = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset   = offsetof(Vertex, texcoord);

		return attributeDescriptions;
	}

	bool operator ==(const Vertex& other)const
	{
		return pos == other.pos && color == other.color && texcoord == other.texcoord;
	}

};

//CREATING A HASH FOR THE UNORDERD MAP IN THE LOAD OBJ
namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texcoord) << 1);
		}
	};
}

struct UniformBufferObject
{
	alignas(16) glm::mat4 model; //-> when not using the define off force aligned gentypes
	alignas(16) glm::mat4 view; // But it can break when using nested variables, (like selfmade structs)
	alignas(16) glm::mat4 proj; //Safer to always manually allign

};