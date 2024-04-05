#include "Game.h"
#include <set>
#include<algorithm>     // for clamp
#include <limits>       //for numeric_limits
//#include <cstdint>      // for uint32_t
#define STB_IMAGE_IMPLEMENTATION // somehow gives LINK errors instead off solving them
#define GLM_FORCE_DEPTH_ZERO_TO_ONE //turns the default value range from -1->1 to 0->1
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h> //obj parser
#include <unordered_map>

#include <fstream>
#include <filesystem>

void Game::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

//Main functions
//------------------------------------------------------

void Game::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //Tell GLFW not to create a openGl context
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // Tell GLFW not to bother with resizing the window
    m_Window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, m_WindowTitle.c_str(), nullptr, nullptr);

    //set user pointer off aplication in window
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallBack);
}

void Game::framebufferResizeCallBack(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));

    app->m_FramebufferResiezed = true;
}

void Game::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();

    createCommandPool();
    createDepthResources();
    createFramebuffer();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();

    createCommandBuffers();
    createSyncObjects();
}

void Game::mainLoop()
{
    while (!glfwWindowShouldClose(m_Window))
    {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(m_LogicalDevice);
}

void Game::cleanup()
{
    cleanupSwapchain();
    vkDestroySampler(m_LogicalDevice, m_TextureSampler, nullptr);
    vkDestroyImageView(m_LogicalDevice, m_TextureImageView, nullptr);
    vkDestroyImage(m_LogicalDevice, m_TextureImage, nullptr);
    vkFreeMemory(m_LogicalDevice, m_TextureImageMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(m_LogicalDevice, m_vUniformBuffers[i], nullptr);
        vkFreeMemory(m_LogicalDevice, m_vUniformBuffersMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_LogicalDevice, m_DescriptorSetLayout, nullptr);
    vkDestroyBuffer(m_LogicalDevice, m_IndexBuffer, nullptr);
    vkFreeMemory(m_LogicalDevice, m_IndexBufferMemory, nullptr);
    vkDestroyBuffer(m_LogicalDevice, m_VertexBuffer, nullptr);
    vkFreeMemory(m_LogicalDevice, m_VertexBufferMemory, nullptr);
    vkDestroyPipeline(m_LogicalDevice, m_GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
    vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, nullptr);
    for (size_t i{}; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(m_LogicalDevice, m_vImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_LogicalDevice, m_vRenderFinishedAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_LogicalDevice, m_vInFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);
    vkDestroyDevice(m_LogicalDevice, nullptr);
    if (enableValidationLayers)
        DestroyDebugUtilMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}


//HelpFunctions
//----------------------------------------------------------

void Game::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not available");
    }

    //optional info
    VkApplicationInfo appInfo{};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName    = "Hello Game";
    appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName         = "No engine";
    appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion          = VK_API_VERSION_1_0;

    //NESSECARY info -> nessecary for selecting the global extensions and validationLayers
    VkInstanceCreateInfo createInfo{};
    createInfo.sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo           = &appInfo;
    //global extensions
    uint32_t glfwExtentionCount          = 0;
    const char** glfwExtensions          = glfwGetRequiredInstanceExtensions(&glfwExtentionCount);
    //validationLayers
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(vValidationLayers.size());
        createInfo.ppEnabledLayerNames = vValidationLayers.data();
    }
    else
         createInfo.enabledLayerCount         = 0;

    //For avoiding the VK_ERROR_INCOMPATIBLE_DRIVER (can happen on mac i guess)
    //std::vector<const char*> requiredExtensions;
    //for (uint32_t i = 0; i < glfwExtentionCount; ++i)
    //{
    //    requiredExtensions.emplace_back(glfwExtensions[i]);
    //}
    //requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    //createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    //createInfo.ppEnabledExtensionNames = requiredExtensions.data();
    //createInfo.enabledExtensionCount = (uint32_t)requiredExtensions.size();

    auto extensions = getRequiredExtensions();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount   = (uint32_t)extensions.size();

    //create instance and throw if failed
    if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
        throw std::runtime_error("failed to create instance");

}

bool Game::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);           //WHAT THE HELLLLLLL
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : vValidationLayers) 
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) 
        {
            if (strcmp(layerName, layerProperties.layerName) == 0) 
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) 
            return false;
       
    }

    return true;
}

