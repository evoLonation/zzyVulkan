import <limits>;
import <optional>;
import <iostream>;
import <string>;
import <format>;
import <vector>;
import <map>;
import <ranges>;
import <concepts>;
import <print>;
import <functional>;
import <algorithm>;

import "vulkan_config.h";



template<uint32_t index, typename Callable>
struct FuncArgT;

template<uint32_t index, typename Func>
using FuncArg = typename FuncArgT<index, Func>::type;

// 用于成员函数类型 (需要考虑 const 和非 const)
template<uint32_t index, typename C, typename R, typename... Args>
struct FuncArgT<index, R(C::*)(Args...) const>
{
	using type = std::tuple_element_t<index, std::tuple<Args...>>;
};
template<uint32_t index, typename C, typename R, typename... Args>
struct FuncArgT<index, R(C::*)(Args...)>
{
	using type = std::tuple_element_t<index, std::tuple<Args...>>;
};
// 用于函数指针
template<uint32_t index, typename R, typename... Args>
struct FuncArgT<index, R(*)(Args...)>
{
	using type = std::tuple_element_t<index, std::tuple<Args...>>;
};
// 用于函数
template<uint32_t index, typename R, typename... Args>
struct FuncArgT<index, R(Args...)>
{
	using type = std::tuple_element_t<index, std::tuple<Args...>>;
};
// 用于持有 operator() 的类
template<uint32_t index, typename Callable>
struct FuncArgT
{
	using type = FuncArgT<index, decltype(&Callable::operator())>::type;
};

class VulkanApplication
{
public:
	VulkanApplication(const uint32_t width, const uint32_t height, const std::string_view appName);

	~VulkanApplication();

	VulkanApplication(const VulkanApplication& other) = delete;
	VulkanApplication(VulkanApplication&& other) noexcept = delete;
	VulkanApplication& operator=(const VulkanApplication& other) = delete;
	VulkanApplication& operator=(VulkanApplication&& other) noexcept = delete;

	[[nodiscard]] GLFWwindow* pWindow() const { return pWindow_; }

private:
	template<typename F, typename... Args>
		requires std::invocable<F, Args&&..., uint32_t*, FuncArg<sizeof...(Args) + 1, F>>
	static auto getVkResource(F func, Args&&... args)
	{
		uint32_t count;
		func(args..., &count, nullptr);
		std::vector<std::remove_pointer_t<FuncArg<sizeof...(Args) + 1, F>>> resources{ count };
		func(args..., &count, resources.data());
		return resources;
	}

private:
/*
 * glfw window 相关
 */
	GLFWwindow* pWindow_;

	void createWindow(const uint32_t width, const uint32_t height, const std::string_view title);
	void destroyWindow() noexcept;

	// 抛出glfw的报错（如果有的话）
	static void checkGlfwError();

private:
/*
 * 是否启用验证层，用于debug
 */
#ifdef NODEBUG
	static constexpr bool enableValiLayer = false;
	static constexpr bool enableDebugOutput = false;
#else
	static constexpr bool enableValiLayer = true;
	static constexpr bool enableDebugOutput = true;
	static constexpr VkDebugUtilsMessageSeverityFlagBitsEXT vkMessageSeverityLevel = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
	static constexpr VkDebugUtilsMessageTypeFlagBitsEXT vkMessageTypes = VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT;
#endif

private:
/*
 * instance, extension, layer相关
 * instance: vulkan最底层的对象，一切皆源于instance，存储几乎所有状态
 */
	// 该类型本质是一个指针，后续的device也类似
	VkInstance instance_;
	// vulkan中的回调也是一种资源，需要创建
	VkDebugUtilsMessengerEXT debugMessenger_;

	void createInstance(const std::string_view appName);
	void destroyInstance() noexcept;

	// 获得所有可能需要提供的 instance 级别的 extension
	// glfw 需要的扩展用于 vulkan 与窗口对接
	// VK_EXT_debug_utils 扩展用于扩展debug功能
	static std::vector<const char*> getInstanceRequiredExtensions();

	static std::vector<const char*> getRequiredLayers();

