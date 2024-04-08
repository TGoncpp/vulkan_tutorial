#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>

class Game;


class ParticleEffect
{
public:
	ParticleEffect(Game* owner) : m_pOwner{ owner } { Init(); };
	void Init();

private:
	static const int m_NumOfParticles{ 100 };
	Game* m_pOwner;
	std::array<VkBuffer, m_NumOfParticles> m_Buffer;
	std::array<VkDeviceMemory, m_NumOfParticles> m_BufferMemory;
};