std::vector<const char*> Game::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void Game::setupDebugMessenger()
{
    if (!enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional

    if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
        throw std::runtime_error("failed to set up debug message");
}

VkResult Game::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugUtilsMessengerEXT * pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Game::DestroyDebugUtilMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void Game::createSurface()
{
    if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface");

}

void Game::pickPhysicalDevice()
{
    uint32_t physDevCount;
    vkEnumeratePhysicalDevices(m_Instance, &physDevCount, nullptr);
    if (physDevCount == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support");

    std::vector<VkPhysicalDevice> devices(physDevCount);
    vkEnumeratePhysicalDevices(m_Instance, &physDevCount, devices.data());

    for (const auto& physDev : devices)
    {
        if (isDeviceSuitable(physDev))
        {
            m_PhysicalDevice = physDev;
            m_MsaaSamples    = getMaxUsableSampleCount();
            break;
        }
    }
    if (m_PhysicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("failed to find a suitable GPU device");

}

bool Game::isDeviceSuitable(VkPhysicalDevice device) const
{
    ////This just checks for a basic suitable device
    ////this can be adjusted by adding score for multiple types depending on what you need
    //VkPhysicalDeviceProperties deviceProp;
    //vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProp);

    //return deviceProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
    //    deviceFeats.geometryShader;


    QueueFamilyIndices indices = findQueueFamilies(device);            //Find vallid queueFamilies for graphics and presentation

    bool extensionsSupported = checkDeviceExtensionSupport(device);     //check system for available swapchain support

    bool swapChainAdequate = false;
    if (extensionsSupported)                                            //Check if swapchain is compatible
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    //VkPhysicalDeviceFeatures deviceFeats;                            //-> for optional render features like for VR
    //vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &deviceFeats);

    return indices.isComplete() && extensionsSupported && swapChainAdequate /*&& bool(deviceFeats.samplerAnisotropy)*/;
}

QueueFamilyIndices Game::findQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> vQueueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, vQueueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : vQueueFamilies) 
    {
        //Check if supports grapics rendering
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
        {
            indices.graphicsFamily = i;
            
            //checks if support surface rendering
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport); //putted in same queuefamily, not necceary
            if (presentSupport)                                                          //Rest off code is set so it can handle more queues
               indices.presentFamily = i;
        }       

        if (indices.isComplete())
            break;
        i++;
    }

    return indices;
}