	// debug 输出的处理函数
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugHandler(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	//需要借助instance 来手动加载 debugMessenger 的create和destroy函数
	static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

private:
	/*
	 * surface 相关
	 * 借助 instance 创建 surface
	 */
	VkSurfaceKHR surface_;

	void createSurface();
	void destroySurface() noexcept;

private:
/*
 * physical device 相关
 * 借助 instance 得到所有 physical device
 * 与 surface 结合得到 physical device 的属性，并选择一个合适的
 * features, queue family indices , extensions 用于创建 logical device
 * surface capability, format, present mode 用于创建 swap chain
 */
	VkPhysicalDevice physicalDevice_;

	VkPhysicalDeviceFeatures physicalDeviceFeatures_;
	struct QueueFamilyIndices
	{
		uint32_t graphicsFamily;
		uint32_t presentFamily;
	};
	QueueFamilyIndices queueFamilyIndices_;

	std::vector<const char*> deviceExtensions_;

	VkSurfaceCapabilitiesKHR surfaceCapabilities_;
	VkSurfaceFormatKHR surfaceFormat_;
	VkPresentModeKHR surfacePresentMode_;

	// 得到合格的 physical device 及其相关信息，用于之后的 device 创建
	void pickPhysicalDevice();

	// 获得所有可能需要提供的 physical device 级别的 extension
	// VK_KHR_SWAPCHAIN_EXTENSION_NAME 对应的扩展用于支持交换链
	static std::vector<const char*> getRequiredDeviceExtensions(VkPhysicalDevice device);

	static QueueFamilyIndices getQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface);

	// 根据 device, surface 获得需要的 capability(extent, image count), format, present mode
	static std::tuple<VkSurfaceCapabilitiesKHR, VkSurfaceFormatKHR, VkPresentModeKHR> getSwapChainSupport(
		VkPhysicalDevice device, VkSurfaceKHR surface);

private:
/* logical device 相关
 * 创建 logical device, 得到 queue
 */

	VkDevice device_;
	struct Queues
	{
		VkQueue graphicsQueue;
		VkQueue presentQueue;
	};
	Queues queues_;
	
	// 根据 physical 创建 logical device ，根据 family 创建 queue
	void createLogicalDevice();
	void destroyLogicalDevice() noexcept;

private:
/*
 * swap chain 相关
 */
	VkSwapchainKHR swapChain_;
	std::vector<VkImage> swapChainImages_;

	void createSwapChain();
	void destroySwapChain() noexcept;

};

VkResult VulkanApplication::createDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger) {
	if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")); func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanApplication::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator) {
	if (const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")); func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void VulkanApplication::createWindow(const uint32_t width, const uint32_t height, const std::string_view title)
{
	try {
		if (glfwInit() != GLFW_TRUE) {
			throw std::runtime_error("glfw init failed");
		}
		// 不要创建openGL上下文
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// 禁用改变窗口尺寸
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		pWindow_ = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);

		checkGlfwError();
	}
	catch (const std::exception& e)
	{
		destroyWindow();
		throw;
	}
}

void VulkanApplication::destroyWindow() noexcept
{
	if (pWindow_ != nullptr) {
		glfwDestroyWindow(pWindow_);
	}
	glfwTerminate();
}

void VulkanApplication::checkGlfwError()
{
	std::string error;
	const char* description;
	bool isError = false;

	while (glfwGetError(&description) != GLFW_NO_ERROR)
	{
		isError = true;
		error.append(description).append("\n");
	}
	if (isError) {
		glfwTerminate();
		throw std::runtime_error(error);
	}
}

void VulkanApplication::createInstance(const std::string_view appName) {
	/*
		 * 1. 创建appInfo
		 * 2. 创建createInfo（指向appInfo）
		 * 3. 调用createInstance创建instance
		*/
	// 大多数 info 结构体的 sType 成员都需要显式声明
	// create 函数忽略allocator参数
	// 选择开启验证层

	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = appName.data(),
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0,
	};

	const auto requiredExtensions = getInstanceRequiredExtensions();
	const auto requiredLayers = getRequiredLayers();
	// checkExtensionSupport(requiredExtensions);
	// checkLayerSupport(requiredLayers_);

	VkInstanceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
		.ppEnabledLayerNames = requiredLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
		.ppEnabledExtensionNames = requiredExtensions.data(),
	};

	auto instanceCreater = [&createInfo, this]() {
		if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vulkan instance");
		}
	};

	if constexpr (enableValiLayer) {
		/*
			* VkDebugUtilsMessengerCreateInfoEXT: 设置要回调的消息类型、回调函数指针
			*/
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debugHandler,
			.pUserData = nullptr
		};
		createInfo.pNext = &debugMessengerCreateInfo;

		instanceCreater();

		if (createDebugUtilsMessengerEXT(instance_, &debugMessengerCreateInfo, nullptr, &debugMessenger_) != VK_SUCCESS) {
			throw std::runtime_error("failed to create debug messenger");
		}
	}
	else {
		instanceCreater();
	}
}

