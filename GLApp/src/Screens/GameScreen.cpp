#include "GameScreen.h"

#include "..\Core\ScreenManager.h"
#include "GLEngine.h"
#include "Graphics\Graphics.h"
#include "Input\Input.h"

#include "Graphics\PerspectiveCamera.h"

#include "Graphics\GL\GLAppVars.h"

#include "Utils\FileHandle.h"
#include "Utils\FPSCameraController.h"
#include "Utils\CheckGLError.h"
#include "Utils\FileModificationManager.h"

#include "rde\vector.h"
#include "rde\rde_string.h"

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>

PerspectiveCamera camera;
FPSCameraController cameraController;

static const unsigned int MAX_LIGHTS = 1024;
static const unsigned int TILE_WIDTH_PX = 32;
static const unsigned int TILE_HEIGHT_PX = 32;
static const unsigned int NUM_TEXTURE_ARRAYS = 16;

GameScreen::GameScreen(ScreenManager* a_screenManager) : IScreen(a_screenManager), m_lightManager(MAX_LIGHTS)
{
	CHECK_GL_ERROR();

	GLEngine::input->registerKeyListener(&cameraController);
	GLEngine::input->registerMouseListener(&cameraController);
	GLEngine::input->registerKeyListener(this);
	GLEngine::input->setMouseCaptured(true);

	camera.initialize(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), (float) GLEngine::graphics->getScreenWidth(), (float) GLEngine::graphics->getScreenHeight(), 90.0f, 0.5f, 1500.0f);
	cameraController.initialize(camera, glm::vec3(0, 0, 1));

	m_clusteredShading.initialize(TILE_WIDTH_PX, TILE_HEIGHT_PX, GLEngine::graphics->getViewport(), camera);

	rde::vector<rde::string> extensions;
	rde::vector<rde::string> defines;

	if (rde::string(GLEngine::graphics->getVendorStr()).find("NVIDIA") != rde::string::npos
		&& GLEngine::graphics->getGLMajorVersion() >= 4
		&& GLEngine::graphics->getGLMinorVersion() >= 2)
	{
		defines.push_back(rde::string("DYNAMIC_INDEXING"));
	}

	defines.push_back(rde::string("MAX_LIGHTS ").append(rde::to_string(MAX_LIGHTS)));
	defines.push_back(rde::string("LIGHT_GRID_WIDTH ").append(rde::to_string(m_clusteredShading.getGridWidth())));
	defines.push_back(rde::string("LIGHT_GRID_HEIGHT ").append(rde::to_string(m_clusteredShading.getGridHeight())));
	defines.push_back(rde::string("LIGHT_GRID_DEPTH ").append(rde::to_string(m_clusteredShading.getGridDepth())));
	defines.push_back(rde::string("LIGHT_GRID_TILE_WIDTH ").append(rde::to_string(TILE_WIDTH_PX)));
	defines.push_back(rde::string("LIGHT_GRID_TILE_HEIGHT ").append(rde::to_string(TILE_HEIGHT_PX)));

#ifdef ANDROID
	rde::string versionStr("300 es");
	defines.push_back(rde::string("GL_ES"));
#else
	rde::string versionStr("330 core");