void Game::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);
    std::vector<VkDeviceQueueCreateInfo> vQueueCreateInfos;
    std::set<uint32_t>sUniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : sUniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType             = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex  = queueFamily;
        queueCreateInfo.queueCount        = 1;
        queueCreateInfo.pQueuePriorities  = &queuePriority; //IMPORTANT used for scheduling other queue commands [0-1]
        vQueueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{}; //default for now, is for when using more cool vulkan stuff
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos       = vQueueCreateInfos.data();
    deviceInfo.queueCreateInfoCount    = static_cast<uint32_t>(vQueueCreateInfos.size());
    deviceInfo.pEnabledFeatures        = &deviceFeatures;

    deviceInfo.enabledExtensionCount   = static_cast<uint32_t>(m_vDeviceExtensions.size()); //extension for swapchain
    deviceInfo.ppEnabledExtensionNames = m_vDeviceExtensions.data();

    //Validation layers
    if (enableValidationLayers) {
        deviceInfo.enabledLayerCount   = static_cast<uint32_t>(vValidationLayers.size());
        deviceInfo.ppEnabledLayerNames = vValidationLayers.data();
    }
    else 
        deviceInfo.enabledLayerCount = 0;

    if (vkCreateDevice(m_PhysicalDevice, &deviceInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
        throw std::runtime_error("creating logical device failed");

    vkGetDeviceQueue(m_LogicalDevice, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_LogicalDevice, indices.presentFamily.value(), 0, &m_PresentQueue);
}

bool Game::checkDeviceExtensionSupport(VkPhysicalDevice phDevice)const
{
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(phDevice, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(phDevice, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(m_vDeviceExtensions.begin(), m_vDeviceExtensions.end());

        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
}

SwapChainSupportDetails Game::querySwapChainSupport(VkPhysicalDevice phDevice)const
{
    SwapChainSupportDetails details;

    //Capabilitie
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phDevice, m_Surface, &details.capabilities);

    //Formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phDevice, m_Surface, &formatCount, nullptr);
    if (formatCount > 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(phDevice, m_Surface, &formatCount, details.formats.data());
    }

    //presentation modes
    uint32_t presentModesCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phDevice, m_Surface, &presentModesCount, nullptr);
    if (presentModesCount > 0)
    {
        details.presentModes.resize(presentModesCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(phDevice, m_Surface, &presentModesCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Game::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) 
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR Game::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    //INFO OF THE POSSIBLE MODES
    //VK_PRESENT_MODE_IMMEDIATE_KHR : presented imediatly, but may cause tearing
    //VK_PRESENT_MODE_FIFO_KHR : images are queued and get swapped at the time when display gets refreshed (= vertical blank!)
    //VK_PRESENT_MODE_FIFO_RELAXED_KHR : same as above, but when queue is empty, it would immediatly present next images at vertical blank(may cause tearing)
    //VK_PREENT_MODE_MAILBOX_KHR : same as second, but here he will renew the queued image when queue is full instead off waiting -> triple buffering
    //only second one is guaranteed to be available

    for (const VkPresentModeKHR& mode : availablePresentModes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Game::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width{}, height{};
        glfwGetFramebufferSize(m_Window, &width, &height);

        VkExtent2D actualExtent = { static_cast<uint32_t>(width),  static_cast<uint32_t>(height) };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);


        return actualExtent;
    }
}

void Game::createSwapChain()
{
    SwapChainSupportDetails swapChainSupDetails = querySwapChainSupport(m_PhysicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupDetails.formats);
    VkPresentModeKHR presentMode     = chooseSwapPresentMode(swapChainSupDetails.presentModes);
    VkExtent2D extent                = chooseSwapExtent(swapChainSupDetails.capabilities);

    uint32_t imageCount = swapChainSupDetails.capabilities.minImageCount + 1; // +1 to avoid having to wait
    if (swapChainSupDetails.capabilities.maxImageCount > 0 && imageCount > swapChainSupDetails.capabilities.maxImageCount) // >0 means there is no max
        imageCount = swapChainSupDetails.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageArrayLayers = 1;
    createInfo.imageExtent = extent;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);
    uint32_t queueFamilyInds[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    if (indices.graphicsFamily != indices.presentFamily)                //Mostly the queuefamily will be same for graphics and present
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT; // images is shared between families
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyInds;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;        // image is exlusive to 1 family
        createInfo.queueFamilyIndexCount = 0;     //optional
        createInfo.pQueueFamilyIndices = nullptr; //optional
    }

    createInfo.preTransform   = swapChainSupDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;

    if (vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swapchain");
    }

    vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &imageCount, nullptr);
    m_vSwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &imageCount, m_vSwapChainImages.data());

    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent      = extent;
}

void Game::recreateSwapchain()
{
    //For minimizing-> wait for as long as window is minimized 
    int width{}, height{};
    glfwGetFramebufferSize(m_Window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_Window, &width, &height);
        glfwWaitEvents();
    }

    //for resizing
    //first wait for resources to be available
    vkDeviceWaitIdle(m_LogicalDevice);

    cleanupSwapchain();

    createSwapChain();
    createImageViews();
    createDepthResources();
    createFramebuffer();
}

