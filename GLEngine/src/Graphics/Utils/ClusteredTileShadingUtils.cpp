/****************************************************************************/
/* Copyright (c) 2011, Markus Billeter, Ola Olsson, Erik Sintorn and Ulf Assarsson
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/
/****************************************************************************/
#include "Graphics/Utils/ClusteredTiledShadingUtils.h"

#include "Core.h"
#include "Graphics/Utils/PerspectiveCamera.h"

BEGIN_UNNAMED_NAMESPACE()

void updateClipRegionRoot(float a_nc, float lc, float lz, float lightRadius, float cameraScale, float &clipMin, float &clipMax)
{
	const float nz = (lightRadius - a_nc * lc) / lz;
	const float pz = (lc * lc + lz * lz - lightRadius * lightRadius) / (lz - (nz / a_nc) * lc);

	if (pz < 0.0f)
	{
		float c = -nz * cameraScale / a_nc;
		if (a_nc < 0.0f)
		{
			clipMin = glm::max(clipMin, c);
		}
		else
		{
			clipMax = glm::min(clipMax, c);
		}
	}
}

void updateClipRegion(float a_lc, float a_lz, float a_lightRadius, float a_cameraScale, float &a_clipMin, float &a_clipMax)
{
	const float rSq = a_lightRadius * a_lightRadius;
	const float lcSqPluslzSq = a_lc * a_lc + a_lz * a_lz;
	const float d = rSq * a_lc * a_lc - lcSqPluslzSq * (rSq - a_lz * a_lz);

	if (d >= 0.0f)
	{
		const float a = a_lightRadius * a_lc;
		const float b = sqrt(d);
		const float nx0 = (a + b) / lcSqPluslzSq;
		const float nx1 = (a - b) / lcSqPluslzSq;

		updateClipRegionRoot(nx0, a_lc, a_lz, a_lightRadius, a_cameraScale, a_clipMin, a_clipMax);
		updateClipRegionRoot(nx1, a_lc, a_lz, a_lightRadius, a_cameraScale, a_clipMin, a_clipMax);
	}
}

glm::vec4 computeClipRegion(const glm::vec3& a_lightPosView, float a_lightRadius, float a_cameraNear, const glm::mat4 &a_projection)
{
	glm::vec4 clipRegion(1.0f, 1.0f, -1.0f, -1.0f);
	if (a_lightPosView.z - a_lightRadius <= -a_cameraNear)
	{
		glm::vec2 clipMin(-1.0f, -1.0f);
		glm::vec2 clipMax(1.0f, 1.0f);

		updateClipRegion(a_lightPosView.x, a_lightPosView.z, a_lightRadius, a_projection[0][0], clipMin.x, clipMax.x);
		updateClipRegion(a_lightPosView.y, a_lightPosView.z, a_lightRadius, a_projection[1][1], clipMin.y, clipMax.y);

		clipRegion = glm::vec4(clipMin, clipMax);
	}

	return clipRegion;
}

inline void swap(float& a_a, float& a_b)
{
	const float tmp = a_a;
	a_a = a_b;
	a_b = tmp;
}

inline float calcClusterZ(float a_viewSpaceZ, float a_recNear, float a_recLogSD1)
{
	return logf(-a_viewSpaceZ * a_recNear) * a_recLogSD1;
}

END_UNNAMED_NAMESPACE()

IBounds2D ClusteredTiledShadingUtils::sphereToScreenSpaceBounds2D(
    const PerspectiveCamera& a_camera, const glm::vec3& a_lightPosViewSpace, float a_lightRadius, uint a_screenWidth, uint a_screenHeight)
{
	glm::vec4 reg = computeClipRegion(a_lightPosViewSpace, a_lightRadius, a_camera.getNear(), a_camera.getProjectionMatrix());
	reg = -reg;

	swap(reg.x, reg.z);
	swap(reg.y, reg.w);
	reg *= 0.5f;
	reg += 0.5f;
	reg = glm::clamp(reg, glm::vec4(0), glm::vec4(1));

	IBounds2D result;
	result.min.x = int(reg.x * float(a_screenWidth));
	result.min.y = int(reg.y * float(a_screenHeight));
	result.max.x = int(reg.z * float(a_screenWidth));
	result.max.y = int(reg.w * float(a_screenHeight));

	return result;
}

IBounds3D ClusteredTiledShadingUtils::sphereToScreenSpaceBounds3D(
	const PerspectiveCamera& a_camera, const glm::vec3& a_lightPosViewSpace, float a_lightRadius, uint a_screenWidth, uint a_screenHeight, 
	uint a_pixelsPerTileW, uint a_pixelsPerTileH, float a_recLogSD1)
{
	const IBounds2D bounds2D = sphereToScreenSpaceBounds2D(a_camera, a_lightPosViewSpace, a_lightRadius, a_screenWidth, a_screenHeight);
	const float recNear = 1.0f / a_camera.getNear();

	const int minZ = glm::max(int(calcClusterZ(a_lightPosViewSpace.z + a_lightRadius, recNear, a_recLogSD1)), 0);
	const int maxZ = glm::max(int(ceilf(calcClusterZ(a_lightPosViewSpace.z - a_lightRadius, recNear, a_recLogSD1)) + 0.5f), 0);

	const glm::ivec2 tileSizePx = glm::ivec2(a_pixelsPerTileW, a_pixelsPerTileH);

	IBounds3D bounds3D;
	bounds3D.min = glm::ivec3(bounds2D.min / tileSizePx, minZ);
	bounds3D.max = glm::ivec3(bounds2D.max / tileSizePx + 1, maxZ);

	return bounds3D;
}