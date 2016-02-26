#pragma once

#include "Core.h"
#include "Graphics/Vulkan/vk_cpp.h"

class VKDepthStencil
{
public:

	~VKDepthStencil();
	void initialize();
	void cleanup();

	bool isInitialized() const { return m_initialized; }

private:

	bool m_initialized = false;

};