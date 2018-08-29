#pragma once

#include <Core.h>

class IBuffer;
enum class EBufferType;

enum class EContextType
{
	OPENGL
};

class IContext
{
public:

	virtual EContextType getType() const = 0;
	virtual owner<IBuffer*> createBuffer(const EBufferType& type) = 0;
	virtual void destroyBuffer(owner<IBuffer*> buffer) = 0;

protected:

	friend class IWindow;
	virtual ~IContext() {}
};