void Game::cleanupSwapchain()
{
    vkDestroyImageView(m_LogicalDevice, m_DepthImageView, nullptr);
    vkDestroyImage(m_LogicalDevice, m_DepthImage, nullptr);
    vkFreeMemory(m_LogicalDevice, m_DepthImageMemory, nullptr);

    for (auto& buffer : m_vSwapchainFramebuffers)
    {
        vkDestroyFramebuffer(m_LogicalDevice, buffer, nullptr);
        m_vSwapchainFramebuffers.clear();
    }
    for (auto imageView : m_vSwapChainImageViews) {
        vkDestroyImageView(m_LogicalDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(m_LogicalDevice, m_SwapChain, nullptr);

}

void Game::createImageViews()
{
    m_vSwapChainImageViews.resize(m_vSwapChainImages.size());

    for (size_t i = 0; i < m_vSwapChainImages.size(); i++) 
    {
        m_vSwapChainImageViews[i] = createImageView(m_vSwapChainImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void Game::createGraphicsPipeline()
{
    auto vertShader = readFile("shader/vert.spv");
    auto fragShader = readFile("shader/frag.spv");

    //create module to help transfer code to pipeline
    VkShaderModule vertShaderModule =  createShaderModule(vertShader);
    VkShaderModule fragShaderModule =  createShaderModule(fragShader);
    
    //PIPELINE INFO
    //Vertex Shader
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    //insert the shader code and the function name that calls it
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";
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
    dynamicInfo.sType                    = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount        = static_cast<uint32_t>(vDynamicStates.size());
    dynamicInfo.pDynamicStates           = vDynamicStates.data();

    //vertex format info 
    auto bindingDescription            = Vertex::getBindDescription();
    auto attributeDescription          = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescription.data();

    //Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType                         = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology                      = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable        = VK_FALSE;

    //viewport
    VkViewport viewPort{};
    viewPort.x            = 0.0f;
    viewPort.y            = 0.0f;
    viewPort.width        = static_cast<float>(m_SwapChainExtent.width);
    viewPort.height       = static_cast<float>(m_SwapChainExtent.height);
    viewPort.minDepth     = 0.0f;
    viewPort.maxDepth     = 1.0f;

    //Apply siccor Rect -> made same size as viewport to have full view off image
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_SwapChainExtent;

    //setting a nonDynamic viewportState ->look at tutorial for dynamic setting
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewPort;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    //Depth
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType             = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable   = VK_TRUE;
    depthStencil.depthWriteEnable  = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;


    //Rastereization info
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE; //if true : clamps values to the far/near planes when close to
    rasterizer.rasterizerDiscardEnable = VK_FALSE; //dissables output to frameBuffer
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;//optional when disabled
    rasterizer.depthBiasClamp          = 0.0f;//optional when disabled
    rasterizer.depthBiasSlopeFactor    = 0.0f;//optional when disabled

    //MultiSampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading      = 1.0f;     // Optional
    multisampling.pSampleMask           = nullptr;  // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable      = VK_FALSE; // Optional

    //Configuration off attachted framebuffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;      // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;      // Optional

    //Color blending
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable       = VK_FALSE;
    colorBlending.logicOp             = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount     = 1;
    colorBlending.pAttachments        = &colorBlendAttachment;
    colorBlending.blendConstants[0]   = 0.0f; // Optional
    colorBlending.blendConstants[1]   = 0.0f; // Optional
    colorBlending.blendConstants[2]   = 0.0f; // Optional
    colorBlending.blendConstants[3]   = 0.0f; // Optional

    //Pipeline Layout ->used for dynamic behaviour like passing the tranform matrix to vertexshader or texture sampler to fragment shader
    VkPipelineLayoutCreateInfo pipelineLayout{};
    pipelineLayout.sType                   = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayout.setLayoutCount          = 1;
    pipelineLayout.pSetLayouts             = &m_DescriptorSetLayout;
    pipelineLayout.pushConstantRangeCount  = 0;//optional
    pipelineLayout.pPushConstantRanges     = nullptr;//optional

    if (vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayout, nullptr, &m_PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    //Shader stages
    pipelineInfo.stageCount            = 2;
    pipelineInfo.pStages               = vShaderStages;
    //Fixed Function stage
    pipelineInfo.pVertexInputState     = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState   = &inputAssemblyInfo;
    pipelineInfo.pViewportState        = &viewportState;
    pipelineInfo.pRasterizationState   = &rasterizer;
    pipelineInfo.pMultisampleState     = &multisampling;
    pipelineInfo.pDepthStencilState    = &depthStencil;//must be specified if the renderpass contains it
    pipelineInfo.pColorBlendState      = &colorBlending;
    pipelineInfo.pDynamicState         = &dynamicInfo;
    //layout
    pipelineInfo.layout                = m_PipelineLayout;
    //Render pass
    pipelineInfo.renderPass            = m_RenderPass;
    pipelineInfo.subpass               = 0; //index
    pipelineInfo.basePipelineHandle    = VK_NULL_HANDLE; //optional
    pipelineInfo.basePipelineIndex     = -1; //optional

    if (vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("creation off grapics pipeline failed");
    }

    //destroying of shader after creation off pipline
    vkDestroyShaderModule(m_LogicalDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_LogicalDevice, vertShaderModule, nullptr);

}

std::vector<char> Game::readFile(const std::string& filename)
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

VkShaderModule  Game::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType      = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize   = code.size();
    createInfo.pCode      = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    //stored localy because after the creation off the pipeline we dont need this value anymore
    return shaderModule;
}

void Game::createRenderPass()
{
    //DISCRIPTIONS
    VkAttachmentDescription collorAttachment{};
    collorAttachment.format = m_SwapChainImageFormat;
    collorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    //color and depth
    collorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //start with black background
    collorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //stencil data
    collorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    collorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    collorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;    //layout at start off creting pass
    collorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;//How th renderpass should look like at the end
    
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    //color and depth
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //start with black background
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //stencil data
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;    //layout at start off creting pass
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;//How th renderpass should look like at the end

    //REFRENCES
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //PASS IT
    VkSubpassDescription subPass{};
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPass.colorAttachmentCount = 1;
    subPass.pColorAttachments = &colorAttachmentRef;
    subPass.pDepthStencilAttachment = &depthAttachmentRef;


    //Creating the RENDERPASS
    std::array<VkAttachmentDescription, 2> attachments = { collorAttachment, depthAttachment };
    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subPass;

    //added for improving synchronic behaviour
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    //assign the stage where we have to wait for
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    //the opperation that has to wait
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_LogicalDevice, &createInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Creating renderPass failed");
    }
}

void Game::createFramebuffer()
{
    m_vSwapchainFramebuffers.resize(m_vSwapChainImageViews.size());
    for (size_t i{0}; i <  m_vSwapchainFramebuffers.size(); ++i)
    {
        std::array<VkImageView,2> arrAttachments = { m_vSwapChainImageViews[i], m_DepthImageView };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType            = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass       = m_RenderPass;
        framebufferInfo.attachmentCount  = static_cast<uint32_t>(arrAttachments.size());
        framebufferInfo.pAttachments     = arrAttachments.data();
        framebufferInfo.width            = m_SwapChainExtent.width;
        framebufferInfo.height           = m_SwapChainExtent.height;
        framebufferInfo.layers           = 1;

        if (vkCreateFramebuffer(m_LogicalDevice, &framebufferInfo, nullptr, &m_vSwapchainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("creation off framebuffer %i failed"), i;
        }

    }
}

void Game::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_PhysicalDevice);

    VkCommandPoolCreateInfo CommandPoolInfo{};
    CommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    CommandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(m_LogicalDevice, &CommandPoolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Creation off commandPool failed");
    }
}

void Game::createCommandBuffers()
{
    m_vCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_vCommandBuffers.size());

    if (vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, m_vCommandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Allocation off command buffer failed");
    }

}

void Game::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pInheritanceInfo = nullptr; //optional
    beginInfo.flags            = 0; //optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to bgin recording command buffer");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = m_RenderPass;
    renderPassInfo.framebuffer       = m_vSwapchainFramebuffers[imageIndex];
    //renderArea defines where the shader load and store happens
    renderPassInfo.renderArea.offset = { 0,0 };
    renderPassInfo.renderArea.extent = m_SwapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color             = {{0.0f, 0.0f, 0.0f, 1.0f}};
    //clearValues[1].depthStencil      = {m_FarPlane, 0};//should be farplane
    clearValues[1].depthStencil      = {1.0f, 0};
    renderPassInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues      = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_SwapChainExtent.width);
    viewport.height = static_cast<float>(m_SwapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = m_SwapChainExtent;
    scissor.offset = { 0,0 };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    ////DRAW some STUFFFF......
    //vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    
    //Binding off vertexbuffer
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

    VkBuffer vertexBuffers[] = { m_VertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

    //bind descriptors to the current frame
    vkCmdBindDescriptorSets(commandBuffer,
                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                             m_PipelineLayout,
                             0, 1, &m_vDescriptorSets[m_CurrentFrame],
                             0, nullptr);


    //vkCmdDraw(commandBuffer, static_cast<uint32_t>(m_vVertices.size()), 1, 0, 0);   -> When not using an index buffer
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_vIndices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer");
    }


}

void Game::drawFrame()
{

    //1. wait for previous frame
    vkWaitForFences(m_LogicalDevice, 1, &m_vInFlightFences[m_CurrentFrame], VK_TRUE, UINT32_MAX);

    //2.Aquire an image from the swap chain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_LogicalDevice, m_SwapChain, UINT64_MAX, m_vImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) 
    {
        recreateSwapchain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("failed to aquire swapchain image during drawframe");

    vkResetFences(m_LogicalDevice, 1, &m_vInFlightFences[m_CurrentFrame]);

    //3.Recording the command buffer
    vkResetCommandBuffer(m_vCommandBuffers[m_CurrentFrame], 0);
    recordCommandBuffer(m_vCommandBuffers[m_CurrentFrame], imageIndex);

    //3.5 Upadate transformation on image
    updateUniformBuffer(m_CurrentFrame); //-> update the descriptor

    //4. Submitting the command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]      = { m_vImageAvailableSemaphores[m_CurrentFrame] };
    VkPipelineStageFlags waitstages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount     = 1;
    submitInfo.pWaitSemaphores        = waitSemaphores;
    submitInfo.pWaitDstStageMask      = waitstages;

    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &m_vCommandBuffers[m_CurrentFrame];

    VkSemaphore signalSemaphore[]   = { m_vRenderFinishedAvailableSemaphores[m_CurrentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphore;

    if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_vInFlightFences[m_CurrentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit the command buffer");
    }

    //5.Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphore;

    VkSwapchainKHR swapChains[] = { m_SwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; //optional

    result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResiezed)
    {
        m_FramebufferResiezed = false;
        recreateSwapchain();
        //return;
    }
    else if (result != VK_SUCCESS)
        throw std::runtime_error("failed to present swapchain image during drawframe");

    m_CurrentFrame = (m_CurrentFrame++) % MAX_FRAMES_IN_FLIGHT;
}

void Game::createSyncObjects()
{
    m_vInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_vImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_vRenderFinishedAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore{};
    semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence{};
    fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT; //this puts the value to true at the start, if we dont do this. the draw function will indefinitly wait for its signal
    
    for (int i{}; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (vkCreateSemaphore(m_LogicalDevice, &semaphore, nullptr, &m_vImageAvailableSemaphores[i]) != VK_SUCCESS
            || vkCreateSemaphore(m_LogicalDevice, &semaphore, nullptr, &m_vRenderFinishedAvailableSemaphores[i]) != VK_SUCCESS
            || vkCreateFence(m_LogicalDevice, &fence, nullptr, &m_vInFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronological helpers");
        }
    }
}

void Game::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(m_vVertices[0]) * m_vVertices.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_vVertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_LogicalDevice, stagingBufferMemory);

    //Binding the vertex buffer will happen in the recordCommandBuffer()

    createBuffer(bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_VertexBuffer, m_VertexBufferMemory);

    copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    //Clean up temp buffer/ bufferMemory
    vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_LogicalDevice, stagingBufferMemory, nullptr);


}

uint32_t Game::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProps);

    for (uint32_t i{}; i < memProps.memoryTypeCount; ++i)
    {
        if (typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type");
}

void Game::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(m_vIndices[0]) * m_vIndices.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory staginBufferMemory;
    createBuffer(bufferSize, 
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
                stagingBuffer, staginBufferMemory);

    void* data;
    vkMapMemory(m_LogicalDevice, staginBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_vIndices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_LogicalDevice, staginBufferMemory);

    createBuffer(bufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_IndexBuffer, m_IndexBufferMemory);

    copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

    vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_LogicalDevice, staginBufferMemory, nullptr);

}