void VulkanApplication::destroyInstance() noexcept {
	if constexpr (enableValiLayer) {
		destroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
	}
	vkDestroyInstance(instance_, nullptr);
}


void VulkanApplication::pickPhysicalDevice()
{
	std::vector<VkPhysicalDevice> devices = getVkResource(vkEnumeratePhysicalDevices, instance_);
	for(const auto device: devices) {
		try {
			/*
			 * 1. 检查 property
			 * 2. 检查 feature
			 * 3. 检查 extension
			 * 4. 检查 queue family
			 * 5. 检查 swap chain support
			 * 6. 赋值 
			 */
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				throw std::runtime_error("device not satisfied VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU");
			}

			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
			if (deviceFeatures.geometryShader != VK_TRUE) {
				throw std::runtime_error("device not satisfied geometryShader feature");
			}

			auto deviceExtensions = getRequiredDeviceExtensions(device);
			auto queueFamilyIndices = getQueueFamilyIndices(device, surface_);
			auto swapChainSupports = getSwapChainSupport(device, surface_);

			physicalDevice_			= device;
			physicalDeviceFeatures_ = deviceFeatures;
			deviceExtensions_		= std::move(deviceExtensions);
			queueFamilyIndices_		= queueFamilyIndices;
			surfaceCapabilities_	= std::get<0>(swapChainSupports);
			surfaceFormat_			= std::get<1>(swapChainSupports);
			surfacePresentMode_		= std::get<2>(swapChainSupports);
			return;
		}catch (const std::exception& e) {}
	}
	throw std::runtime_error("can not find suitable physical device");
}

void VulkanApplication::createLogicalDevice()
{
	/*
	 * 1. 创建 VkDeviceQueueCreateInfo 数组, 用于指定 logic device 中的队列
	 * 2. 根据 queue create info 和 deviceFeatures_ 创建 device create info
	 *
	 */

	// 一个 queueCreateInfo 可以创建多个来自于 相同 队列族的队列
	// queueCreateInfos 中的每个 queueCreateInfo 需要指定不同的队列族
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	std::vector<uint32_t> indices{ queueFamilyIndices_.graphicsFamily, queueFamilyIndices_.presentFamily };
	std::map<uint32_t, uint32_t> index2CountMap;
	std::map<uint32_t, std::vector<float>> index2PrioritiesMap;
	for (const auto index : indices) {
		index2PrioritiesMap[index].push_back(1.0f);
		if (index2CountMap.contains(index)) {
			index2CountMap[index]++;
		}
		else {
			index2CountMap[index] = 1;
		}
	}

	for (const auto [index, count] : index2CountMap) {
		queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = index,
			.queueCount = count,
			.pQueuePriorities = index2PrioritiesMap[index].data(),
		});
	}

	VkDeviceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions_.size()),
		.ppEnabledExtensionNames = deviceExtensions_.data(),
		.pEnabledFeatures = &physicalDeviceFeatures_,
	};

	// 旧实现在 (instance, physical device) 和 (logic device以上) 两个层面有不同的 layer, 而在新实现中合并了
	// 不再需要定义 enabledLayerCount 和 ppEnabledLayerNames
	// createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers_.size());
	// createInfo.ppEnabledLayerNames = requiredLayers_.data();

	if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	std::map<uint32_t, uint32_t> family2QueueIndexMap;
	for (const auto index : index2CountMap | std::views::keys) {
		family2QueueIndexMap[index] = 0;
	}
	// 要与前面的 indices 相对应
	std::vector<VkQueue Queues::*> pQueues{ &Queues::graphicsQueue, &Queues::presentQueue };
	for(const auto [familyIndex, pQueue]: std::views::zip(indices, pQueues)) {
		auto& queueIndex = family2QueueIndexMap[familyIndex];
		vkGetDeviceQueue(device_, familyIndex, queueIndex, &(queues_.*pQueue));
		queueIndex++;
	}
}

