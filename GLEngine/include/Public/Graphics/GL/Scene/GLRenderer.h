#pragma once

#include "Graphics/GL/Wrappers/GLFramebuffer.h"
#include "Graphics/GL/Wrappers/GLConstantBuffer.h"
#include "Graphics/GL/Wrappers/GLTexture.h"
#include "Graphics/GL/Wrappers/GLShader.h"
#include "Graphics/GL/Tech/ClusteredShading.h"
#include "Graphics/GL/Tech/HBAO.h"
#include "Graphics/GL/Tech/FXAA.h"
#include "Graphics/GL/Tech/Bloom.h"
#include "Graphics/GL/Tech/CubeMapGen.h"
#include "Graphics/Utils/PerspectiveCamera.h"

#include "EASTL/vector.h"

#include <glm/glm.hpp>

class LightManager;
class GLRenderObject;

class GLRenderer
{
public:

	struct ModelMatrixData
	{
		glm::mat4 u_modelMatrix;
	};

	struct CameraVarsData
	{
		glm::mat4 u_vpMatrix;
		glm::mat4 u_viewMatrix;
		glm::mat4 u_normalMatrix;
		glm::vec3 u_eyePos;
		float u_camNear;
		float u_camFar;
	};

	struct LightingGlobalsData
	{
		glm::vec3 u_ambient;
		float padding_LightningGlobals;
		glm::vec3 u_sunDir;
		float padding2_LightingGlobals;
		glm::vec4 u_sunColorIntensity;
		glm::mat4 u_shadowMat;
	};

	struct SettingsGlobalsData
	{
		int u_hbaoEnabled;
		int u_bloomEnabled;
		int u_shadowsEnabled;
	};

public:

	GLRenderer() {}
	GLRenderer(const GLRenderer& copy) = delete;

	void initialize(const PerspectiveCamera& camera);
	void render(const PerspectiveCamera& camera, const LightManager& lightManager);

	void addRenderObject(GLRenderObject* renderObject);
	void removeRenderObject(GLRenderObject* renderObject);
	void addSkybox(GLRenderObject* renderObject);
	void removeSkybox(GLRenderObject* renderObject);

	void reloadShaders();

	void setSun(const glm::vec3& direction, const glm::vec3& color, float intensity);

	void setHBAOEnabled(bool a_enabled);
	bool isHBAOEnabled() const { return m_hbaoEnabled; }

	void setBloomEnabled(bool a_enabled);
	bool isBloomEnabled() const { return m_bloomEnabled; }

	void setShadowsEnabled(bool a_enabled);
	bool isShadowsEnabled() const { return m_shadowsEnabled; }

	void setDepthPrepassEnabled(bool a_enabled) { m_depthPrepassEnabled = a_enabled; }
	bool isDepthPrepassEnabled() const { return m_depthPrepassEnabled; }

	void setFXAAEnabled(bool a_enabled) { m_fxaaEnabled = a_enabled; }
	bool isFXAAEnabled() const { return m_fxaaEnabled; }

	const PerspectiveCamera* getSceneCamera() const { return m_sceneCamera; }

private:

	void updateLightingGlobalsUBO(const PerspectiveCamera& camera);
	void updateCameraDataUBO(const PerspectiveCamera& camera);
	void updateSettingsGlobalsUBO();

private:

	eastl::vector<GLRenderObject*> m_renderObjects;
	eastl::vector<GLRenderObject*> m_skyboxObjects;

	bool m_hbaoEnabled         = true;
	bool m_fxaaEnabled         = false;
	bool m_bloomEnabled        = true;
	bool m_shadowsEnabled      = true;
	bool m_depthPrepassEnabled = false;
	//bool m_cubeMapGenerated  = false;

	HBAO m_hbao;
	FXAA m_fxaa;
	Bloom m_bloom;
	ClusteredShading m_clusteredShading;
	//CubeMapGen m_cubeMapGenerator;

	GLShader m_depthPrepassShader;
	GLShader m_skyboxShader;
	GLShader m_modelShader;
	GLShader m_combineShader;

	GLFramebuffer m_sceneFBO;
	GLFramebuffer m_shadowFBO;
	PerspectiveCamera m_shadowCamera;
	const PerspectiveCamera* m_sceneCamera = NULL;

	GLTexture m_dfvTexture;

	GLConstantBuffer m_modelMatrixUBO;
	GLConstantBuffer m_cameraVarsUBO;
	GLConstantBuffer m_lightningGlobalsUBO;
	GLConstantBuffer m_settingsGlobalsUBO;

	glm::vec3 m_sunDir;
	glm::vec4 m_sunColorIntensity;
};