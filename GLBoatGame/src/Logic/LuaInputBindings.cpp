#include "Logic/LuaInputBindings.h"

#include "ECS/LogicSystem.h"

void LuaInputBindings::initialize()
{
	m_logicSystem.loadLuaScript(m_lua, "assets/lua/input.lua");

	m_keyDown = m_lua["input"]["keydown"];
	assert(m_keyDown.valid());
	m_keyUp = m_lua["input"]["keyup"];
	assert(m_keyUp.valid());
	m_update = m_lua["input"]["update"];
	assert(m_update.valid());
}

void LuaInputBindings::update(float a_deltaSec)
{
	m_update(a_deltaSec);
}

void LuaInputBindings::keyDown(EKey a_key)
{
	m_keyDown(a_key);
}

void LuaInputBindings::keyUp(EKey a_key)
{
	m_keyUp(a_key);
}
