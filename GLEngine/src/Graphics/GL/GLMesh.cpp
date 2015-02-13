#include "Graphics/GL/GLMesh.h"

#include "GLEngine.h"

#include "Graphics/Graphics.h"
#include "Graphics/Pixmap.h"
#include "Graphics/GL/GL.h"
#include "Graphics/GL/Core/GLConstantBuffer.h"
#include "Graphics/GL/Core/GLShader.h"
#include "Graphics/GL/Core/GLStateBuffer.h"
#include "Graphics/GL/Core/GLTextureArray.h"
#include "Graphics/GL/Core/GLVertexBuffer.h"

#include "Utils/CheckGLError.h"
#include "Utils/EResourceType.h"
#include "Utils/FileHandle.h"

#include "Input/Input.h"
#include "Input/Key.h"

#include "rde/rde_string.h"

#include <glm/glm.hpp>

BEGIN_UNNAMED_NAMESPACE()

enum { MAX_MATERIALS = 200 };

#ifdef ANDROID
#define _MAX_PATH 260
#endif

struct MeshEntry
{
	uint meshIndex;
	uint numIndices;
	uint baseVertex;
	uint baseIndex;
	uint materialIndex;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec2 texcoords;
	glm::vec3 normal;
	glm::vec3 tangents;
	glm::vec3 bitangents;
	uint materialID;
};

template <typename T>
uint readVector(const FileHandle& a_handle, rde::vector<T>& a_vector, uint a_offset)
{
	int size;
	a_handle.readBytes(reinterpret_cast<char*>(&size), sizeof(int), a_offset);
	a_vector.resize(size);
	a_handle.readBytes(reinterpret_cast<char*>(&a_vector[0]), size * sizeof(a_vector[0]), a_offset + sizeof(int));

	return a_offset + sizeof(int) + size * sizeof(a_vector[0]);
}

END_UNNAMED_NAMESPACE()

GLMesh::~GLMesh()
{
	SAFE_DELETE(m_indiceBuffer);
	SAFE_DELETE(m_vertexBuffer);
	SAFE_DELETE(m_matUniformBuffer);
}

void GLMesh::loadFromFile(const char* a_filePath, uint a_textureUnit, GLuint a_matUBOBindingPoint)
{
	assert(!m_initialized);

	FileHandle file(a_filePath);
	if (!file.exists())
	{
		print("Mesh file does not exist: %s \n", a_filePath);
		return;
	}

	m_textureUnit = a_textureUnit;
	m_matUBOBindingPoint = a_matUBOBindingPoint;

	rde::vector<uint> indices;
	rde::vector<Vertex> vertices;
	rde::vector<uint> baseIndices;

	int type;
	file.readBytes(reinterpret_cast<char*>(&type), sizeof(int), 0);
	assert(type == EResourceType_MODEL);

	int numAtlases;
	file.readBytes(reinterpret_cast<char*>(&numAtlases), sizeof(int), sizeof(int) * 1);
	file.readBytes(reinterpret_cast<char*>(&m_numTransparentMeshes), sizeof(uint), sizeof(int) * 2);
	uint offset = sizeof(uint) * 3;

	offset = readVector(file, m_matProperties, offset);
	offset = readVector(file, indices, offset);
	offset = readVector(file, vertices, offset);
	file.close();

	m_numIndices = indices.size();

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
		VertexAttribute(0, "Position", VertexAttribute::EFormat::EFormat_FLOAT, 3),
		VertexAttribute(1, "Texcoords", VertexAttribute::EFormat::EFormat_FLOAT, 2),
		VertexAttribute(2, "Normals", VertexAttribute::EFormat::EFormat_FLOAT, 3),
		VertexAttribute(3, "Tangents", VertexAttribute::EFormat::EFormat_FLOAT, 3),
		VertexAttribute(4, "Bitangents", VertexAttribute::EFormat::EFormat_FLOAT, 3),
		VertexAttribute(5, "MaterialID", VertexAttribute::EFormat::EFormat_UNSIGNED_INT, 1)
	};

	m_vertexBuffer->setVertexAttributes(6, attributes);
	m_stateBuffer.end();

	rde::string atlasBasePath(a_filePath);
	atlasBasePath = atlasBasePath.substr(0, atlasBasePath.find_index_of_last('.'));
	atlasBasePath.append("-atlas-");

	rde::vector<Pixmap*> pixmaps(numAtlases);
	int atlasCounter = 0;

	for (int i = 0; i < numAtlases; ++i, ++atlasCounter)
	{
		pixmaps[i] = new Pixmap();
		pixmaps[i]->read(FileHandle(rde::string(atlasBasePath).append(rde::to_string(atlasCounter)).append(".da")));
		assert(pixmaps[i]->exists());
	}

	if (pixmaps.size())
		m_textureArray.initialize(pixmaps);
	assert(m_textureArray.isInitialized());
	m_initialized = true;
}

void GLMesh::uploadMaterialUBO(const GLShader& a_shader)
{
	m_stateBuffer.begin();
	m_matUniformBuffer = new GLConstantBuffer();
	m_matUniformBuffer->initialize(a_shader, m_matUBOBindingPoint, "MaterialProperties", GL_STREAM_DRAW);
	m_matUniformBuffer->upload(m_matProperties.size() * sizeof(m_matProperties[0]), &m_matProperties[0]);
	m_stateBuffer.end();
}

void GLMesh::render(const GLShader& shader, bool a_renderOpague, bool a_renderTransparent, bool a_bindMaterials)
{
	if (!m_matUniformBuffer)
	{
		uploadMaterialUBO(shader);
	}

	m_stateBuffer.begin();
	{
		if (m_textureArray.isInitialized())
			m_textureArray.bind(m_textureUnit);
		
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, NULL);
	}
	m_stateBuffer.end();
}