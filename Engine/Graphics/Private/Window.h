#pragma once

#include "Graphics/IWindow.h"

struct SDL_Window;

class Window final : public IWindow
{
public:

	virtual owner<IContext*> createContext(const EContextType& type) override;
	virtual void destroyContext(owner<IContext*> context) override;
	virtual void swapBuffer() override;

	SDL_Window* getSDLWindow() { return m_window; }
	void* getHDC()             { return m_hdc; }
	void* getHWND()            { return m_hwnd; }
	void* getHINSTANCE()       { return m_hinstance; }
	void* getWNDPROC()         { return m_wndproc; }

	uint getWidth() const      { return m_width; }
	uint getHeight() const     { return m_height; }

private:

	friend class Graphics;
	Window(const char* name, uint width, uint height, uint xPos, uint yPos, const EWindowMode& mode);
	Window(const Window&) = delete;
	~Window() override;

#ifdef DEBUG_BUILD
private:
	int md_numContextsCreated = 0;
#endif

private:

	SDL_Window* m_window = NULL;
	void* m_hinstance = NULL;
	void* m_hwnd = NULL;
	void* m_hdc = NULL;
	void* m_wndproc = NULL;

	uint m_width = 0;
	uint m_height = 0;
	uint m_xPos = 0;
	uint m_yPos = 0;
};