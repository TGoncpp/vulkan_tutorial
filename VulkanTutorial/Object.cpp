#include "Object.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h> //obj parser
#include <unordered_map>

#include "Pipeline.h"


void SceneObject::Init(VkPhysicalDevice& physicalDevice, VkDevice& logicDevice, VkCommandPool& commandPool, const int FrmasInFlight, VkQueue& graphicsQueue)
{
    loadModel();
    createVertexBuffer(physicalDevice, logicDevice, commandPool, graphicsQueue);
    createIndexBuffer(physicalDevice, logicDevice, commandPool, graphicsQueue);
    //createCommandBuffers(logicDevice, commandPool, FrmasInFlight);
}

void SceneObject::Record(VkCommandBuffer commandBuffer)
{
    VkBuffer vertexBuffers[] = { m_VertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_vIndices.size()), 1, 0, 0, 0);
}


void SceneObject::Destroy(VkDevice& logicDevice)
{
    vkDestroyBuffer(logicDevice, m_IndexBuffer, nullptr);
    vkFreeMemory(logicDevice, m_IndexBufferMemory, nullptr);
    vkDestroyBuffer(logicDevice, m_VertexBuffer, nullptr);
    vkFreeMemory(logicDevice, m_VertexBufferMemory, nullptr);
   
}




void SceneObject::loadModel()
{
    if (m_ModelPath == "")return;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warningMessage, errorMessege;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warningMessage, &errorMessege, m_ModelPath.c_str()))
    {
        throw std::runtime_error{ "failed to load in obj" + warningMessage + errorMessege };
    }

    std::unordered_map<Vertex3D, uint32_t> mUniqueVertexes{};
    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex3D vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            if(m_IsCollored)
            {
                vertex.texcoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                   1.0f - attrib.texcoords[2 * index.texcoord_index + 1] //1.0- is because we flip the y-axis in our settings
                };
            }
            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (mUniqueVertexes.count(vertex) == 0)
            {
                mUniqueVertexes[vertex] = static_cast<uint32_t>(m_vVertices3D.size());
                m_vVertices3D.push_back(vertex);
            }

            m_vIndices.push_back(mUniqueVertexes[vertex]);
        }
    }
}

//void SceneObject::createCommandBuffers(VkDevice& logicDevice, VkCommandPool& commandPool, const int FrmasInFlight)
//{
//    m_vCommandBuffers.resize(FrmasInFlight);
//
//    VkCommandBufferAllocateInfo allocInfo{};
//    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//    allocInfo.commandPool        = commandPool;
//    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//    allocInfo.commandBufferCount = static_cast<uint32_t>(m_vCommandBuffers.size());
//
//    if (vkAllocateCommandBuffers(logicDevice, &allocInfo, m_vCommandBuffers.data()) != VK_SUCCESS)
//    {
//        throw std::runtime_error("Allocation off command buffer failed");
//    }
//
//}

void SceneObject::createVertexBuffer(VkPhysicalDevice& physicalDevice, VkDevice& logicDevice, VkCommandPool& commandPool, VkQueue& graphicsQueue)
{
    VkDeviceSize bufferSize = m_Is3D ? sizeof(m_vVertices3D[0]) * m_vVertices3D.size() : sizeof(m_vVertices2D[0]) * m_vVertices2D.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(physicalDevice, logicDevice, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;
    void* srcData{ m_Is3D ? (void*)m_vVertices3D.data() : (void*)m_vVertices2D.data() };
    vkMapMemory(logicDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, srcData, static_cast<size_t>(bufferSize));
    vkUnmapMemory(logicDevice, stagingBufferMemory);

    //Binding the vertex buffer will happen in the recordCommandBuffer()

    createBuffer(physicalDevice, logicDevice, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_VertexBuffer, m_VertexBufferMemory);

    copyBuffer(logicDevice, commandPool, graphicsQueue, stagingBuffer, m_VertexBuffer, bufferSize);

    //Clean up temp buffer/ bufferMemory
    vkDestroyBuffer(logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(logicDevice, stagingBufferMemory, nullptr);

}

void SceneObject::createIndexBuffer(VkPhysicalDevice& physicalDevice, VkDevice& logicDevice, VkCommandPool& commandPool, VkQueue& graphicsQueue)
{
    VkDeviceSize bufferSize = sizeof(m_vIndices[0]) * m_vIndices.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory staginBufferMemory;
    createBuffer(physicalDevice, logicDevice, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        stagingBuffer, staginBufferMemory);

    void* data;
    vkMapMemory(logicDevice, staginBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_vIndices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(logicDevice, staginBufferMemory);

    createBuffer(physicalDevice, logicDevice, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_IndexBuffer, m_IndexBufferMemory);

    copyBuffer(logicDevice, commandPool, graphicsQueue ,stagingBuffer, m_IndexBuffer, bufferSize);

    vkDestroyBuffer(logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(logicDevice, staginBufferMemory, nullptr);
}







VkCommandBuffer SceneObject::BeginSingleCommand(VkDevice& logicDevice, VkCommandPool& commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(logicDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void SceneObject::EndSingleCommand(VkDevice& logicDevice, VkCommandPool& commandPool, VkQueue& graphicsQueue, VkCommandBuffer& commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(logicDevice, commandPool, 1, &commandBuffer);

}

void SceneObject::copyBuffer(VkDevice& logicDevice, VkCommandPool& commandPool, VkQueue& graphicsQueue, VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize& size)
{
    VkCommandBuffer commandBuffer = BeginSingleCommand(logicDevice, commandPool);

    //Copy
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    //Stop recording
    EndSingleCommand(logicDevice, commandPool, graphicsQueue, commandBuffer);


}

void SceneObject::createBuffer(
    VkPhysicalDevice& physicalDevice, VkDevice& logicDevice,
    VkDeviceSize& bufferSize, VkBufferUsageFlags flags, 
    VkMemoryPropertyFlags memryProps, 
    VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = flags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //only used by graphicsqueue so exlusive is enough

    if (vkCreateBuffer(logicDevice, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS)
        throw std::runtime_error("creation off vertex buffer failed");

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(logicDevice, vertexBuffer, &memRequirements);

    //Memory allocation
    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize  = memRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, memryProps);

    if (vkAllocateMemory(logicDevice, &allocateInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate memory for vertex buffer");

    //Filling the Vertex buffer
    vkBindBufferMemory(logicDevice, vertexBuffer, vertexBufferMemory, 0);
}

uint32_t SceneObject::findMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    for (uint32_t i{}; i < memProps.memoryTypeCount; ++i)
    {
        if (typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type");
}