#endif
	m_modelShader.initialize("Shaders/modelshader.vert", "Shaders/modelshader.frag", versionStr, &defines, &extensions);
	CHECK_GL_ERROR();

	m_modelShader.begin();
	//m_modelShader.setUniform3f("u_ambient", glm::vec3(0.2f));
	m_modelShader.end();
		
	m_modelShader.begin();
	m_modelShader.setUniform1i("u_dfvTexture", GLAppVars::TextureUnits_DFV_TEXTURE);
	m_modelShader.setUniform1i("u_1cTextureArray", GLAppVars::TextureUnits_MODEL_1_COMPONENT_TEXTURE_ARRAY);
	m_modelShader.setUniform1i("u_3cTextureArray", GLAppVars::TextureUnits_MODEL_3_COMPONENT_TEXTURE_ARRAY);
	m_mesh.loadFromFile("Models/palace/palace.da", m_modelShader, 
		GLAppVars::TextureUnits_MODEL_1_COMPONENT_TEXTURE_ARRAY,
		GLAppVars::TextureUnits_MODEL_3_COMPONENT_TEXTURE_ARRAY,
		GLAppVars::UBOBindingPoints_MODEL_MATERIAL_UBO_BINDING_POINT);

	CHECK_GL_ERROR();

	m_lightPositionRangeBuffer.initialize(m_modelShader, GLAppVars::UBOBindingPoints_LIGHT_POSITION_RANGE_UBO_BINDING_POINT, "LightPositionRanges", GL_STREAM_DRAW);
	m_lightColorBuffer.initialize(m_modelShader, GLAppVars::UBOBindingPoints_LIGHT_COLOR_UBO_BINDING_POINT, "LightColors", GL_STREAM_DRAW);
	m_lightGridBuffer.initialize(m_modelShader, "u_lightGrid", GLAppVars::TextureUnits_CLUSTERED_LIGHTING_GRID_TEXTURE, GL_RG32UI, GL_STREAM_DRAW);
	m_lightIndiceBuffer.initialize(m_modelShader, "u_lightIndices", GLAppVars::TextureUnits_CLUSTERED_LIGHTING_LIGHT_ID_TEXTURE, GL_R16UI, GL_STREAM_DRAW);

	m_recLogSD1Uniform.initialize(m_modelShader, "u_recLogSD1");
	m_recNearUniform.initialize(m_modelShader, "u_recNear");
	CHECK_GL_ERROR();

	m_modelShader.end();

	m_dfvTexture.initialize("Utils/ggx-helper-dfv.da", GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
}

GameScreen::~GameScreen()
{

}

void GameScreen::render(float a_deltaSec)
{
	GLEngine::graphics->clear(glm::vec4(0.4f, 0.7f, 1.0f, 1.0f));

	cameraController.update(a_deltaSec);
	camera.update();

	m_dfvTexture.bind(GLAppVars::TextureUnits_DFV_TEXTURE);
	m_modelShader.begin();
	{
		const glm::vec4* viewspaceLightPositionRanges = m_lightManager.updateViewspaceLightPositionRangeList(camera);
		m_clusteredShading.update(camera, m_lightManager.getNumLights(), viewspaceLightPositionRanges);

		m_lightGridBuffer.upload(m_clusteredShading.getGridSize() * sizeof(glm::uvec2), m_clusteredShading.getLightGrid());
		m_lightIndiceBuffer.upload(m_clusteredShading.getNumLightIndices() * sizeof(ushort), m_clusteredShading.getLightIndices());
		m_lightGridBuffer.bind();
		m_lightIndiceBuffer.bind();
		m_recLogSD1Uniform.set(m_clusteredShading.getRecLogSD1());
		m_recNearUniform.set(m_clusteredShading.getRecNear());

		m_lightPositionRangeBuffer.upload(m_lightManager.getNumLights() * sizeof(glm::vec4), viewspaceLightPositionRanges);
		m_lightColorBuffer.upload(m_lightManager.getNumLights() * sizeof(glm::vec4), m_lightManager.getLightColors());

		m_modelShader.setUniform3f("u_eyePos", glm::vec3(camera.m_viewMatrix * glm::vec4(camera.m_position, 1.0f)));
		m_modelShader.setUniformMatrix4f("u_mv", camera.m_viewMatrix);
		m_modelShader.setUniformMatrix4f("u_mvp", camera.m_combinedMatrix);
		m_modelShader.setUniformMatrix3f("u_normalMat", glm::mat3(glm::inverse(glm::transpose(camera.m_viewMatrix))));

		m_modelShader.setUniformMatrix4f("u_transform", glm::mat4(1));
		m_mesh.render();
	}
	m_modelShader.end();
	FileModificationManager::update();
	CHECK_GL_ERROR();
	
	GLEngine::graphics->swap();
}

void GameScreen::show(uint a_width, uint a_height)
{

}

void GameScreen::resize(uint a_width, uint a_height)
{

}

void GameScreen::hide()
{

}

bool GameScreen::keyDown(Key a_key)
{
	switch (a_key)
	{
	case Key_T:
		{
			m_lightManager.createLight(
				camera.m_position,
				glm::normalize(glm::vec3(
				(rand() % 1000) / 1000.0f,
				(rand() % 1000) / 1000.0f,
				(rand() % 1000) / 1000.0f)),
				15.0f);
			return true;
		}
	case Key_ESCAPE:
		{
			m_screenManager->quit();
			return true;
		}
	}
	return false;
}
