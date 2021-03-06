#include "../globals.glsl"

/* REQUIRED DEFINES /* !NOT IN GLOBAL DEFINES!
BLUR_KERNEL_RADIUS (float)
SHARPNESS    (float)
VALTYPE      (e.g: vec2)
VALACCESSOR  (e.g: rg)
*/

in vec2 v_texcoord;

layout(binding = BLUR_TEXTURE_BINDING_POINT) uniform FBOSampler u_blurTex;
layout(binding = DEPTH_TEXTURE_BINDING_POINT) uniform FBOSampler u_depthTex;

layout(location = 0) out float out_val;

uniform vec2 u_invScreenSize;

vec2 getSamplePos(vec2 a_pxOffset)
{
	return v_texcoord + a_pxOffset * u_invScreenSize;
}

float sampleDepth(vec2 a_texcoord)
{
	return singleSampleFBO(u_depthTex, a_texcoord).r;
}

VALTYPE sampleVal(vec2 a_texcoord)
{
	return singleSampleFBO(u_blurTex, a_texcoord).VALACCESSOR;
}

float crossBilateralWeight(float a_r, float a_z, float a_z0)
{
	float blurSigma = (BLUR_KERNEL_RADIUS + 1.0f) * 0.5f;
	float blurFalloff = 1.0f / (2.0f * blurSigma * blurSigma);
	float dz = (a_z0 - a_z) * SHARPNESS;
	return exp2(-a_r * a_r * blurFalloff - dz * dz);
}

void main()
{
	VALTYPE centerVal = sampleVal(v_texcoord);
	float centerDepth = sampleDepth(v_texcoord);
	VALTYPE totalVal  = centerVal;
	float totalWeight = 1.0;
	float i = 1.0;

	for(; i <= BLUR_KERNEL_RADIUS / 2; i += 1.0)
	{
		{
			vec2 samplePos = getSamplePos(vec2(0, i));
			VALTYPE val    = sampleVal(samplePos);
			float depth    = sampleDepth(samplePos);
			float weight   = crossBilateralWeight(i, depth, centerDepth);
			totalVal      += val * weight;
			totalWeight   += weight;
		}
		{
			vec2 samplePos = getSamplePos(vec2(0, -i));
			VALTYPE val    = sampleVal(samplePos);
			float depth    = sampleDepth(samplePos);
			float weight   = crossBilateralWeight(i, depth, centerDepth);
			totalVal      += val * weight;
			totalWeight   += weight;
		}
	}

	for(; i <= BLUR_KERNEL_RADIUS; i += 2.0)
	{
		{
			vec2 samplePos = getSamplePos(vec2(0, 0.5 + i));
			VALTYPE val    = sampleVal(samplePos);
			float depth    = sampleDepth(samplePos);
			float weight   = crossBilateralWeight(i, depth, centerDepth);
			totalVal      += val * weight;
			totalWeight   += weight;
		}
		{
			vec2 samplePos = getSamplePos(vec2(0, -0.5 - i));
			VALTYPE val    = sampleVal(samplePos);
			float depth    = sampleDepth(samplePos);
			float weight   = crossBilateralWeight(i, depth, centerDepth);
			totalVal      += val * weight;
			totalWeight   += weight;
		}
	}

	VALTYPE avgVal = totalVal / totalWeight;
	out_val = avgVal;
}