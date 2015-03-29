#include "Graphics/GL/Tech/TiledShading.h"

#include "Graphics/PerspectiveCamera.h"
#include "Graphics/GL/GL.h"
#include "Graphics/GL/Wrappers/GLShader.h"
#include "Utils/sphereToScreenSpaceBounds.h"

#include <assert.h>
#include <glm/glm.hpp>

void TiledShading::resize(uint a_pixelsPerTileW, uint a_pixelsPerTileH, uint a_screenWidth, uint a_screenHeight, const PerspectiveCamera& a_camera)
{
	m_screenWidth = a_screenWidth;
	m_screenHeight = a_screenHeight;
	m_pixelsPerTileW = a_pixelsPerTileW;
	m_pixelsPerTileH = a_pixelsPerTileH;

	m_gridWidth = a_screenWidth / a_pixelsPerTileW;
	m_gridHeight = a_screenHeight / a_pixelsPerTileH;

	m_gridSize = m_gridWidth * m_gridHeight;

	m_lightGrid.resize(m_gridSize);
	m_tileLightIndices.resize(m_gridSize);
}

void TiledShading::setupShader(const GLShader& a_shader)
{
	assert(a_shader.isBegun());

	m_lightGridBuffer.initialize(a_shader, 0, "LightGrid", GLConstantBuffer::EDrawUsage::STREAM);
	m_lightIndiceBuffer.initialize(a_shader, 1, "LightIndices", GLConstantBuffer::EDrawUsage::STREAM);
}

void TiledShading::update(const PerspectiveCamera& a_camera, uint a_numLights, const glm::vec4* a_viewspaceLightPositionRangeList)
{
	memset(&m_lightGrid[0], 0, m_gridSize * sizeof(m_lightGrid[0]));
	for (auto& tileIndices : m_tileLightIndices)
		tileIndices.clear();
	m_lightIndices.clear();

	for (ushort i = 0; i < a_numLights; ++i)
	{
		glm::vec4 lightPositionRange = a_viewspaceLightPositionRangeList[i];
		float radius = lightPositionRange.w;
		glm::vec3 lightPosition = glm::vec3(lightPositionRange);

		IBounds2D bounds2D = sphereToScreenSpaceBounds2D(a_camera, lightPosition, radius, m_screenWidth, m_screenHeight);

		bounds2D.minX = glm::clamp(bounds2D.minX, 0, (int) m_gridWidth);
		bounds2D.maxX = glm::clamp(bounds2D.maxX, 0, (int) m_gridWidth);
		bounds2D.minY = glm::clamp(bounds2D.minY, 0, (int) m_gridHeight);
		bounds2D.maxY = glm::clamp(bounds2D.maxY, 0, (int) m_gridHeight);

		for (int x = bounds2D.minX; x < bounds2D.maxX; ++x)
		{
			for (int y = bounds2D.minY; y < bounds2D.maxY; ++y)
			{
				uint gridIdx = x * m_gridHeight + y;
				m_tileLightIndices[gridIdx].push_back(i);
			}
		}
	}

	for (int i = 0; i < m_tileLightIndices.size(); ++i)
	{
		int lightIndicesSize = m_lightIndices.size();
		m_lightGrid[i].x = lightIndicesSize;
		m_lightGrid[i].y = lightIndicesSize + m_tileLightIndices[i].size();
		for (int j = 0; j < m_tileLightIndices[i].size(); ++j)
		{
			m_lightIndices.push_back(m_tileLightIndices[i][j]);
		}
	}
	if (m_lightIndices.size() > 0)
	{
		m_lightIndiceBuffer.upload(m_lightIndices.size() * sizeof(m_lightIndices[0]), &m_lightIndices[0]);
		m_lightIndiceBuffer.bind();
	}
	m_lightGridBuffer.upload(m_lightGrid.size() * sizeof(m_lightGrid[0]), &m_lightGrid[0]);
	m_lightGridBuffer.bind();
}