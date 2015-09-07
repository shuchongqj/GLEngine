#include "Graphics/GL/GLMesh.h"

#include "GLEngine.h"

#include "Graphics/Graphics.h"
#include "Graphics/Pixmap.h"
#include "Graphics/GL/GL.h"

#include "Utils/EResourceType.h"
#include "Utils/FileHandle.h"
#include "Utils/readVector.h"

#include "Input/Input.h"
#include "Input/EKey.h"

#include "3rdparty/rde/rde_string.h"

#include <glm/glm.hpp>

BEGIN_UNNAMED_NAMESPACE()

static const glm::vec4 NO_TEX_COLOR(0.8f, 0.8f, 0.8f, 1.0f);

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

byte floatToByteCol(float f)
{
	return (byte) floor(f >= 1.0 ? 255 : f * 255.0);
}

END_UNNAMED_NAMESPACE()

void GLMesh::loadFromFile(const char* a_filePath, uint a_textureUnit, uint a_matUBOBindingPoint)
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
	assert(type == (int) EResourceType::MODEL);

	int numAtlases;
	file.readBytes(reinterpret_cast<char*>(&numAtlases), sizeof(int), sizeof(int) * 1);
	uint offset = sizeof(uint) * 2;

	offset = readVector(file, m_matProperties, offset);
	offset = readVector(file, indices, offset);
	offset = readVector(file, vertices, offset);
	file.close();

	m_numIndices = indices.size();

	m_stateBuffer.initialize();
	m_stateBuffer.begin();

	m_indiceBuffer.initialize(GLVertexBuffer::EBufferType::ELEMENT_ARRAY, GLVertexBuffer::EDrawUsage::STATIC);
	m_indiceBuffer.upload(sizeof(indices[0]) * indices.size(), &indices[0]);

	m_vertexBuffer.initialize(GLVertexBuffer::EBufferType::ARRAY, GLVertexBuffer::EDrawUsage::STATIC);
	m_vertexBuffer.upload(sizeof(vertices[0]) * vertices.size(), &vertices[0]);

	VertexAttribute attributes[] =
	{
		VertexAttribute(0, "Position", VertexAttribute::EFormat::FLOAT, 3),
		VertexAttribute(1, "Texcoords", VertexAttribute::EFormat::FLOAT, 2),
		VertexAttribute(2, "Normals", VertexAttribute::EFormat::FLOAT, 3),
		VertexAttribute(3, "Tangents", VertexAttribute::EFormat::FLOAT, 3),
		VertexAttribute(4, "Bitangents", VertexAttribute::EFormat::FLOAT, 3),
		VertexAttribute(5, "MaterialID", VertexAttribute::EFormat::UNSIGNED_INT, 1)
	};

	m_vertexBuffer.setVertexAttributes(ARRAY_SIZE(attributes), attributes);
	m_stateBuffer.end();

	if (numAtlases)
	{
		rde::string atlasBasePath(a_filePath);
		atlasBasePath = atlasBasePath.substr(0, atlasBasePath.find_index_of_last('.'));
		atlasBasePath.append("-atlas-");

		rde::vector<rde::string> atlasImagePaths;
		int atlasCounter = 0;
		for (int i = 0; i < numAtlases; ++i, ++atlasCounter)
			atlasImagePaths.push_back(rde::string(atlasBasePath).append(rde::to_string(atlasCounter)).append(".da"));
		m_textureArray.initialize(atlasImagePaths);
		assert(m_textureArray.isInitialized());
	}
	else
	{	// If the model doesnt have any textures, just create a 2x2 atlas
		rde::vector<Pixmap> pixmaps(1);
		pixmaps[0].set(2, 2, 3, NO_TEX_COLOR);
		m_textureArray.initialize(pixmaps, 0, GLTextureArray::ETextureMinFilter::NEAREST, GLTextureArray::ETextureMagFilter::NEAREST);
		assert(m_textureArray.isInitialized());
	}

	m_initialized = true;
}

void GLMesh::initializeUBO(const GLShader& a_shader)
{
	m_stateBuffer.begin();
	m_matUniformBuffer.initialize(a_shader, m_matUBOBindingPoint, "MaterialProperties", GLConstantBuffer::EDrawUsage::STREAM);
	m_matUniformBuffer.upload(m_matProperties.size() * sizeof(m_matProperties[0]), &m_matProperties[0]);
	m_stateBuffer.end();
	m_matProperties.clear();
}

void GLMesh::render(const GLShader& shader, bool a_renderOpague, bool a_renderTransparent, bool a_bindMaterials)
{
	assert(shader.isBegun());

	if (!m_matUniformBuffer.isInitialized())
		initializeUBO(shader);
	m_matUniformBuffer.bind();

	m_stateBuffer.begin();
	if (m_textureArray.isInitialized())
		m_textureArray.bind(m_textureUnit);
	glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, NULL);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); wireframe rendering
	m_stateBuffer.end();
}

