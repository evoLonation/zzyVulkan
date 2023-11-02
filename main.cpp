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

import "vulkan_config.h";


// void print(std::string_view fmtStr, auto&&... args) {
//     // format�����ĵ�һ������ֻ֧�ֱ����ڳ���
//     // ���ʹ��vformat�������䱾���ǲ����б�������ʽ����format
//     std::cout << std::vformat(fmtStr, std::make_format_args(args...));
// }


template<uint32_t index, typename Callable>
struct FuncArgT;

template<uint32_t index, typename Func>
using FuncArg = typename FuncArgT<index, Func>::type;

// ���ڳ�Ա�������� (��Ҫ���� const �ͷ� const)
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
// ���ں���ָ��
template<uint32_t index, typename R, typename... Args>
struct FuncArgT<index, R(*)(Args...)>
{
	using type = std::tuple_element_t<index, std::tuple<Args...>>;
};
// ���ں���
template<uint32_t index, typename R, typename... Args>
struct FuncArgT<index, R(Args...)>
{
	using type = std::tuple_element_t<index, std::tuple<Args...>>;
};
// ���ڳ��� operator() ����
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
 * glfw window ���
 */
	GLFWwindow* pWindow_;

	void createWindow(const uint32_t width, const uint32_t height, const std::string_view title);
	void destroyWindow() noexcept;

	// �׳�glfw�ı�������еĻ���
	static void checkGlfwError();

private:
/*
 * �Ƿ�������֤�㣬����debug
 */
#ifdef NODEBUG
	static constexpr bool enableValiLayer = false;
	static constexpr bool enableDebugOutput = false;
#else
	static constexpr bool enableValiLayer = true;
	static constexpr bool enableDebugOutput = true;
#endif

private:
/*
 * instance, extension, layer���
 * instance: vulkan��ײ�Ķ���һ�н�Դ��instance���洢��������״̬
 */
	// �����ͱ�����һ��ָ�룬������deviceҲ����
	VkInstance instance_;
	// vulkan�еĻص�Ҳ��һ����Դ����Ҫ����
	VkDebugUtilsMessengerEXT debugMessenger_;

	void createInstance(const std::string_view appName);
	void destroyInstance() noexcept;

	// ������п�����Ҫ�ṩ�� instance ����� extension
	// glfw ��Ҫ����չ���� vulkan �봰�ڶԽ�
	// VK_EXT_debug_utils ��չ������չdebug����
	static std::vector<const char*> getInstanceRequiredExtensions();

	static std::vector<const char*> getRequiredLayers();

	// debug ����Ĵ�����
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugHandler(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	//��Ҫ����instance ���ֶ����� debugMessenger ��create��destroy����
	static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

private:
	/*
	 * surface ���
	 */
	VkSurfaceKHR surface_;

	void createSurface();
	void destroySurface() noexcept;

private:
/*
 * device �� queue ���
 * ���� instance �õ����õ�һ�� physical device��
 * �õ����� ��Ҫ�� queueFamily ������������ӳ��
 * ���� logical device
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

	// �õ��ϸ�� physical device ���������Ϣ������֮��� device ����
	void pickPhysicalDevice();
	bool pickCertainPhysicalDevice(VkPhysicalDevice device) noexcept;

	VkDevice device_;
	struct Queues
	{
		VkQueue graphicsQueue;
		VkQueue presentQueue;
	};
	Queues queues_;
	
	// ���� physical ���� logical device ������ family ���� queue
	void createLogicalDevice();
	void destroyLogicalDevice() noexcept;

	// ������п�����Ҫ�ṩ�� physical device ����� extension
	// VK_KHR_SWAPCHAIN_EXTENSION_NAME ��Ӧ����չ����֧�ֽ�����
	static std::vector<const char*> getRequiredDeviceExtensions(VkPhysicalDevice device);


    
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
		// ��Ҫ����openGL������
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// ���øı䴰�ڳߴ�
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
		 * 1. ����appInfo
		 * 2. ����createInfo��ָ��appInfo��
		 * 3. ����createInstance����instance
		*/
	// ����� info �ṹ��� sType ��Ա����Ҫ��ʽ����
	// create ��������allocator����
	// ѡ������֤��

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
			* VkDebugUtilsMessengerCreateInfoEXT: ����Ҫ�ص�����Ϣ���͡��ص�����ָ��
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
	if (instance_ == VK_NULL_HANDLE) return;
	if constexpr (enableValiLayer) {
		destroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
	}
	vkDestroyInstance(instance_, nullptr);
}


void VulkanApplication::pickPhysicalDevice()
{
	std::vector<VkPhysicalDevice> devices = getVkResource(vkEnumeratePhysicalDevices, instance_);
	for(const auto device: devices) {
		if (pickCertainPhysicalDevice(device)) {
			return;
		}
	}
	throw std::runtime_error("can not find suitable physical device");
}

