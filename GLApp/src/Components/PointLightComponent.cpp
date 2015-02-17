#include "Components/PointLightComponent.h"

#include "Graphics/LightManager.h"

void PointLightComponent::set(const glm::vec3& a_position, float a_radius, const glm::vec3& a_color, float a_intensity)
{
	m_lightManager->setLight(m_lightHandle, a_position, a_radius, a_color, a_intensity);
}

void PointLightComponent::setPosition(const glm::vec3& a_position)
{
	m_lightManager->setLightPosition(m_lightHandle, a_position);
}

void PointLightComponent::setRadius(float a_radius)
{
	m_lightManager->setLightRange(m_lightHandle, a_radius);
}

void PointLightComponent::setColor(const glm::vec3& a_color)
{
	m_lightManager->setLightColor(m_lightHandle, a_color);
}

void PointLightComponent::setIntensity(float a_intensity)
{
	m_lightManager->setLightIntensity(m_lightHandle, a_intensity);
}