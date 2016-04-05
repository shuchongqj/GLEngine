/*
* Vulkan Example base class
*
* Copyright (C) 2015 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#define VERTEX_BUFFER_BIND_ID 0

#include "Graphics/Vulkan/Test/vulkanexamplebase.h"
#include "Graphics/Vulkan/Wrappers/VKInstance.h"

#include <glm/gtc/matrix_transform.hpp>

bool VulkanExampleBase::checkCommandBuffers()
{
	for (auto& cmdBuffer : m_drawCmdBuffers)
	{
		if (!cmdBuffer.isInitialized())
		{
			return false;
		}
	}
	return true;
}

void VulkanExampleBase::createCommandBuffers()
{
	m_drawCmdBuffers.resize(m_swapchain.getNumImages());
	eastl::for_each(m_drawCmdBuffers.begin(), m_drawCmdBuffers.end(), [&](VKCommandBuffer& a_buffer) {
		a_buffer.initialize(*m_device);
	});
	m_postPresentCmdBuffer.initialize(*m_device);
}

void VulkanExampleBase::destroyCommandBuffers()
{
	m_postPresentCmdBuffer.cleanup();
	eastl::for_each(m_drawCmdBuffers.begin(), m_drawCmdBuffers.end(), [&](VKCommandBuffer& a_buffer) {
		a_buffer.cleanup();
	});
}

void VulkanExampleBase::createPipelineCache()
{
	vk::PipelineCacheCreateInfo pipelineCacheCreateInfo = vk::PipelineCacheCreateInfo();
	pipelineCache = m_device->getVKDevice().createPipelineCache(pipelineCacheCreateInfo, NULL);
}

void VulkanExampleBase::prepare()
{
	if (enableValidation)
		vkDebug::setupDebugging(instance, VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT, NULL);

	VKCommandBuffer setupCmdBuffer;
	setupCmdBuffer.initialize(*m_device);
	setupCmdBuffer.begin();
	m_swapchain.initialize(*m_physDevice, setupCmdBuffer, width, height);
	setupDepthStencil(setupCmdBuffer);
	setupCmdBuffer.end();
	setupCmdBuffer.submit();
	setupCmdBuffer.waitIdle();

	createCommandBuffers();
	
	setupRenderPass();
	createPipelineCache();
	setupFrameBuffer();

	textureLoader = new vkTools::VulkanTextureLoader(physicalDevice, device, queue, cmdPool);

	prepareVertices();
	prepareUniformBuffers();
	setupDescriptorSetLayout();
	preparePipelines();
	setupDescriptorPool();
	setupDescriptorSet();
	buildCommandBuffers();
	prepared = true;
}

VkPipelineShaderStageCreateInfo VulkanExampleBase::loadShader(const char* a_fileName, vk::ShaderStageFlagBits a_stage)
{
	vk::PipelineShaderStageCreateInfo shaderStage = vk::PipelineShaderStageCreateInfo()
		.stage(a_stage)
		.module(vkTools::loadShader(a_fileName, device, a_stage))
		.pName("main");
	assert(shaderStage.module() != NULL);
	shaderModules.push_back(shaderStage.module());
	return shaderStage;
}

VkBool32 VulkanExampleBase::createBuffer(
	VkBufferUsageFlags usage, 
	VkDeviceSize size, 
	void * data, 
	VkBuffer *buffer, 
	VkDeviceMemory *memory)
{
	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
	VkBufferCreateInfo bufferCreateInfo = vkTools::initializers::bufferCreateInfo(usage, size);

	VkResult err = vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer);
	assert(!err);
	vkGetBufferMemoryRequirements(device, *buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAlloc, nullptr, memory);
	assert(!err);
	if (data != nullptr)
	{
		void *mapped;
		err = vkMapMemory(device, *memory, 0, size, 0, &mapped);
		assert(!err);
		memcpy(mapped, data, size);
		vkUnmapMemory(device, *memory);
	}
	err = vkBindBufferMemory(device, *buffer, *memory, 0);
	assert(!err);
	return true;
}

VkBool32 VulkanExampleBase::createBuffer(VkBufferUsageFlags usage, VkDeviceSize size, void * data, VkBuffer * buffer, VkDeviceMemory * memory, VkDescriptorBufferInfo * descriptor)
{
	VkBool32 res = createBuffer(usage, size, data, buffer, memory);
	if (res)
	{
		descriptor->offset = 0;
		descriptor->buffer = *buffer;
		descriptor->range = size;
		return true;
	}
	else
	{
		return false;
	}
}

void VulkanExampleBase::loadMesh(
	const char * filename, 
	vkMeshLoader::MeshBuffer * meshBuffer, 
	std::vector<vkMeshLoader::VertexLayout> vertexLayout, 
	float scale)
{
	VulkanMeshLoader *mesh = new VulkanMeshLoader();
	mesh->LoadMesh(filename);
	assert(mesh->m_Entries.size() > 0);

	mesh->createVulkanBuffers(
		device,
		deviceMemoryProperties,
		meshBuffer,
		vertexLayout,
		scale);

	delete(mesh);
}

void VulkanExampleBase::renderLoop()
{
	viewChanged();
	MSG msg;
	while (TRUE)
	{
		auto tStart = std::chrono::high_resolution_clock::now();
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		if (msg.message == WM_QUIT)
		{
			break;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		render();
		auto tEnd = std::chrono::high_resolution_clock::now();
		auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
		frameTimer = (float)tDiff / 1000.0f;
		// Convert to clamped timer value
		if (!paused)
		{
			timer += timerSpeed * frameTimer;
			if (timer > 1.0)
			{
				timer -= 1.0f;
			}
		}
	}
}

// todo : comment
void VulkanExampleBase::submitPostPresentBarrier(VkImage image)
{
	m_postPresentCmdBuffer.begin();
	VkCommandBuffer postPresentBuffer = m_postPresentCmdBuffer.getVKCommandBuffer();
	VkImageMemoryBarrier postPresentBarrier = vkTools::postPresentBarrier(image);
	vkCmdPipelineBarrier(
		postPresentBuffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, NULL, // No memory barriers,
		0, NULL, // No buffer barriers,
		1, &postPresentBarrier);
	m_postPresentCmdBuffer.end();

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &postPresentBuffer;
	VkResult vkRes = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	assert(!vkRes);
	vkRes = vkQueueWaitIdle(queue);
	assert(!vkRes);
}

VulkanExampleBase::VulkanExampleBase(bool enableValidation)
{
	setupConsole("VulkanExample");

	width = 1280;
	height = 720;
	zoom = -2.5f;
	title = "Vulkan Example - Basic indexed triangle";
	initVulkan(enableValidation);
}

void VulkanExampleBase::initVulkan(bool enableValidation)
{
	m_instance.initialize();
	m_physDevice = &m_instance.getPhysicalDevice();
	m_device = &m_physDevice->getDevice();

	depthFormat = scast<VkFormat>(m_physDevice->getDepthFormat());
	instance = m_instance.getVKInstance();
	physicalDevice = m_physDevice->getVKPhysicalDevice();
	deviceMemoryProperties = m_physDevice->getMemProperties();
	device = m_device->getVKDevice();
	queue = m_device->getVKQueue();

	//swapChain.init(instance, physicalDevice, device);
}

void VulkanExampleBase::setupConsole(std::string title)
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	SetConsoleTitle(TEXT(title.c_str()));
	if (enableValidation)
	{
		std::cout << "Validation enabled:\n";
	}
}

HWND VulkanExampleBase::setupWindow(HINSTANCE hinstance, HWND hwnd)
{
	this->windowInstance = hinstance;
	this->window = hwnd;
	return window;
}

void VulkanExampleBase::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		prepared = false;
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		ValidateRect(window, NULL);
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 0x50:
			paused = !paused;
			break;
		case VK_ESCAPE:
			exit(0);
			break;
		}
		break;
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		mousePos.x = (float)LOWORD(lParam);
		mousePos.y = (float)HIWORD(lParam);
		break;
	case WM_MOUSEMOVE:
		if (wParam & MK_RBUTTON)
		{
			int32_t posx = LOWORD(lParam);
			int32_t posy = HIWORD(lParam);
			zoom += (mousePos.y - (float)posy) * .005f * zoomSpeed;
			mousePos = glm::vec2((float)posx, (float)posy);
			viewChanged();
		}
		if (wParam & MK_LBUTTON)
		{
			int32_t posx = LOWORD(lParam);
			int32_t posy = HIWORD(lParam);
			rotation.x += (mousePos.y - (float)posy) * 1.25f * rotationSpeed;
			rotation.y -= (mousePos.x - (float)posx) * 1.25f * rotationSpeed;
			mousePos = glm::vec2((float)posx, (float)posy);
			viewChanged();
		}
		break;
	}
}

void VulkanExampleBase::viewChanged()
{
	updateUniformBuffers();
}

VkBool32 VulkanExampleBase::getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t * typeIndex)
{
	for (uint32_t i = 0; i < 32; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	return false;
}

void VulkanExampleBase::setupDepthStencil(VKCommandBuffer& a_setupCmdBuffer)
{
	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.pNext = NULL;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = depthFormat;
	image.extent = { width, height, 1 };
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image.flags = 0;

	VkMemoryAllocateInfo mem_alloc = {};
	mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc.pNext = NULL;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	VkImageViewCreateInfo depthStencilView = {};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.pNext = NULL;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = depthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;

	VkMemoryRequirements memReqs;
	VkResult err;

	err = vkCreateImage(device, &image, nullptr, &depthStencil.image);
	assert(!err);
	vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
	mem_alloc.allocationSize = memReqs.size;
	getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &mem_alloc, nullptr, &depthStencil.mem);
	assert(!err);

	err = vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0);
	assert(!err);
	
	vkTools::setImageLayout(a_setupCmdBuffer.getVKCommandBuffer(), depthStencil.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	depthStencilView.image = depthStencil.image;
	err = vkCreateImageView(device, &depthStencilView, nullptr, &depthStencil.view);
	assert(!err);
}

void VulkanExampleBase::setupFrameBuffer()
{
	VkImageView attachments[2];

	attachments[1] = depthStencil.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = renderPass;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = width;
	frameBufferCreateInfo.height = height;
	frameBufferCreateInfo.layers = 1;

	frameBuffers.resize(m_swapchain.getNumImages());
	for (uint32_t i = 0; i < frameBuffers.size(); i++)
	{
		attachments[0] = m_swapchain.getView(i);
		VkResult err = vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]);
		assert(!err);
	}
}

void VulkanExampleBase::setupRenderPass()
{
	VkAttachmentDescription attachments[2];
	attachments[0].format = colorformat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachments[1].format = depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = &depthReference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = NULL;

	VkResult err;

	err = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
	assert(!err);
}

void VulkanExampleBase::setupSwapChain()
{
}

void VulkanExampleBase::buildCommandBuffers()
{
	VkClearValue clearValues[2];
	clearValues[0].color = defaultClearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = NULL;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	for (int32_t i = 0; i < m_drawCmdBuffers.size(); ++i)
	{
		renderPassBeginInfo.framebuffer = frameBuffers[i];
		m_drawCmdBuffers[i].begin();

		vkCmdBeginRenderPass(m_drawCmdBuffers[i].getVKCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {};
		viewport.height = (float)height;
		viewport.width = (float)width;
		viewport.minDepth = (float) 0.0f;
		viewport.maxDepth = (float) 1.0f;
		vkCmdSetViewport(m_drawCmdBuffers[i].getVKCommandBuffer(), 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(m_drawCmdBuffers[i].getVKCommandBuffer(), 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_drawCmdBuffers[i].getVKCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
		vkCmdBindPipeline(m_drawCmdBuffers[i].getVKCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_drawCmdBuffers[i].getVKCommandBuffer(), VERTEX_BUFFER_BIND_ID, 1, &vertices.buf, offsets);
		vkCmdBindIndexBuffer(m_drawCmdBuffers[i].getVKCommandBuffer(), indices.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(m_drawCmdBuffers[i].getVKCommandBuffer(), indices.count, 1, 0, 0, 1);
		vkCmdEndRenderPass(m_drawCmdBuffers[i].getVKCommandBuffer());

		VkImageMemoryBarrier prePresentBarrier = {};
		prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		prePresentBarrier.pNext = NULL;
		prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		prePresentBarrier.dstAccessMask = 0;
		prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		prePresentBarrier.image = m_swapchain.getImage(i);

		VkImageMemoryBarrier *pMemoryBarrier = &prePresentBarrier;
		vkCmdPipelineBarrier(
			m_drawCmdBuffers[i].getVKCommandBuffer(),
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_FLAGS_NONE,
			0, nullptr,
			0, nullptr,
			1, &prePresentBarrier);

		m_drawCmdBuffers[i].end();
	}
}

void VulkanExampleBase::draw()
{
	VkResult err;
	VkSemaphore presentCompleteSemaphore;
	VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {};
	presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	presentCompleteSemaphoreCreateInfo.pNext = NULL;
	presentCompleteSemaphoreCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	err = vkCreateSemaphore(device, &presentCompleteSemaphoreCreateInfo, nullptr, &presentCompleteSemaphore);
	assert(!err);

	m_swapchain.acquireNextImage(presentCompleteSemaphore, currentBuffer);

	VkCommandBuffer drawCmdBuffer = m_drawCmdBuffers[currentBuffer].getVKCommandBuffer();

	m_drawCmdBuffers[currentBuffer].submit(presentCompleteSemaphore);

	vk::SwapchainKHR swapchain = m_swapchain.getVKSwapchain();
	vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
		.swapchainCount(1)
		.pSwapchains(&swapchain)
		.pImageIndices(&currentBuffer);
	m_device->getVKQueue().presentKHR(presentInfo);

	vkDestroySemaphore(device, presentCompleteSemaphore, nullptr);

	VkImageMemoryBarrier postPresentBarrier = {};
	postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	postPresentBarrier.pNext = NULL;
	postPresentBarrier.srcAccessMask = 0;
	postPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	postPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	postPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	postPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	postPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	postPresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	postPresentBarrier.image = m_swapchain.getImage(currentBuffer);

	m_postPresentCmdBuffer.begin();
	vkCmdPipelineBarrier(
		m_postPresentCmdBuffer.getVKCommandBuffer(),
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_FLAGS_NONE,
		0, nullptr,
		0, nullptr,
		1, &postPresentBarrier);
	
	m_postPresentCmdBuffer.end();
	m_postPresentCmdBuffer.submit();
	m_postPresentCmdBuffer.waitIdle();
}

void VulkanExampleBase::prepareVertices()
{
	struct Vertex {
		float pos[3];
		float col[3];
	};
	std::vector<Vertex> vertexBuffer = {
		{ { 1.0f,  1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },
		{ { -1.0f,  1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
		{ { 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
	};
	int vertexBufferSize = int(vertexBuffer.size()) * sizeof(Vertex);

	std::vector<uint32_t> indexBuffer = { 0, 1, 2 };
	int indexBufferSize = int(indexBuffer.size()) * sizeof(uint32_t);

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAlloc.pNext = NULL;
	memAlloc.allocationSize = 0;
	memAlloc.memoryTypeIndex = 0;
	VkMemoryRequirements memReqs;

	VkResult err;
	void *data;

	// Generate vertex buffer
	VkBufferCreateInfo bufInfo = {};
	bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufInfo.pNext = NULL;
	bufInfo.size = vertexBufferSize;
	bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufInfo.flags = 0;
	memset(&vertices, 0, sizeof(vertices));
	err = vkCreateBuffer(device, &bufInfo, nullptr, &vertices.buf);
	assert(!err);
	vkGetBufferMemoryRequirements(device, vertices.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	vkAllocateMemory(device, &memAlloc, nullptr, &vertices.mem);
	assert(!err);
	err = vkMapMemory(device, vertices.mem, 0, memAlloc.allocationSize, 0, &data);
	assert(!err);
	memcpy(data, vertexBuffer.data(), vertexBufferSize);
	vkUnmapMemory(device, vertices.mem);
	assert(!err);
	err = vkBindBufferMemory(device, vertices.buf, vertices.mem, 0);
	assert(!err);

	// Generate index buffer
	VkBufferCreateInfo indexbufferInfo = {};
	indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	indexbufferInfo.pNext = NULL;
	indexbufferInfo.size = indexBufferSize;
	indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	indexbufferInfo.flags = 0;
	memset(&indices, 0, sizeof(indices));
	err = vkCreateBuffer(device, &bufInfo, nullptr, &indices.buf);
	assert(!err);
	vkGetBufferMemoryRequirements(device, indices.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAlloc, nullptr, &indices.mem);
	assert(!err);
	err = vkMapMemory(device, indices.mem, 0, indexBufferSize, 0, &data);
	assert(!err);
	memcpy(data, indexBuffer.data(), indexBufferSize);
	vkUnmapMemory(device, indices.mem);
	err = vkBindBufferMemory(device, indices.buf, indices.mem, 0);
	assert(!err);
	indices.count = int(indexBuffer.size());

	vertices.bindingDescriptions.resize(1);
	vertices.bindingDescriptions[0].binding = VERTEX_BUFFER_BIND_ID;
	vertices.bindingDescriptions[0].stride = sizeof(Vertex);
	vertices.bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertices.attributeDescriptions.resize(2);
	// Location 0 : Position
	vertices.attributeDescriptions[0].binding = VERTEX_BUFFER_BIND_ID;
	vertices.attributeDescriptions[0].location = 0;
	vertices.attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertices.attributeDescriptions[0].offset = 0;
	vertices.attributeDescriptions[0].binding = 0;
	// Location 1 : Color
	vertices.attributeDescriptions[1].binding = VERTEX_BUFFER_BIND_ID;
	vertices.attributeDescriptions[1].location = 1;
	vertices.attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertices.attributeDescriptions[1].offset = sizeof(float) * 3;
	vertices.attributeDescriptions[1].binding = 0;

	vertices.vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertices.vi.pNext = NULL;
	vertices.vi.vertexBindingDescriptionCount = int(vertices.bindingDescriptions.size());
	vertices.vi.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
	vertices.vi.vertexAttributeDescriptionCount = int(vertices.attributeDescriptions.size());
	vertices.vi.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
}

void VulkanExampleBase::setupDescriptorPool()
{
	VkDescriptorPoolSize typeCounts[1];
	typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	typeCounts[0].descriptorCount = 1;
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = NULL;
	descriptorPoolInfo.poolSizeCount = 1;
	descriptorPoolInfo.pPoolSizes = typeCounts;
	descriptorPoolInfo.maxSets = 1;

	VkResult vkRes = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);
	assert(!vkRes);
}

void VulkanExampleBase::setupDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBinding.pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
	descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext = NULL;
	descriptorLayout.bindingCount = 1;
	descriptorLayout.pBindings = &layoutBinding;

	VkResult err = vkCreateDescriptorSetLayout(device, &descriptorLayout, NULL, &descriptorSetLayout);
	assert(!err);

	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = NULL;
	pPipelineLayoutCreateInfo.setLayoutCount = 1;
	pPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

	err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	assert(!err);
}

void VulkanExampleBase::setupDescriptorSet()
{
	VkWriteDescriptorSet writeDescriptorSet = {};

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	VkResult vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
	assert(!vkRes);

	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = descriptorSet;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet.pBufferInfo = &uniformDataVS.descriptor;
	writeDescriptorSet.dstBinding = 0;

	vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);
}

void VulkanExampleBase::preparePipelines()
{
	VkResult err;
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.depthBiasEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
	blendAttachmentState[0].colorWriteMask = 0xf;
	blendAttachmentState[0].blendEnable = VK_FALSE;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = blendAttachmentState;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.dynamicStateCount = int(dynamicStateEnables.size());

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front = depthStencilState.back;

	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.pSampleMask = NULL;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineShaderStageCreateInfo shaderStages[2] = { {},{} };
	shaderStages[0] = loadShader("Assets/Shaders/vulkan/triangle.vert.spv", vk::ShaderStageFlagBits::eVertex);
	shaderStages[1] = loadShader("Assets/Shaders/vulkan/triangle.frag.spv", vk::ShaderStageFlagBits::eFragment);

	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pVertexInputState = &vertices.vi;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.pDynamicState = &dynamicState;

	err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.solid);
	assert(!err);
}

void VulkanExampleBase::prepareUniformBuffers()
{
	VkMemoryRequirements memReqs;
	VkBufferCreateInfo bufferInfo = {};
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = NULL;
	allocInfo.allocationSize = 0;
	allocInfo.memoryTypeIndex = 0;
	VkResult err;

	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(uboVS);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	err = vkCreateBuffer(device, &bufferInfo, nullptr, &uniformDataVS.buffer);
	assert(!err);
	vkGetBufferMemoryRequirements(device, uniformDataVS.buffer, &memReqs);
	allocInfo.allocationSize = memReqs.size;
	getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex);
	err = vkAllocateMemory(device, &allocInfo, nullptr, &(uniformDataVS.memory));
	assert(!err);
	err = vkBindBufferMemory(device, uniformDataVS.buffer, uniformDataVS.memory, 0);
	assert(!err);

	// Store information in the uniform's descriptor
	uniformDataVS.descriptor.buffer = uniformDataVS.buffer;
	uniformDataVS.descriptor.offset = 0;
	uniformDataVS.descriptor.range = sizeof(uboVS);
	updateUniformBuffers();
}

void VulkanExampleBase::updateUniformBuffers()
{
	uboVS.projectionMatrix = glm::perspective(deg_to_rad(60.0f), (float)width / (float)height, 0.1f, 256.0f);
	uboVS.viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));
	uboVS.modelMatrix = glm::mat4();
	uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, deg_to_rad(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, deg_to_rad(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, deg_to_rad(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	uint8_t *pData;
	VkResult err = vkMapMemory(device, uniformDataVS.memory, 0, sizeof(uboVS), 0, (void **)&pData);
	assert(!err);
	memcpy(pData, &uboVS, sizeof(uboVS));
	vkUnmapMemory(device, uniformDataVS.memory);
	assert(!err);
}

void VulkanExampleBase::render()
{
	if (!prepared)
		return;
	vkDeviceWaitIdle(device);
	draw();
	vkDeviceWaitIdle(device);
}