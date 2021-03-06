#include "Database/Processors/ByteImageProcessor.h"

#include "Database/Assets/DBTexture.h"

#include <fstream>

bool ByteImageProcessor::process(const eastl::string& a_resourcePath, AssetDatabase& a_assetDatabase)
{
	owner<DBTexture*> texture = new DBTexture();
	texture->loadFromFile(a_resourcePath);
	a_assetDatabase.addAsset(FileUtils::getFileNameFromPath(a_resourcePath), texture);
	return true;
}