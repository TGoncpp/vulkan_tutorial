#include "computeShader.h"
#include "Game.h"

void ParticleEffect::Init()
{
	for (int i{}; i < m_NumOfParticles; ++i)
	{
		VkDeviceSize bufferSize{};
		m_pOwner->createBuffer(bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_Buffer[i], m_BufferMemory[i]);
	}
}
