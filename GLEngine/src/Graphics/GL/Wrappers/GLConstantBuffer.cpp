#include "Graphics/GL/Wrappers/GLConstantBuffer.h"

#include "Graphics/GL/GL.h"
#include "Graphics/GL/Wrappers/GLShader.h"

#include <assert.h>

GLConstantBuffer::~GLConstantBuffer()
{
	if (m_initialized)
		glDeleteBuffers(1, &m_ubo);
}

void GLConstantBuffer::initialize(const GLShader& a_shader, uint a_bindingPoint, const char* a_blockName, EDrawUsage a_drawUsage)
{
	assert(!m_initialized);

	m_shader = &a_shader;
	m_drawUsage = a_drawUsage;
	m_bindingPoint = a_bindingPoint;

	m_uboIndex = glGetUniformBlockIndex(a_shader.getID(), a_blockName);
	if (m_uboIndex == GL_INVALID_INDEX)
	{
		print("Failed to initialize ubo: %s \n", a_blockName);
		return;
	}

	if (m_ubo)
		glDeleteBuffers(1, &m_ubo);

	glGenBuffers(1, &m_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);

	glUniformBlockBinding(a_shader.getID(), m_uboIndex, m_bindingPoint);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_bindingPoint, m_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	m_initialized = true;
}

void GLConstantBuffer::upload(uint a_numBytes, const void* a_data)
{
	assert(m_shader->isBegun());
	assert(m_initialized);
	if (a_numBytes)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, a_numBytes, a_data, (GLenum) m_drawUsage);
	}
}

void GLConstantBuffer::bind()
{
	assert(m_shader->isBegun());
	assert(m_initialized);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_bindingPoint, m_ubo);
}