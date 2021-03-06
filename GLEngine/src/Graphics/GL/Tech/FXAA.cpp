#include "Graphics/GL/Tech/FXAA.h"

#include "Graphics/GL/Tech/QuadDrawer.h"

#include <glm/glm.hpp>

BEGIN_UNNAMED_NAMESPACE()

const char* const FXAA_VERT_SHADER_PATH = "../EngineAssets/Shaders/FXAA/FXAA.vert";
const char* const FXAA_FRAG_SHADER_PATHS[4] = { 
	"../EngineAssets/Shaders/FXAA/FXAA_Fastest.frag", 
	"../EngineAssets/Shaders/FXAA/FXAA_Default.frag", 
	"../EngineAssets/Shaders/FXAA/FXAA_High_Quality.frag", 
	"../EngineAssets/Shaders/FXAA/FXAA_Extreme_Quality.frag" 
};

END_UNNAMED_NAMESPACE()

void FXAA::initialize(EQuality a_quality, uint a_screenWidth, uint a_screenHeight)
{
	uint quality = uint(a_quality);
	m_fxaaShader.initialize(FXAA_VERT_SHADER_PATH, FXAA_FRAG_SHADER_PATHS[quality]);
	m_fxaaShader.begin();
	m_fxaaShader.setUniform2f("RCPFrame", glm::vec2(1.0f / float(a_screenWidth), 1.0f / float(a_screenHeight)));
	m_fxaaShader.end();

	m_fxaaTexture.initialize(GLTexture::ESizedFormat::RGB8, a_screenWidth, a_screenHeight);

	m_fxaaFBO.initialize();
	m_fxaaFBO.addFramebufferTexture(m_fxaaTexture);
}

void FXAA::begin()
{
	m_fxaaFBO.begin();
}

void FXAA::endAndRender()
{
	m_fxaaFBO.end();

	m_fxaaShader.begin();
	m_fxaaFBO.bindTexture(0, 0);
	QuadDrawer::drawQuad(m_fxaaShader);
	m_fxaaShader.end();
}