void VulkanApplication::destroyLogicalDevice() noexcept
{
	vkDestroyDevice(device_, nullptr);
}

void VulkanApplication::createSwapChain()
{
	uint32_t imageCount = surfaceCapabilities_.minImageCount + 1;
	// maxImageCount == 0意味着没有最大值
	if(surfaceCapabilities_.maxImageCount != 0) {
		imageCount = std::min(imageCount, surfaceCapabilities_.maxImageCount);
	}

	VkExtent2D extent{};
	if (surfaceCapabilities_.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		extent = surfaceCapabilities_.currentExtent;
	}else {
		int width, height;
		glfwGetFramebufferSize(pWindow_, &width, &height);
		extent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		extent.width = std::clamp(extent.width, surfaceCapabilities_.minImageExtent.width, surfaceCapabilities_.maxImageExtent.width);
		extent.height = std::clamp(extent.height, surfaceCapabilities_.minImageExtent.height, surfaceCapabilities_.maxImageExtent.height);
	}

	VkSwapchainCreateInfoKHR createInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface_,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat_.format,
		.imageColorSpace = surfaceFormat_.colorSpace,
		.imageExtent = extent,
		// 不整 3D 应用程序的话就设置为1
		.imageArrayLayers = 1,
		/*
		 * VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT: 交换链的图像直接用于渲染
		 * VK_IMAGE_USAGE_TRANSFER_DST_BIT : 先渲染到单独的图像上（以便进行后处理），然后传输到交换链图像
		 */
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = surfaceCapabilities_.currentTransform,
		// alpha通道是否应用于与窗口系统中的其他窗口混合
		// 简单地忽略alpha通道
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = surfacePresentMode_,
		// 不关心被遮挡的像素的颜色
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};

	/*
	 * VK_SHARING_MODE_CONCURRENT: 图像可以跨多个队列族使用，而无需明确的所有权转移
	 * VK_SHARING_MODE_EXCLUSIVE: 一个图像一次由一个队列族所有，在将其用于另一队列族之前，必须明确转移所有权
	 */
	const std::array<uint32_t, 2> queueFamilyIndices = { queueFamilyIndices_.graphicsFamily, queueFamilyIndices_.presentFamily};
	if (queueFamilyIndices_.graphicsFamily != queueFamilyIndices_.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	}else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; 
		createInfo.pQueueFamilyIndices = nullptr; 
	}

	if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain");
	}

	if constexpr (enableDebugOutput) {
		std::println("the info of created swap chain:");
		std::println("image count:{}", imageCount);
		std::println("extent:({},{})", extent.width, extent.height);
	}

	swapChainImages_ = getVkResource(vkGetSwapchainImagesKHR, device_, swapChain_);
}

void VulkanApplication::destroySwapChain() noexcept
{
	vkDestroySwapchainKHR(device_, swapChain_, nullptr);
}

std::vector<const char*> VulkanApplication::getRequiredDeviceExtensions(VkPhysicalDevice device)
{
	std::vector<const char*> requiredExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	/*
	 * 使用vkEnumerateInstance系列函数检查需要的 extension 是否支持
	 */
	std::vector<VkExtensionProperties> availableExtensions = getVkResource(vkEnumerateDeviceExtensionProperties, device, nullptr);

	if constexpr (enableDebugOutput) {
		std::println("the available {} device extensions are:", availableExtensions.size());
		for (const auto& [extensionName, specVersion] : availableExtensions) {
			std::println("{} (version {})", extensionName, specVersion);
		}
	}

	std::vector<std::string_view> unsupportedExtensions;
	for (const auto required : requiredExtensions) {
		if (
			std::ranges::find_if(availableExtensions,
				[required](const VkExtensionProperties& available) {
					return std::string_view(required) == available.extensionName;
				})
			== availableExtensions.end())
		{
			unsupportedExtensions.push_back(required);
		}
	}
	if (unsupportedExtensions.size() != 0) {
		std::string str =
			unsupportedExtensions |
			std::views::join_with(',') |
			std::ranges::to<std::string>();
		throw std::runtime_error(std::format("device extension requested {}, but not available", str));
	}

	return requiredExtensions;
}

