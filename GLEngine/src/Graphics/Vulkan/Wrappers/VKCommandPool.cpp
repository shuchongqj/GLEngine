#include "Graphics/Vulkan/Wrappers/VKCommandPool.h"
#include "Graphics/Vulkan/Utils/VKVerifier.h"

VKCommandPool::~VKCommandPool()
{
	if (m_initialized)
		cleanup();
}

void VKCommandPool::initialize(vk::Device a_device, uint a_queueNodeIndex)
{
	if (m_initialized)
		cleanup();
	vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
		.queueFamilyIndex(a_queueNodeIndex)
		.flags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

	VKVerifier result = vk::createCommandPool(a_device, &commandPoolCreateInfo, NULL, &m_pool);

	m_initialized = true;
}

void VKCommandPool::cleanup()
{
	assert(m_initialized);

	assert(false); //TODO

	m_initialized = false;
}
