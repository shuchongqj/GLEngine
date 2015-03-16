#include "FloatImageProcessor.h"

#include "EResourceType.h"
#include "Utils/stb_image.h"

#include <fstream>
#include <assert.h>

bool FloatImageProcessor::process(const char* a_inResourcePath, const char* a_outResourcePath)
{
    const int type = EResourceType_FLOATIMAGE;
    int width, height, numComponents;

    const float* data = stbi_loadf(a_inResourcePath, &width, &height, &numComponents, 0);
    if (!data)
    {
	printf("FAILED TO LOAD IMAGE: %s \n", a_inResourcePath);
	return false;
    }
    std::fstream file(a_outResourcePath, std::ios::out | std::ios::binary);

    assert(file.is_open());
    file.write(reinterpret_cast<const char*>(&type), sizeof(int));
    file.write(reinterpret_cast<const char*>(&width), sizeof(int));
    file.write(reinterpret_cast<const char*>(&height), sizeof(int));
    file.write(reinterpret_cast<const char*>(&numComponents), sizeof(int));
    file.write(reinterpret_cast<const char*>(data), width * height * numComponents * 4);
    file.close();

    delete [] data;
    return true;
}
