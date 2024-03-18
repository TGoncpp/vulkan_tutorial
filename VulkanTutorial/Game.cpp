#include "Game.h"
#include <set>
#include<algorithm>     // for clamp
#include <limits>       //for numeric_limits
//#include <cstdint>      // for uint32_t

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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // Tell GLFW not to bother with resizing the window
    m_Window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, m_WindowTitle.c_str(), nullptr, nullptr);
}

void Game::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageView();
    createRenderPass();
    createGraphicsPipeline();
}

void Game::mainLoop()
{
    while (!glfwWindowShouldClose(m_Window))
    {
        glfwPollEvents();
    }
}

void Game::cleanup()
{
    vkDestroyPipeline(m_LogicalDevice, m_GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
    vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, nullptr);
    for (auto imageView : m_vSwapChainImageViews) {
        vkDestroyImageView(m_LogicalDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(m_LogicalDevice, m_SwapChain, nullptr);
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
    //VkPhysicalDeviceFeatures deviceFeats;                            //-> for optional render features like for VR
    //vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &deviceFeats);

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

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
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

void Game::createImageView()
{
    m_vSwapChainImageViews.resize(m_vSwapChainImages.size());

    for (size_t i = 0; i < m_vSwapChainImages.size(); i++) 
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_vSwapChainImages[i];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_SwapChainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_LogicalDevice, &createInfo, nullptr, &m_vSwapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void Game::createGraphicsPipeline()
{
    auto vertShader = readFile("vert.spv");
    auto fragShader = readFile("frag.spv");

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
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = static_cast<uint32_t>(vDynamicStates.size());
    dynamicInfo.pDynamicStates = vDynamicStates.data();

    //vertex format info 
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.pVertexBindingDescriptions      = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions    = nullptr; // Optional

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

    //Rastereization info
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE; //if true : clamps values to the far/near planes when close to
    rasterizer.rasterizerDiscardEnable = VK_FALSE; //dissables output to frameBuffer
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
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
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    //Pipeline Layout ->used for dynamic behaviour like passing the tranform matrix to vertexshader or texture sampler to fragment shader
    VkPipelineLayoutCreateInfo pipelineLayout{};
    pipelineLayout.sType                   = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayout.pSetLayouts             = 0;//optional
    pipelineLayout.pSetLayouts             = nullptr;//optional
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
    pipelineInfo.pDepthStencilState    = nullptr; //optional
    pipelineInfo.pColorBlendState      = &colorBlending;
    pipelineInfo.pDynamicState         = &dynamicInfo;
    //layout
    pipelineInfo.layout = m_PipelineLayout;
    //Render pass
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = 0; //index
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; //optional
    pipelineInfo.basePipelineIndex = -1; //optional

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


    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subPass{};
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPass.colorAttachmentCount = 1;
    subPass.pColorAttachments = &colorAttachmentRef;


    //Creating the RENDERPASS
    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &collorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subPass;

    if (vkCreateRenderPass(m_LogicalDevice, &createInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Creating renderPass failed");
    }


}