VulkanApplication::QueueFamilyIndices VulkanApplication::getQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	std::vector<VkQueueFamilyProperties> queueFamilies = getVkResource(vkGetPhysicalDeviceQueueFamilyProperties, device);

	QueueFamilyIndices queueFamilyIndices{};
	bool graphic = false;
	bool present = false;

	auto pGraphicsFamily = std::ranges::find_if(queueFamilies, [](const auto& family) {
		return family.queueFlags & VK_QUEUE_GRAPHICS_BIT;
	});
	if (pGraphicsFamily == queueFamilies.end()) throw std::runtime_error("can not found queue family which satisfied VK_QUEUE_GRAPHICS_BIT");
	queueFamilyIndices.graphicsFamily = pGraphicsFamily - queueFamilies.begin();

	auto enumQueueFamilies = queueFamilies | std::views::enumerate;
	auto pPresentFamily = std::ranges::find_if(enumQueueFamilies, [device, surface](const auto& pair) {
		const auto& [index, family] = pair;
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupport);
		return presentSupport == VK_TRUE;
	});
	if (pPresentFamily == enumQueueFamilies.end()) throw std::runtime_error("can not found queue family which satisfied SurfaceSupport");
	queueFamilyIndices.presentFamily = pPresentFamily - enumQueueFamilies.begin();

	if constexpr (enableDebugOutput) {
		std::println("choose queue family {} for graphics, {} for present",
			queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily);
	}
	return queueFamilyIndices;
}

std::tuple<VkSurfaceCapabilitiesKHR, VkSurfaceFormatKHR, VkPresentModeKHR> VulkanApplication::getSwapChainSupport(
	VkPhysicalDevice device, VkSurfaceKHR surface)
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

	std::vector<VkSurfaceFormatKHR> formats = getVkResource(vkGetPhysicalDeviceSurfaceFormatsKHR, device, surface);
	auto pFormat = std::ranges::find_if(formats, [](auto format) {
		return format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	});
	if (pFormat == formats.end()) throw std::runtime_error("no suitable format");

	std::vector<VkPresentModeKHR> presentModes = getVkResource(vkGetPhysicalDeviceSurfacePresentModesKHR, device, surface);
	/*
	 * VK_PRESENT_MODE_IMMEDIATE_KHR: 图像提交后直接渲染到屏幕上
	 * VK_PRESENT_MODE_FIFO_KHR: 有一个队列，队列以刷新率的速度消耗图像显示在屏幕上，图像提交后入队，队列满时等待（也即只能在 "vertical blank" 时刻提交图像）
	 * VK_PRESENT_MODE_FIFO_RELAXED_KHR: 当图像提交时，若队列为空，就直接渲染到屏幕上，否则同上
	 * VK_PRESENT_MODE_MAILBOX_KHR: 当队列满时，不阻塞而是直接将队中图像替换为已提交的图像
	 */
	auto pPresentMode = std::ranges::find(presentModes, VK_PRESENT_MODE_FIFO_KHR);
	if(pPresentMode == presentModes.end()) throw std::runtime_error("no suitable present mode");

	return { capabilities, *pFormat, *pPresentMode };
}

