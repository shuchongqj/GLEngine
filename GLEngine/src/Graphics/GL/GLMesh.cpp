#include "Graphics\GL\GLMesh.h"

#include "GLEngine.h"
#include "Graphics\GL\GL.h"
#include "Graphics\Graphics.h"

#include "Graphics\Pixmap.h"
#include "Graphics\GL\Core\GLShader.h"
#include "Graphics\GL\Core\GLTextureArray.h"
#include "Graphics\GL\Core\GLVertexBuffer.h"
#include "Graphics\GL\Core\GLConstantBuffer.h"
#include "Graphics\GL\Core\GLStateBuffer.h"

#include "Utils\FileHandle.h"
#include "Utils\CheckGLError.h"
#include "Utils\ResourceType.h"

#include "rde\rde_string.h"
#include "Input\Input.h"
#include "Input\Key.h"

#include <glm\glm.hpp>

enum { MAX_MATERIALS = 200 };

#ifdef ANDROID
#define _MAX_PATH 260
#endif

struct MeshEntry
{
	unsigned int meshIndex;
	unsigned int numIndices;
	unsigned int baseVertex;
	unsigned int baseIndex;
	unsigned int materialIndex;
};
struct Vertex
{
	glm::vec3 position;
	glm::vec2 texcoords;
	glm::vec3 normal;
	glm::vec3 tangents;
	glm::vec3 bitangents;
	unsigned int materialID;
};

GLMesh::GLMesh() : m_numOpagueMeshes(0), m_initialized(false)
{
}

GLMesh::~GLMesh()
{
	if (m_indiceBuffer)
		delete m_indiceBuffer;
	if (m_vertexBuffer)
		delete m_vertexBuffer;
	if (m_matUniformBuffer)
		delete m_matUniformBuffer;
}

template <typename T>
uint readVector(const FileHandle& handle, rde::vector<T>& vector, uint offset)
{
	int size;
	handle.readBytes(reinterpret_cast<char*>(&size), sizeof(int), offset);
	vector.resize(size);
	handle.readBytes(reinterpret_cast<char*>(&vector[0]), size * sizeof(vector[0]), offset + sizeof(int));

	return offset + sizeof(int) + size * sizeof(vector[0]);
}

void GLMesh::loadFromFile(const char* filePath, GLShader& shader, uint textureArray1CUnit, uint textureArray3CUnit, GLuint matUBOBindingPoint)
{
	assert(!m_initialized);

	FileHandle file(filePath);
	if (!file.exists())
	{
		print("Mesh file does not exist: %s \n", filePath);
		return;
	}

	m_1cTextureUnit = textureArray1CUnit;
	m_3cTextureUnit = textureArray3CUnit;

	rde::vector<uint> indices;
	rde::vector<Vertex> vertices;
	rde::vector<uint> baseIndices;

	int type;
	file.readBytes(reinterpret_cast<char*>(&type), sizeof(int), 0);
	assert(type == ResourceType_MODEL);

	int num1CompAtlasses, num3CompAtlasses;
	file.readBytes(reinterpret_cast<char*>(&num1CompAtlasses), sizeof(int), sizeof(int));
	file.readBytes(reinterpret_cast<char*>(&num3CompAtlasses), sizeof(int), sizeof(int) * 2);
	file.readBytes(reinterpret_cast<char*>(&m_numOpagueMeshes), sizeof(uint), sizeof(int) * 3);
	uint offset = sizeof(uint) * 4;

	offset = readVector(file, m_indiceCounts, offset);
	offset = readVector(file, baseIndices, offset);
	offset = readVector(file, m_matProperties, offset);
	offset = readVector(file, indices, offset);
	offset = readVector(file, vertices, offset);
	file.close();

	m_numIndices = indices.size();

	uint numBaseIndices = baseIndices.size();
	m_baseIndices.resize(numBaseIndices);
	for (uint i = 0; i < numBaseIndices; ++i)
	{
		m_baseIndices[i] = (GLvoid*) baseIndices[i];
	}

	m_stateBuffer.initialize();
	m_stateBuffer.begin();

	m_indiceBuffer = new GLVertexBuffer();
	m_indiceBuffer->initialize(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
	m_indiceBuffer->upload(sizeof(indices[0]) * indices.size(), &indices[0]);

	m_vertexBuffer = new GLVertexBuffer();
	m_vertexBuffer->initialize(GL_ARRAY_BUFFER, GL_STATIC_DRAW);
	m_vertexBuffer->upload(sizeof(vertices[0]) * vertices.size(), &vertices[0]);

	VertexAttribute attributes[] =
	{
		VertexAttribute(0, "Position", VertexAttribute::Format::FLOAT, 3),
		VertexAttribute(1, "Texcoords", VertexAttribute::Format::FLOAT, 2),
		VertexAttribute(2, "Normals", VertexAttribute::Format::FLOAT, 3),
		VertexAttribute(3, "Tangents", VertexAttribute::Format::FLOAT, 3),
		VertexAttribute(4, "Bitangents", VertexAttribute::Format::FLOAT, 3),
		VertexAttribute(5, "MaterialID", VertexAttribute::Format::UNSIGNED_INT, 1)
	};

	m_vertexBuffer->setVertexAttributes(6, attributes);
	m_stateBuffer.end();

	GLTextureManager& textureManager = GLEngine::graphics->getTextureManager();

	rde::string atlasBasePath(filePath);
	atlasBasePath = atlasBasePath.substr(0, atlasBasePath.find_index_of_last('.'));
	atlasBasePath.append("-atlas-");

	rde::vector<Pixmap*> pixmaps1c(num1CompAtlasses), pixmaps3c(num3CompAtlasses);
	int atlasCounter = 0;

	for (int i = 0; i < num1CompAtlasses; ++i, ++atlasCounter)
	{
		pixmaps1c[i] = new Pixmap();
		pixmaps1c[i]->read(FileHandle(rde::string(atlasBasePath).append(rde::to_string(atlasCounter)).append(".da")));
		assert(pixmaps1c[i]->exists());
	}
	for (int i = 0; i < num3CompAtlasses; ++i, ++atlasCounter)
	{
		pixmaps3c[i] = new Pixmap();
		pixmaps3c[i]->read(FileHandle(rde::string(atlasBasePath).append(rde::to_string(atlasCounter)).append(".da")));
		assert(pixmaps3c[i]->exists());
	}

	if (pixmaps1c.size())
		m_1cTextureArray.initialize(pixmaps1c);
	if (pixmaps3c.size())
		m_3cTextureArray.initialize(pixmaps3c);

	m_stateBuffer.begin();

	m_matUniformBuffer = new GLConstantBuffer();
	m_matUniformBuffer->initialize(shader, matUBOBindingPoint, "MaterialProperties", GL_STREAM_DRAW);
	m_matUniformBuffer->upload(m_matProperties.size() * sizeof(m_matProperties[0]), &m_matProperties[0]);
	m_matUBOBindingPoint = matUBOBindingPoint;

	m_stateBuffer.end();

	m_initialized = true;
}

void GLMesh::render(bool renderOpague, bool renderTransparent, bool bindMaterials)
{
	m_stateBuffer.begin();
	{
		if (m_1cTextureArray.isInitialized())
			m_1cTextureArray.bind(m_1cTextureUnit);
		if (m_3cTextureArray.isInitialized())
			m_3cTextureArray.bind(m_3cTextureUnit);
		glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, NULL);
	}
	m_stateBuffer.end();
}