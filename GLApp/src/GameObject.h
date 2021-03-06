#pragma once

#include "Graphics/GL/Scene/GLRenderObject.h"
#include "Graphics/GL/Scene/GLScene.h"

class GameObject : public GLRenderObject
{
public:

	GameObject() {}
	virtual ~GameObject() {}

	void initialize(GLScene* a_scene) { m_scene = a_scene; }

	virtual void render(GLRenderer& renderer, bool depthOnly) override;

	void setPosition(const glm::vec3& a_position);
	void setScale(float a_scale);
	void setRotation(const glm::vec3& a_axis, float a_degrees);

	const glm::vec3& getPosition() const     { return m_position; }
	const glm::vec4& getAxisRotation() const { return m_axisRotation; }
	float getScale() const                   { return m_scale; }
	
private:
	
	void updateTransform();

private:

	glm::vec3 m_position     = glm::vec3(0, 0, 0);
	float m_scale            = 1.0f;
	glm::vec4 m_axisRotation = glm::vec4(0, 1, 0, 0);
	bool m_dirty             = true;
	glm::mat4 m_transform    = glm::mat4(1);
	GLScene* m_scene         = NULL;
};