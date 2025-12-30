#include <cstdio>
#include <cassert>
#include <cstring>

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#define ARRAYSZ(a) sizeof(a) / sizeof(a[0])
#define VK_CHECK(call) \
do{ \
	VkResult result_ = call; \
	if (result_ != VK_SUCCESS) { \
		printf("Vulkan error: %d at %s:%d\n", result_, __FILE__, __LINE__); \
	} \
	assert(result_ == VK_SUCCESS); \
} while(0)

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
    	bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

VkInstance createInstance(){
	if(enableValidationLayers)
		assert(checkValidationLayerSupport());

	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	VkInstance instance = 0;
	assert(vkCreateInstance(&createInfo, nullptr, &instance) == VK_SUCCESS);
	return instance;
}

VkPhysicalDevice pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDeviceCount){
	for(int i = 0; i < physicalDeviceCount; i++){

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevices[i], &props);
	
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevices[i], &deviceFeatures);

		if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader){
			printf("Picking discrete GPU %s\n", props.deviceName);
			return physicalDevices[i];
		}
	}

	if(physicalDeviceCount > 0){
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevices[0], &props);

		printf("Picking fallback GPU %s\n", props.deviceName);
		return physicalDevices[0];
	}

	printf("No physical devices available!");
	return 0; // VK_NULL_HANDLE
}


int main(){
	int rc = glfwInit();
	assert(rc);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // no OpenGL context
	
	VkInstance instance = createInstance();

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()); // query the extension details
	printf("available extensions:\n");

	for (const auto& extension : extensions){
    	printf("\t %s\n", extension.extensionName);
	}
	
	GLFWwindow* window = glfwCreateWindow(1024, 728, "tinyrt", nullptr, nullptr);
	assert(window);
	

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));



	VkPhysicalDevice physicalDevices[8];
	uint32_t physicalDeviceCount = ARRAYSZ(physicalDevices);
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));
	
	VkPhysicalDevice physicalDevice = pickPhysicalDevice(physicalDevices, physicalDeviceCount);
	assert(physicalDevice != VK_NULL_HANDLE);

	uint32_t queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);

	VkQueueFamilyProperties queueProps[8]; // TODO: using const, use vector etc.
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps);
	
	uint32_t graphicsQueueId = 0;
	for (int i = 0; i < queueCount; i++) {
		if (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicsQueueId = i;
			break;
		}
	}

	VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueCreateInfo.queueFamilyIndex = graphicsQueueId;
	queueCreateInfo.queueCount = 1;
	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDevice device;
	{
		VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pEnabledFeatures = &deviceFeatures;

		const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		createInfo.enabledExtensionCount = 1;
		createInfo.ppEnabledExtensionNames = deviceExtensions;

		VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
	}

	VkQueue graphicsQueue;
	vkGetDeviceQueue(device, graphicsQueueId, 0, &graphicsQueue);

	VkBool32 presentSupport = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsQueueId, surface, &presentSupport); // drawing&present in the same queue
	assert(presentSupport);

	VkSurfaceCapabilitiesKHR caps;
	VkSurfaceFormatKHR formats[32];
	VkPresentModeKHR presentModes[16];

    VkExtent2D                       currentExtent;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);
	printf("number of images\t min: %u, max: %u\nimage size\t current [%u, %u]\n ", caps.minImageCount, caps.maxImageCount, caps.currentExtent.height, caps.currentExtent.width);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	assert(formatCount && formatCount < ARRAYSZ(formats));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	assert(presentModeCount && presentModeCount < ARRAYSZ(presentModes));
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);

	VkSurfaceFormatKHR chosenFormat = formats[0];
	for (int i = 0; i < formatCount; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFormat = formats[i];
        }
    }

	VkPresentModeKHR chosenMode = presentModes[0];
	for (int i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosenMode = presentModes[i];
        }
    }

	uint32_t swapImageCount = caps.minImageCount + 1;
	VkSwapchainKHR swapChain;
	{
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = swapImageCount;
		createInfo.imageFormat = chosenFormat.format;
		createInfo.imageColorSpace = chosenFormat.colorSpace;
		createInfo.imageExtent = caps.currentExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
		// or if we have multiple queues, not sharing a queue for drawing&present
		// createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		// createInfo.queueFamilyIndexCount = 2;
		// createInfo.pQueueFamilyIndices = queueFamilyIndices;
		createInfo.preTransform = caps.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = chosenMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain));
	}

	VkImage swapChainImages[8];
	// vkGetSwapchainImagesKHR(device, swapChain, &swapImageCount, nullptr);
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapChain, &swapImageCount, swapChainImages));
	VkImageView swapChainImageViews[8];
	{
		for (size_t i = 0; i < swapImageCount; i++) {
			VkImageViewCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = chosenFormat.format;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;
			VK_CHECK(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]));
		}
		
	}

	VkSemaphore acqSemaphore;
	{
		VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &acqSemaphore));
	}

	VkSemaphore relSemaphore;
	{
		VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &relSemaphore));
	}

	VkCommandPool cmdPool;
	{
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = graphicsQueueId;

		VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &cmdPool));
	}

	VkCommandBuffer cmdBuffer;
	{
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandPool = cmdPool;
		allocateInfo.commandBufferCount = 1;

		VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &cmdBuffer));
	}




	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		uint32_t imageIndex;
		VK_CHECK(vkAcquireNextImageKHR(device, swapChain, ~0ull, acqSemaphore, VK_NULL_HANDLE, &imageIndex));

		VK_CHECK(vkResetCommandPool(device, cmdPool, 0));
		{
			VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
		}

		VkClearColorValue color = {1, 0, 1, 1};
		VkImageSubresourceRange range;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.levelCount = 1;
		range.layerCount = 1;
		range.baseArrayLayer = 0;
		range.baseMipLevel = 0;

		vkCmdClearColorImage(cmdBuffer, swapChainImages[imageIndex], VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range);

		VK_CHECK(vkEndCommandBuffer(cmdBuffer));

		VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &relSemaphore;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &acqSemaphore;
		submitInfo.pWaitDstStageMask = &submitStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));


		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.pWaitSemaphores = &relSemaphore;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &imageIndex;

		VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

		VK_CHECK(vkDeviceWaitIdle(device));

	}

	vkDestroySemaphore(device, acqSemaphore, nullptr);
	vkDestroySemaphore(device, relSemaphore, nullptr);
	vkDestroyCommandPool(device, cmdPool, nullptr);


	for (size_t i = 0; i < swapImageCount; i++) {
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

	vkDestroySwapchainKHR(device, swapChain, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();

}