void VulkanApplication::createSurface()
{
	VkWin32SurfaceCreateInfoKHR createInfo{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = GetModuleHandle(nullptr),
		.hwnd = glfwGetWin32Window(pWindow_),
	};
	
	if(vkCreateWin32SurfaceKHR(instance_, &createInfo, nullptr, &surface_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create surface!");
	}
}

void VulkanApplication::destroySurface() noexcept
{
	vkDestroySurfaceKHR(instance_, surface_, nullptr);
}

VulkanApplication::VulkanApplication(const uint32_t width, const uint32_t height, const std::string_view appName)
{
	pWindow_ = nullptr;
	instance_ = VK_NULL_HANDLE;
	surface_ = VK_NULL_HANDLE;
	device_ = VK_NULL_HANDLE;
	swapChain_ = VK_NULL_HANDLE;
	createWindow(width, height, appName);
	createInstance(appName);
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
}

VulkanApplication::~VulkanApplication()
{
	destroySwapChain();
	destroyLogicalDevice();
	destroySurface();
	destroyInstance();
	destroyWindow();
}


std::vector<const char*> VulkanApplication::getRequiredLayers() {
	std::vector<const char*> requiredLayers;
	if constexpr (enableValiLayer) {
		requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
	}

	std::vector<VkLayerProperties> availableLayers = getVkResource(vkEnumerateInstanceLayerProperties);

	if constexpr (enableDebugOutput) {
		std::println("the available {} layers are:", availableLayers.size());
		for (const auto& [layerName, specVersion, implementationVersion, description] : availableLayers) {
			std::println("{} (spec version {}, implementation version {}) : {} ", layerName, specVersion, implementationVersion, description);
		}
	}

	std::vector<std::string_view> unsupportedLayers;
	for (const auto required : requiredLayers) {
		if (
			std::ranges::find_if(availableLayers,
				[required](const VkLayerProperties& available) {
					return std::string_view(required) == available.layerName;
				})
			== availableLayers.end())
		{
			unsupportedLayers.push_back(required);
		}
	}
	if (unsupportedLayers.size() != 0) {
		if (unsupportedLayers.size() != 0) {
			std::string str =
				unsupportedLayers |
				std::views::join_with(',') |
				std::ranges::to<std::string>();
			throw std::runtime_error(std::format("layer requested {}, but not available", str));
		}
	}

	return requiredLayers;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApplication::debugHandler(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType, 
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity < vkMessageSeverityLevel)return VK_FALSE;
	if (!(messageType & vkMessageTypes)) return VK_FALSE;
	/*
	 * VkDebugUtilsMessageSeverityFlagBitsEXT : 严重性， VERBOSE, INFO, WARNING, ERROR (可以比较，越严重越大）
	 * VkDebugUtilsMessageTypeFlagsEXT : 类型， GENERAL, VALIDATION, PERFORMANCE
	 * return: 是否要中止 触发验证层消息 的 vulkan调用
	 */
	auto serverityGetter = [](VkDebugUtilsMessageSeverityFlagBitsEXT e) {
		switch (e) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "VERBOSE";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "INFO";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "WARNING";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "ERROR";
		default: return "OTHER";
		}
	};
	auto typeGetter = [](VkDebugUtilsMessageTypeFlagsEXT e) {
		switch (e) {
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "GENERAL";
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "VALIDATION";
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "PERFORMANCE";
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: return "DEVICE_ADDRESS_BINDING";
		default: return "OTHER";
		}
	};
	std::println("validation layer: ({},{}) {}",
	      serverityGetter(messageSeverity),
	      typeGetter(messageType),
	      pCallbackData->pMessage);

	return VK_FALSE;
}

std::vector<const char*> VulkanApplication::getInstanceRequiredExtensions() {
	uint32_t glfwExtensionCount;
	const auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> requiredExtensions{ glfwExtensions, glfwExtensions + glfwExtensionCount };

	if constexpr (enableValiLayer) {
		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	
	/*
	 * 使用vkEnumerateInstance系列函数检查需要的 extension 是否支持
	 */
	std::vector<VkExtensionProperties> availableExtensions = getVkResource(vkEnumerateInstanceExtensionProperties, nullptr);

	if constexpr (enableDebugOutput) {
		std::println("the available {} extensions are:", availableExtensions.size());
		for (const auto& [extensionName, specVersion] : availableExtensions) {
			std::println("{} (version {})", extensionName, specVersion);
		}
	}

	std::vector<std::string_view> unsupportedExtensions;
	for (const auto required : requiredExtensions) {
		if(
			std::ranges::find_if(availableExtensions, 
			[required](const VkExtensionProperties& available) {
				return std::string_view(required) == available.extensionName;
			}) 
			== availableExtensions.end()) 
		{
			unsupportedExtensions.push_back(required);
		}
	}
	if(unsupportedExtensions.size() != 0) {
		std::string str = 
			unsupportedExtensions | 
			std::views::join_with(',') | 
			std::ranges::to<std::string>();
		throw std::runtime_error(std::format("extension requested {}, but not available", str));
	}

	return requiredExtensions;
}

int main() {
	try {
        std::string applicationName = "hello, vulkan!";
        uint32_t width = 800;
        uint32_t height = 600;
		VulkanApplication application{width, height, applicationName };

		glfwSetKeyCallback(application.pWindow(), [](GLFWwindow* pWindow, int key, int scancode, int action, int mods) {
			if (action == GLFW_PRESS) {
				std::cout << "press key!" << std::endl;
			}
		});

        while (!glfwWindowShouldClose(application.pWindow())) {
            glfwPollEvents();
        }

        
    }
    catch (const std::exception& e) {

		std::print("catch exception at root:\n{}\n", e.what());
        return 1;
    }
    return 0;
}
