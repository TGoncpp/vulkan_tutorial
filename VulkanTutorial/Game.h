#pragma once

//#include <vulkan/vulkan.h> been replaced with belowed Define AND include
//GLFW is interface for window Handle
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory> //used for RAI manegment

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <vector>
#include <string>

#include "Structs.h"

//enable validationLayers while on debug mode
const std::vector<const char*> vValidationLayers = { "VK_LAYER_KHRONOS_validation" };
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) 
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        //if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        //    // Message is important enough to show
        //}
        return VK_FALSE;
    }


class Game 
{
public:
    void run();
   
    bool m_FramebufferResiezed{ false };

private:
    //Window variables
    GLFWwindow* m_Window = nullptr;
    const uint32_t m_WindowWidth{ 800 };
    const uint32_t m_WindowHeight{ 600 };
    const std::string m_WindowTitle{ "My Awesome screen" };

    //Vulkan variable
    VkInstance m_Instance;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    VkSurfaceKHR m_Surface;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE; //Implicitly destroyed??
    VkDevice m_LogicalDevice = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;
    const std::vector<const char*> m_vDeviceExtensions = {
                                                        VK_KHR_SWAPCHAIN_EXTENSION_NAME
                                                       };
    VkSwapchainKHR m_SwapChain;
    std::vector <VkImage> m_vSwapChainImages;
    VkFormat m_SwapChainImageFormat;
    VkExtent2D m_SwapChainExtent;
    std::vector<VkImageView> m_vSwapChainImageViews;
    VkRenderPass m_RenderPass;
    VkPipelineLayout m_PipelineLayout;
    VkPipeline m_GraphicsPipeline;
    std::vector<VkFramebuffer> m_vSwapchainFramebuffers;
    VkCommandPool m_CommandPool;
    std::vector<VkCommandBuffer> m_vCommandBuffers;

    std::vector < VkSemaphore> m_vImageAvailableSemaphores;
    std::vector < VkSemaphore> m_vRenderFinishedAvailableSemaphores;
    std::vector < VkFence> m_vInFlightFences;

    const std::vector<Vertex> m_vVertices = {
          {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
          {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
          {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
          {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
           };
    const std::vector<uint16_t> m_vIndices = { //use uint32_t when amount get above 65535
             0, 1, 2, 2, 3, 0
    };
    VkBuffer m_VertexBuffer;
    VkDeviceMemory m_VertexBufferMemory;
    VkBuffer m_IndexBuffer;
    VkDeviceMemory m_IndexBufferMemory;

    //gloabal variables for keeping track off rendering frames and the max off frames to deal with
    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t m_CurrentFrame        = 0;



    //-----------------------------------------------------------
    //Main functions
    void initWindow();
    static void framebufferResizeCallBack(GLFWwindow* window, int width, int height);
    
    //create an instance to create a link between the application and the Vulkan library
    void initVulkan();

    void mainLoop(); 

    void cleanup();

    //Help functions
    //Instance
    void  createInstance();

    //validation layers
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void setupDebugMessenger();
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);
    static void DestroyDebugUtilMessengerEXT(VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);

    //surface creation
    void createSurface();

    //physicalDevice
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device)const;

    //Queue families
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)const;

    //logical Device
    void createLogicalDevice();

    //SWAPCHAIN
    //check if valid extension is available
    bool checkDeviceExtensionSupport(VkPhysicalDevice phDevice)const;
    //fill in property details for swapchain
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice phDevice)const;
    //setting the properties for the swapchain to check if your prefered one is available 
        //->Format (color depth)
    VkSurfaceFormatKHR  chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        //Presentation Modes !!Most IMPORTANT ONE (swapping conditions)
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        //SwapExtent (resolution)
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& availableExtents);
    //creating the swapchain
    void createSwapChain();
    //this is used for when surface become invallid and needs to recalculate
    void recreateSwapchain();
    void cleanupSwapchain();

    //IMAGE VIEW
    void createImageView();

    //PIPELINE
    //-------------------------------
    void createGraphicsPipeline();
    //load the shader
    static std::vector<char> readFile(const std::string& filename);
    //helperModule to pass the filecode to pipeline
    VkShaderModule  createShaderModule(const std::vector<char>& code);
    //RENDER PASS
    void createRenderPass();

    //DRAWING
    //----------------------------------
    void createFramebuffer();
    void createCommandPool();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void drawFrame();

    //SEMAPHORE AND FENCE
    void createSyncObjects();

    //BUFFERS
    void createVertexBuffer();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createIndexBuffer();

    //Abstraction
    void createBuffer(VkDeviceSize bufferSize, 
        VkBufferUsageFlags flags,
        VkMemoryPropertyFlags memryProps, 
        VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


};