void Game::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    m_vUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_vUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    m_vUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    // no need to assign a staging buffer since we plan to update this every frame, which would cause it to be slower
    for (size_t i{}; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        createBuffer(bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_vUniformBuffers[i], m_vUniformBuffersMemory[i]);
        vkMapMemory(m_LogicalDevice, m_vUniformBuffersMemory[i], 0, bufferSize, 0, &m_vUniformBuffersMapped[i]);
        //Will be mapped until end off aplication -> called: Persistent Mapping
        //Maps only at creation instead off every frame
    }


}

void Game::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
 
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType           = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount   = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes      = poolSizes.data();
    poolInfo.maxSets         = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolInfo.flags           = 0;

    if (vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error{ "failed to create decriptor pool" };
    }

}

void Game::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_DescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts        = layouts.data();

    m_vDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, m_vDescriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error{ "failed to allocate discriptorSets" };
    }
    //will be destroyed automaticly when descriptorpool is destroyed

    for (size_t i{}; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer    = m_vUniformBuffers[i];
        bufferInfo.offset    = 0;
        bufferInfo.range     = sizeof(UniformBufferObject);
        
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView   = m_TextureImageView;
        imageInfo.sampler     = m_TextureSampler;

        //here they get combined
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet          = m_vDescriptorSets[i];
        descriptorWrites[0].dstBinding      = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo     = &bufferInfo;
        
        descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet          = m_vDescriptorSets[i];
        descriptorWrites[1].dstBinding      = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo      = &imageInfo;
        

        vkUpdateDescriptorSets(m_LogicalDevice, 
            static_cast<uint32_t>(descriptorWrites.size()), 
            descriptorWrites.data(), 0, nullptr);


    }
}

