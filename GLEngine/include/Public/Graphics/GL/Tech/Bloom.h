#pragma once

#include "Graphics/GL/Wrappers/GLFramebuffer.h"
#include "Graphics/GL/Wrappers/GLShader.h"
#include "Graphics/GL/Tech/GaussianBlur.h"

class Bloom
{
public:

	Bloom() {}
	Bloom(const Bloom& copy) = delete;

	void initialize(uint xRes, uint yRes);
	void reloadShader();
	GLFramebuffer& getBloomResultFBO(GLFramebuffer& sceneFBO);

private:

	bool m_initialized = false;
	uint m_xRes        = 0;
	uint m_yRes        = 0;

	GLShader m_bloomShader;
	GLTexture m_bloomResultTexture;
	GLFramebuffer m_bloomResultFBO;
	GaussianBlur m_gaussianBlur;
};