bool VulkanApplication::pickCertainPhysicalDevice(const VkPhysicalDevice device) noexcept
{
/*
 * 1. ��� property
 * 2. ��� feature
 * 3. ��� extension
 * 4. �õ���Ҫ�Ķ�����
 * 5. ��ֵ device features, physical device, queue family indices, device extensions
 */
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	if(deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		return false;
	}

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	if(deviceFeatures.geometryShader != VK_TRUE) {
		return false;
	}

	std::vector<const char*> extensions;
	try {
		extensions = getRequiredDeviceExtensions(device);
	} catch (const std::exception& e) {
		return false;
	}
	std::vector<VkQueueFamilyProperties> queueFamilies = getVkResource(vkGetPhysicalDeviceQueueFamilyProperties, device);

	QueueFamilyIndices queueFamilyIndices {};
	bool graphic = false;
	bool present = false;
	
	for (const auto& [i, queueFamily] : std::views::enumerate(queueFamilies)) {
		if (!graphic && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamilyIndices.graphicsFamily = i;
			graphic = true;
		}
		if(!present) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
			if(presentSupport == VK_TRUE) {
				queueFamilyIndices.presentFamily = i;
				present = true;
			}
		}
		if(graphic && present) {
			break;
		}
	}
	if(!graphic || !present) {
		return false;
	}
	physicalDevice_ = device;
	physicalDeviceFeatures_ = deviceFeatures;
	deviceExtensions_ = extensions;
	queueFamilyIndices_ = queueFamilyIndices;
	return true;
}

void VulkanApplication::createLogicalDevice()
{
	/*
	 * 1. ���� VkDeviceQueueCreateInfo ����, ����ָ�� logic device �еĶ���
	 * 2. ���� queue create info �� deviceFeatures_ ���� device create info
	 *
	 */

	 // һ�� queueCreateInfo ���Դ������������ ��ͬ ������Ķ���
	 // queueCreateInfos �е�ÿ�� queueCreateInfo ��Ҫָ����ͬ�Ķ�����
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

	// �� instance �㲻ͬ�� device �㲻��Ҫ�κ���չ
	createInfo.enabledExtensionCount = 0;

	// ��ʵ���� (instance, physical device) �� (logic device����) ���������в�ͬ�� layer, ������ʵ���кϲ���
	// ������Ҫ���� enabledLayerCount �� ppEnabledLayerNames
	// createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers_.size());
	// createInfo.ppEnabledLayerNames = requiredLayers_.data();

	if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	std::map<uint32_t, uint32_t> family2QueueIndexMap;
	for (const auto index : index2CountMap | std::views::keys) {
		family2QueueIndexMap[index] = 0;
	}
	// Ҫ��ǰ��� indices ���Ӧ
	std::vector<VkQueue Queues::*> pQueues{ &Queues::graphicsQueue, &Queues::presentQueue };
	for(const auto [familyIndex, pQueue]: std::views::zip(indices, pQueues)) {
		auto& queueIndex = family2QueueIndexMap[familyIndex];
		vkGetDeviceQueue(device_, familyIndex, queueIndex, &(queues_.*pQueue));
		queueIndex++;
	}
}

void VulkanApplication::destroyLogicalDevice() noexcept
{
	if (device_ == VK_NULL_HANDLE) return;
	vkDestroyDevice(device_, nullptr);
}

std::vector<const char*> VulkanApplication::getRequiredDeviceExtensions(VkPhysicalDevice device)
{
	std::vector<const char*> requiredExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	/*
	 * ʹ��vkEnumerateInstanceϵ�к��������Ҫ�� extension �Ƿ�֧��
	 */
	std::vector<VkExtensionProperties> availableExtensions = getVkResource(vkEnumerateDeviceExtensionProperties, device, nullptr);

	if constexpr (enableDebugOutput) {
		std::print("the available {} device extensions are:\n", availableExtensions.size());
		for (const auto& [extensionName, specVersion] : availableExtensions) {
			std::print("{} (version {})\n", extensionName, specVersion);
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
	// if (glfwCreateWindowSurface(instance_, pWindow_, nullptr, &surface_) != VK_SUCCESS) {
	// 	throw std::runtime_error("failed to create surface!");
	// }
}

void VulkanApplication::destroySurface() noexcept
{
	if (surface_ == VK_NULL_HANDLE) return;
	vkDestroySurfaceKHR(instance_, surface_, nullptr);
}

VulkanApplication::VulkanApplication(const uint32_t width, const uint32_t height, const std::string_view appName)
{
	pWindow_ = nullptr;
	instance_ = VK_NULL_HANDLE;
	surface_ = VK_NULL_HANDLE;
	device_ = VK_NULL_HANDLE;
	createWindow(width, height, appName);
	createInstance(appName);
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
}

VulkanApplication::~VulkanApplication()
{
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
		std::print("the available {} layers are:\n", availableLayers.size());
		for (const auto& [layerName, specVersion, implementationVersion, description] : availableLayers) {
			std::print("{} (spec version {}, implementation version {}) : {} \n", layerName, specVersion, implementationVersion, description);
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
	if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)return VK_FALSE;
	/*
	 * VkDebugUtilsMessageSeverityFlagBitsEXT : �����ԣ� VERBOSE, INFO, WARNING, ERROR (���ԱȽϣ�Խ����Խ��
	 * VkDebugUtilsMessageTypeFlagsEXT : ���ͣ� GENERAL, VALIDATION, PERFORMANCE
	 * return: �Ƿ�Ҫ��ֹ ������֤����Ϣ �� vulkan����
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
		default: return "OTHER";
		}
	};
	std::print("validation layer: ({},{}) {}\n",
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
	 * ʹ��vkEnumerateInstanceϵ�к��������Ҫ�� extension �Ƿ�֧��
	 */
	std::vector<VkExtensionProperties> availableExtensions = getVkResource(vkEnumerateInstanceExtensionProperties, nullptr);

	if constexpr (enableDebugOutput) {
		std::print("the available {} extensions are:\n", availableExtensions.size());
		for (const auto& [extensionName, specVersion] : availableExtensions) {
			std::print("{} (version {})\n", extensionName, specVersion);
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
        uint32_t width = 1920;
        uint32_t height = 1080;
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
