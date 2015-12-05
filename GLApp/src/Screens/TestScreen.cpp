#include "Screens/TestScreen.h"

#include "EASTL/string.h"

#include "Components/CameraComponent.h"
#include "Components/FPSControlledComponent.h"
#include "Components/ModelComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyComponent.h"
#include "Components/TransformComponent.h"
#include "Components/UIComponent.h"

#include "GLEngine.h"
#include "Graphics/Graphics.h"

#include "Input/Input.h"

#include "Systems/CameraSystem.h"
#include "Systems/FPSControlSystem.h"
#include "Systems/LightSystem.h"
#include "Systems/ModelManageSystem.h"
#include "Systems/RenderSystem.h"
#include "Systems/UISystem.h"

#include <glm/gtc/random.hpp>

BEGIN_UNNAMED_NAMESPACE()

static const eastl::string MODEL_FILE_PATH("Models/palace/palace.obj.da");
static const eastl::string MODEL2_FILE_PATH("Models/meshes/dragon.obj.da");
static const eastl::string SKYBOX_FILE_PATH("Models/skybox/skysphere.obj.da");
static const eastl::string IFCTEST_FILE_PATH("Models/meshes/ifctest.ifc.da");
static const eastl::string IFC1_FILE_PATH("Models/BIM/ifc2.ifc.da");
static const eastl::string UI_JSON_FILE_PATH("UI/uitest.json");

END_UNNAMED_NAMESPACE()

using namespace entityx;

TestScreen::TestScreen()
{
	uint viewportWidth = GLEngine::graphics->getViewportWidth();
	uint viewportHeight = GLEngine::graphics->getViewportHeight();

	m_keyDownListener.setFunc([this](EKey a_key) { keyDown(a_key); });
	m_windowQuitListener.setFunc([this]() { GLEngine::shutdown(); });

	SystemManager& systems = m_entityx.systems;
	systems.add<ModelManageSystem>();
	systems.add<FPSControlSystem>();
	systems.add<CameraSystem>();
	systems.add<UISystem>();
	systems.add<LightSystem>(*systems.system<CameraSystem>());
	systems.add<RenderSystem>(*systems.system<CameraSystem>(), *systems.system<LightSystem>(), *systems.system<UISystem>());
	systems.configure();

	Entity cameraEntity = m_entityx.entities.create();
	cameraEntity.assign<CameraComponent>((float) viewportWidth, (float) viewportHeight, 90.0f, 0.5f, 1000.0f);
	cameraEntity.assign<TransformComponent>(0.0f, 0.0f, 0.0f);
	cameraEntity.assign<FPSControlledComponent>(10.0f, 0.7f);

	Entity skyboxEntity = m_entityx.entities.create();
	skyboxEntity.assign<SkyComponent>();
	skyboxEntity.assign<ModelComponent>(SKYBOX_FILE_PATH);

	Entity modelEntity = m_entityx.entities.create();
	modelEntity.assign<ModelComponent>(MODEL_FILE_PATH);
	modelEntity.assign<TransformComponent>(0.0f, -10.0f, -70.0f);

	Entity model2Entity = m_entityx.entities.create();
	model2Entity.assign<ModelComponent>(MODEL2_FILE_PATH);
	model2Entity.assign<TransformComponent>(0.0f, -7.0f, -58.0f);

	Entity model3Entity = m_entityx.entities.create();
	model3Entity.assign<ModelComponent>(IFC1_FILE_PATH);
	ComponentHandle<TransformComponent> transform = model3Entity.assign<TransformComponent>(0.0f, 0.0f, 100.0f);
	transform->setRotation(glm::vec3(1, 0, 0), -90.0f);
	transform->scale(glm::vec3(0.0005f));

	for (uint i = 0; i < 30; ++i)
	{
		Entity lightEntity = m_entityx.entities.create();
		glm::vec3 color = glm::normalize(glm::linearRand(glm::vec3(0), glm::vec3(1)));
		float intensity = glm::linearRand(7.0f, 25.0f);
		float radius = glm::linearRand(10.0f, 20.0f);
		glm::vec3 position = glm::linearRand(glm::vec3(-22.0f, -6.0f, -17.0f), glm::vec3(20.0f, 8.0f, -92.0f));
		lightEntity.assign<PointLightComponent>()->set(position, radius, color, intensity);
	}

	Entity uiEntity = m_entityx.entities.create();
	uiEntity.assign<UIComponent>(UI_JSON_FILE_PATH, UIComponent::ELayer::Layer0);
}

TestScreen::~TestScreen()
{
}

void TestScreen::render(float a_deltaSec)
{
	SystemManager& systems = m_entityx.systems;
	systems.update<FPSControlSystem>(a_deltaSec);
	systems.update<CameraSystem>(a_deltaSec);
	systems.update<LightSystem>(a_deltaSec);
	systems.update<RenderSystem>(a_deltaSec);
}

void TestScreen::keyDown(EKey a_key)
{
	SystemManager& systems = m_entityx.systems;

	switch (a_key)
	{
	case EKey::T:
	{
		const PerspectiveCamera* camera = systems.system<CameraSystem>()->getActiveCamera();
		// Place light 1.0f in front of camera position.
		glm::vec3 position = camera->getPosition() + camera->getDirection();
		glm::vec3 color = glm::normalize(glm::linearRand(glm::vec3(0), glm::vec3(1)));
		float intensity = glm::linearRand(7.0f, 25.0f);
		float radius = glm::linearRand(10.0f, 20.0f);

		entityx::Entity lightEntity = m_entityx.entities.create();
		lightEntity.assign<PointLightComponent>()->set(position, radius, color, intensity);
		m_lightEntities.push_back(lightEntity);
		break;
	}
	case EKey::Y:
	{
		if (m_lightEntities.size())
		{
			m_lightEntities.back().destroy();
			m_lightEntities.pop_back();
		}
		break;
	}
	case EKey::G:
	{
		auto renderSystem = systems.system<RenderSystem>();
		renderSystem->setHBAOEnabled(!renderSystem->isHBAOEnabled());
		break;
	}
	case EKey::H:
	{
		glm::vec3 pos = systems.system<CameraSystem>()->getActiveCamera()->getPosition();
		print("CameraPos: %f %f %f\n", pos.x, pos.y, pos.z);
		break;
	}
	case EKey::ESCAPE:
	{
		GLEngine::shutdown();
		break;
	}
	}
}