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

//Abstract classes
#include "camera.h"
#include "Pipeline.h"
#include "Object.h"


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
    VkDescriptorSetLayout m_DescriptorSetLayout;
   
    std::vector<VkFramebuffer> m_vSwapchainFramebuffers;


   VkCommandPool m_CommandPool;
  
    //const std::string m_ModelPath{ "models/room.obj" };
    const std::string m_TexturePath{ "textures/viking_room.png" };


    std::vector < VkSemaphore> m_vImageAvailableSemaphores;
    std::vector < VkSemaphore> m_vRenderFinishedAvailableSemaphores;
    std::vector < VkFence> m_vInFlightFences;

    std::vector<VkCommandBuffer> m_vCommandBuffers;
    std::vector<VkCommandBuffer> m_vCommandBuffers2D;
    std::vector<VkBuffer> m_vUniformBuffers;
    std::vector<VkDeviceMemory> m_vUniformBuffersMemory;
    std::vector<void*> m_vUniformBuffersMapped;
    VkDescriptorPool m_DescriptorPool;
    std::vector<VkDescriptorSet> m_vDescriptorSets;

    uint32_t m_MipLvl;
    VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;

    VkImage m_TextureImage;
    VkDeviceMemory m_TextureImageMemory;
    VkImageView m_TextureImageView;
    VkSampler m_TextureSampler;

    VkImage m_DepthImage;
    VkDeviceMemory m_DepthImageMemory;
    VkImageView m_DepthImageView;

    VkImage m_ColorImage;
    VkDeviceMemory m_ColorImageMemory;
    VkImageView m_ColorImageView;

    std::unique_ptr<Camera> m_pCamera;
    std::unique_ptr<Pipeline> m_p3DPipeline;
    std::unique_ptr<SceneObject> m_p3DObject;
    std::unique_ptr<SceneObject> m_p3DObject2;

    std::vector<Vertex2D> m_vVertex2D
    {
       {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{1.0, 0}},
       {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f},{1.0, 0} },
       {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, { 1.0, 0 }},
       {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f},{ 1.0, 0 }}
    };
    std::vector<uint32_t> m_vIndices
    {
        0, 1, 2, 0, 2, 3
    };
    std::unique_ptr<Pipeline> m_p2DPipeline;
    std::unique_ptr<SceneObject> m_p2DObject;


    //gloabal variables for keeping track off rendering frames and the max off frames to deal with
    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t m_CurrentFrame        = 0;


    float m_RotationSpeed{ 50.f };


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
    void createImageViews();

   // //RENDER PASS
    void createRenderPass();
    void beginRenderPass(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    //DRAWING
    //----------------------------------
    void createFramebuffer();
    void createCommandPool();
    void createCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, Pipeline* pipeline, SceneObject* object);
    void drawFrame();

    //SEMAPHORE AND FENCE
    void createSyncObjects();

    //BUFFERS
    //void createVertexBuffer();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    //void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();

    public:
    //Abstraction
    void createBuffer(VkDeviceSize bufferSize, 
        VkBufferUsageFlags flags,
        VkMemoryPropertyFlags memryProps, 
        VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    private:
    //Descripters
    void createDescriptorSetLayout();


    //UPDATE
    void updateUniformBuffer(uint32_t currentImage);

    //TEXTURES
    void createTextureImage();
    void createImage(uint32_t width, uint32_t height, uint32_t mipLvls, VkSampleCountFlagBits numSamples,
                     VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage& image, VkDeviceMemory& imageMemory);
    void createTextureImageView();
    void createTextureSampler();
    void createDepthResources();//for all depth resources
    void createColorResources();// for all multisampling resources

    //helper functions
    VkCommandBuffer beginSingleCommands();
    void endSingleCommands(VkCommandBuffer commandBuffer);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void transitionImageLayout(VkImage image, VkFormat format, 
                                VkImageLayout oldLayout, VkImageLayout newLayout,
                                uint32_t mipLvls);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLvls);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);
    void generateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    VkSampleCountFlagBits getMaxUsableSampleCount();

};