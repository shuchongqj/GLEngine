#pragma once

#include "../ResourceProcessor.h"

class ModelProcessor : public ResourceProcessor
{
public:
	virtual void process(const char* inResourcePath, const char* outResourcePath);
};