void Game::createBuffer(VkDeviceSize bufferSize, 
                    VkBufferUsageFlags flags, 
                    VkMemoryPropertyFlags memryProps, 
                    VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = flags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //only used by graphicsqueue so exlusive is enough

    if (vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS)
        throw std::runtime_error("creation off vertex buffer failed");

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(m_LogicalDevice, vertexBuffer, &memRequirements);

    //Memory allocation
    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, memryProps);

    if (vkAllocateMemory(m_LogicalDevice, &allocateInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate memory for vertex buffer");

    //Filling the Vertex buffer
    vkBindBufferMemory(m_LogicalDevice, vertexBuffer, vertexBufferMemory, 0);
}

void Game::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleCommands();

    //Copy
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    //Stop recording
    endSingleCommands(commandBuffer);


}

void Game::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutDescription{};
    uboLayoutDescription.binding            = 0;
    uboLayoutDescription.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutDescription.descriptorCount    = 1;
    uboLayoutDescription.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutDescription.pImmutableSamplers = nullptr;//only relevant for image sampling
   
    VkDescriptorSetLayoutBinding samplerLayoutDescription{};
    samplerLayoutDescription.binding            = 1;
    samplerLayoutDescription.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutDescription.descriptorCount    = 1;
    samplerLayoutDescription.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutDescription.pImmutableSamplers = nullptr;//only relevant for image sampling

    std::array<VkDescriptorSetLayoutBinding, 2>  bindings = { uboLayoutDescription, samplerLayoutDescription };
    VkDescriptorSetLayoutCreateInfo uboInfo{};
    uboInfo.sType            = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uboInfo.bindingCount     = static_cast<uint32_t>(bindings.size());
    uboInfo.pBindings        = bindings.data();

    if(vkCreateDescriptorSetLayout(m_LogicalDevice, &uboInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error{ "failed to create descriptor set layout" };
    }
}

