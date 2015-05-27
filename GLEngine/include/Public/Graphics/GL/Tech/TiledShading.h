#pragma once

#include "Core.h"
#include "rde/vector.h"
#include "Utils/VecForward.h"

#include "Graphics/GL/Wrappers/GLConstantBuffer.h"

class PerspectiveCamera;
class GLShader;

class TiledShading
{
public:
	TiledShading() {}
	~TiledShading() {}

	void resize(uint pixelsPerTileW, uint pixelsPerTileH, uint screenWidth, uint screenHeight, const PerspectiveCamera& camera);
	void setupShader(const GLShader& shader);
	void update(const PerspectiveCamera& camera, uint numLights, const glm::vec4* viewspaceLightPositionRangeList);

	uint getGridWidth()  { return m_gridWidth; }
	uint getGridHeight() { return m_gridHeight; }

private:

	uint m_screenWidth    = 0;
	uint m_screenHeight   = 0;
	uint m_gridWidth      = 0;
	uint m_gridHeight     = 0;
	uint m_gridSize       = 0;
	uint m_pixelsPerTileW = 0;
	uint m_pixelsPerTileH = 0;

	rde::vector<glm::uvec2> m_lightGrid;
	rde::vector<rde::vector<ushort>> m_tileLightIndices;
	rde::vector<uint> m_lightIndices;

	GLConstantBuffer m_lightIndiceBuffer;
	GLConstantBuffer m_lightGridBuffer;
};