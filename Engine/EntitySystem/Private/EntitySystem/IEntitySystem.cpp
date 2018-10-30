#include "EntitySystem/IEntitySystem.h"

#include "EntitySystem.h"

IEntitySystem::IEntitySystem()
	: m_impl(new EntitySystem())
{
}

IEntitySystem::~IEntitySystem() 
{ 
	delete m_impl;
}

void IEntitySystem::doStuff() 
{ 
	m_impl->doStuff();
}