void Game::updateUniformBuffer(uint32_t currentImage)
{
    static auto start = std::chrono::high_resolution_clock::now();
    auto currentTime  = std::chrono::high_resolution_clock::now();
    float time        = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - start).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(m_RotationSpeed), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view  = glm::lookAt(m_CameraPos, m_WorldCenter, glm::vec3(0.0f, 0.0f, 1.0f));// up vector
    ubo.proj  = glm::perspective(m_FieldOfView, m_SwapChainExtent.width / (float)m_SwapChainExtent.height, m_NearPlane, m_FarPlane);

    ubo.proj[1][1] *= -1; // flip the y-axis. now it wil be from bottom(0) to top(1)

    //Copy the data in to the current Uniform buffer object
    memcpy(m_vUniformBuffersMapped[currentImage], &ubo, sizeof(ubo));

}

void Game::loadModel()
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warningMessage, errorMessege;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warningMessage, &errorMessege, m_ModelPath.c_str()))
    {
        throw std::runtime_error{ "failed to load in obj" + warningMessage + errorMessege};
    }

    std::unordered_map<Vertex, uint32_t> mUniqueVertexes{};
    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.texcoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
               1.0f - attrib.texcoords[2 * index.texcoord_index + 1] //1.0- is because we flip the y-axis in our settings
            };
            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (mUniqueVertexes.count(vertex) == 0)
            {
                mUniqueVertexes[vertex] = static_cast<uint32_t>(m_vVertices.size());
                m_vVertices.push_back(vertex);
            }

            
            m_vIndices.push_back(mUniqueVertexes[vertex]);
        }
    }
}

void Game::createTextureImage()
{
    //load image
    int texWidth{}, textHeight{}, textChannels{};
    stbi_uc* pixels = stbi_load(m_TexturePath.c_str(), &texWidth, &textHeight, &textChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * textHeight * 4;

    m_MipLvl = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, textHeight)))) + 1;

    if (!pixels)
    {
        throw std::runtime_error{ "failed to load texture image" };
    }

    //create staging buffer to load in host memory
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingbufferMemory;
    createBuffer(imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        stagingBuffer, stagingbufferMemory);

    void* data;
    vkMapMemory(m_LogicalDevice, stagingbufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_LogicalDevice, stagingbufferMemory);
    
    stbi_image_free(pixels);

    //create image to transfer to and bind
    createImage(texWidth, textHeight, m_MipLvl, VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_TextureImage, m_TextureImageMemory);

    transitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLvl);
    copyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(textHeight));
    //transitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_MipLvl);
    generateMipmaps(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, textHeight, m_MipLvl);

    vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_LogicalDevice, stagingbufferMemory, nullptr);
}

