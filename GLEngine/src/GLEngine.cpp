#include "GLEngine.h"

#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Threading/ThreadManager.h"
#include "Utils/FileModificationManager.h"

#include <SDL/SDL.h>

Input* GLEngine::input                   = NULL;
Graphics* GLEngine::graphics             = NULL;
bool GLEngine::s_shutdown                = false;
ThreadManager* GLEngine::s_threadManager = NULL;

void GLEngine::initialize()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		print("Unable to initialize SDL: %s\n", SDL_GetError());
		SDL_Quit();
		return;
	}
	graphics = new Graphics("GLEngine", 1900 / 2, 1000 / 2, 10, 30); // TODO: move construction args to app
	input = new Input();
	s_threadManager = new ThreadManager();
}

void GLEngine::createThread(const char* a_threadName, std::function<void()> a_func)
{
	s_threadManager->createThread(a_threadName, a_func);
}

void GLEngine::doMainThreadTick()
{
	input->pollEvents();
}

void GLEngine::doEngineTick()
{
	input->processEvents();
	FileModificationManager::update();
}

void GLEngine::sleep(uint a_timeMs)
{
	SDL_Delay(a_timeMs);
}

uint GLEngine::getTimeMs()
{
	return SDL_GetTicks();
}

void GLEngine::createGLContext()
{
	graphics->createGLContext();
}

void GLEngine::destroyGLContext()
{
	graphics->destroyGLContext();
}

void GLEngine::finish()
{
	s_threadManager->waitForAllThreadShutdown();
	SAFE_DELETE(input);
	SAFE_DELETE(graphics);
	SAFE_DELETE(s_threadManager);
	SDL_Quit();
}
