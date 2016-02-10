#include "GameObject.h"

#include <glm/gtc/matrix_transform.hpp>

void GameObject::render(const GLRenderer& renderer, GLConstantBuffer& modelMatrixUBO, bool depthOnly)
{
	if (m_dirty) 
		updateTransform();
	m_scene->render(m_transform, modelMatrixUBO, depthOnly);
}

void GameObject::updateTransform()
{
	glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(m_scale));
	glm::mat4 rotation = glm::rotate(glm::mat4(1), m_axisRotation.w, glm::vec3(m_axisRotation));
	glm::mat4 position = glm::translate(glm::mat4(1), m_position);
	m_transform = position * rotation * scale;
	m_dirty = false;
}