void Game::createImage(uint32_t width, uint32_t height, uint32_t mipLvls, VkSampleCountFlagBits numSamples,
    VkFormat format, VkImageTiling tiling, 
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
    VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType      = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width   = static_cast<uint32_t>(width);
    imageInfo.extent.height  = static_cast<uint32_t>(height);
    imageInfo.extent.depth   = 1;
    imageInfo.mipLevels      = mipLvls;
    imageInfo.arrayLayers    = 1;
    imageInfo.format         = format; //!!use same format as used with the pixels
    imageInfo.tiling         = tiling; //use LINEAR if you want direct acces to the pixels
    imageInfo.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage          = usage;
    imageInfo.sharingMode    = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples        = numSamples;
    imageInfo.flags          = 0;

    if (vkCreateImage(m_LogicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error{ "failed to create image" };
    }

    //transfer to fast allocation memory
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_LogicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error{ "failed to allocate image memory" };
    }

    vkBindImageMemory(m_LogicalDevice, image, imageMemory, 0);

}

void Game::createTextureImageView()
{
    m_TextureImageView = createImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLvl);
}

void Game::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    //if you dont want to use anisotropic behaviour
    //samplerInfo.anisotropyEnable = VK_FALSE;
    //samplerInfo.maxAnisotropy = 1.0f;


    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE; //-> false returns 0-1| true returns 0-width, 0-height
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp     = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias    = 0.0f;
    samplerInfo.minLod        = static_cast<float>(m_MipLvl/2.f);
    samplerInfo.maxLod        = static_cast<float>(m_MipLvl);

    if (vkCreateSampler(m_LogicalDevice, &samplerInfo, nullptr, &m_TextureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error{ "failed to create texture sampler" };
    }

}

void Game::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();
    createImage(m_SwapChainExtent.width, m_SwapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
    m_DepthImageView = createImageView(m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
  
    //optional since its mostly handeld by render pass
    transitionImageLayout(m_DepthImage, depthFormat, 
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

}

void Game::transitionImageLayout(VkImage image, VkFormat format, 
                        VkImageLayout oldLayout, VkImageLayout newLayout,
                            uint32_t mipLvls)
{
    VkCommandBuffer commandbuffer = beginSingleCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (hasStencilComponent(format))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLvls;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;
    VkPipelineStageFlags srceStage;
    VkPipelineStageFlags dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
        && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
        && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        srceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandbuffer,
       srceStage, dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleCommands(commandbuffer);

}

void Game::generateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = beginSingleCommands();

    //check if physical device supports linear filtering
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) 
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }
    //--
    
    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image                           = image;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;
    
    int32_t mipWidth  = texWidth;
    int32_t mipHeight = texHeight;
    for (uint32_t i{ 1 }; i < mipLevels; ++i)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                  0, nullptr,
                  0, nullptr,
                  1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
                        image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1, &blit,
                        VK_FILTER_LINEAR);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;

    }
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);


    endSingleCommands(commandBuffer);
}

VkImageView Game::createImageView(VkImage image, VkFormat format, 
                                VkImageAspectFlags aspectFlags, uint32_t mipLvls)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                   = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                   = image;
    viewInfo.viewType                = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                  = format;
    viewInfo.subresourceRange.aspectMask     = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = mipLvls;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;


    VkImageView imageView;
    if (vkCreateImageView(m_LogicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }


    return imageView;
}

VkFormat Game::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    throw std::runtime_error{ "failde to find a correct format for depth resources" };
}

VkFormat Game::findDepthFormat()
{
    return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

}

bool Game::hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkCommandBuffer Game::beginSingleCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = m_CommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Game::endSingleCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_GraphicsQueue);

    vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, 1, &commandBuffer);

}

void Game::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleCommands();


    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0,0,0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleCommands(commandBuffer);

}

VkSampleCountFlagBits Game::getMaxUsableSampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = 
        physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
    
    return VK_SAMPLE_COUNT_1_BIT;
}