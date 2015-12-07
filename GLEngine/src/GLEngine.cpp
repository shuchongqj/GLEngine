#include "GLEngine.h"

#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Utils/ThreadManager.h"
#include "Utils/FileModificationManager.h"
#include "EASTL/custom_glengine_allocator.h"

#include <SDL/SDL.h>

BEGIN_UNNAMED_NAMESPACE()

const uint INITIAL_WINDOW_OFFSET_X = 10;
const uint INITIAL_WINDOW_OFFSET_Y = 30;

END_UNNAMED_NAMESPACE()

Input* GLEngine::input                   = NULL;
Graphics* GLEngine::graphics             = NULL;
bool GLEngine::s_shutdown                = false;
ThreadManager* GLEngine::s_threadManager = NULL;

void GLEngine::initialize(const char* a_windowName, uint a_width, uint a_height, bool a_createWindow)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		print("Unable to initialize SDL: %s\n", SDL_GetError());
		SDL_Quit();
		return;
	}
	if (a_createWindow)
		graphics = new Graphics(a_windowName, a_width, a_height, INITIAL_WINDOW_OFFSET_X, INITIAL_WINDOW_OFFSET_Y);
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
