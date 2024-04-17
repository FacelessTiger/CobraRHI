#include <Core/Window.h>
#include <Core/MouseButtonCodes.h>
#include <Core/KeyCodes.h>
#include <Events/ApplicationEvent.h>
#include <Events/KeyEvent.h>
#include <Events/MouseEvent.h>

#include <Windows.h>

#include <locale>
#include <unordered_map>
#include <iostream>
#include <assert.h>

namespace Cobra {

	namespace Utils {

		KeyCode WindowsKeyCodeToGA(uint32_t code)
		{
			switch (code)
			{
				case 57:	return Key::Space;
				case 40:	return Key::Apostrophe;
				case 51:	return Key::Comma;
				case 12:	return Key::Minus;
				case 52:	return Key::Period;
				case 53:	return Key::Slash;

				case 11:	return Key::D0;
				case 2:		return Key::D1;
				case 3:		return Key::D2;
				case 4:		return Key::D3;
				case 5:		return Key::D4;
				case 6:		return Key::D5;
				case 7:		return Key::D6;
				case 8:		return Key::D7;
				case 9:		return Key::D8;
				case 10:	return Key::D9;

				case 39:	return Key::Semicolon;
				case 13:	return Key::Equal;

				case 30:	return Key::A;
				case 48:	return Key::B;
				case 46:	return Key::C;
				case 32:	return Key::D;
				case 18:	return Key::E;
				case 33:	return Key::F;
				case 34:	return Key::G;
				case 35:	return Key::H;
				case 23:	return Key::I;
				case 36:	return Key::J;
				case 37:	return Key::K;
				case 38:	return Key::L;
				case 50:	return Key::M;
				case 49:	return Key::N;
				case 24:	return Key::O;
				case 25:	return Key::P;
				case 16:	return Key::Q;
				case 19:	return Key::R;
				case 31:	return Key::S;
				case 20:	return Key::T;
				case 22:	return Key::U;
				case 47:	return Key::V;
				case 17:	return Key::W;
				case 45:	return Key::X;
				case 21:	return Key::Y;
				case 44:	return Key::Z;

				case 26:	return Key::LeftBracket;
				case 43:	return Key::Backslash;
				case 27:	return Key::RightBracket;
				case 41:	return Key::GraveAccent;

				case 1:		return Key::Escape;
				case 28:	return Key::Enter;
				case 15:	return Key::Tab;
				case 14:	return Key::Backspace;
				case 338:	return Key::Insert;
				case 339:	return Key::Delete;
				case 333:	return Key::Right;
				case 331:	return Key::Left;
				case 336:	return Key::Down;
				case 328:	return Key::Up;
				case 329:	return Key::PageUp;
				case 337:	return Key::PageDown;
				case 327:	return Key::Home;
				case 335:	return Key::End;
				case 58:	return Key::CapsLock;
				case 70:	return Key::ScrollLock;
				case 325:	return Key::NumLock;
				case 311:	return Key::PrintScreen;
				case 69:	return Key::Pause;
				case 59:	return Key::F1;
				case 60:	return Key::F2;
				case 61:	return Key::F3;
				case 62:	return Key::F4;
				case 63:	return Key::F5;
				case 64:	return Key::F6;
				case 65:	return Key::F7;
				case 66:	return Key::F8;
				case 67:	return Key::F9;
				case 68:	return Key::F10;
				case 87:	return Key::F11;
				case 88:	return Key::F12;

				case 82:	return Key::KP0;
				case 79:	return Key::KP1;
				case 80:	return Key::KP2;
				case 81:	return Key::KP3;
				case 75:	return Key::KP4;
				case 76:	return Key::KP5;
				case 77:	return Key::KP6;
				case 71:	return Key::KP7;
				case 72:	return Key::KP8;
				case 73:	return Key::KP9;
				case 83:	return Key::KPDecimal;
				case 309:	return Key::KPDivide;
				case 55:	return Key::KPMultiply;
				case 74:	return Key::KPSubtract;
				case 78:	return Key::KPAdd;
				case 284:	return Key::KPEnter;

				case 42:	return Key::LeftShift;
				case 29:	return Key::LeftControl;
				case 56:	return Key::LeftAlt;
				case 347:	return Key::LeftSuper;
				case 54:	return Key::RightShift;
				case 285:	return Key::RightControl;
				case 312:	return Key::RightAlt;
				case 349:	return Key::RightSuper;
			}

			assert(false && "Unrecgonized keycode!");
			return Key::Z;
		}

	}

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static bool s_Initialized = false;
	static ATOM s_WindowClass;
	static HINSTANCE s_Instance;

	template<>
	struct Impl<Window>
	{
		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;

			Window::EventCallbackFn EventCallback;
		};

		WindowData Data;
		HWND Window;
		std::unordered_map<KeyCode, bool> Keys;
		bool MouseButtons[3] = { false };

		Impl(const WindowProps& props);
		~Impl();
	};

	Window::Window(const WindowProps& props)
	{
		pimpl = std::make_unique<Impl<Window>>(props);
	}

	Window::~Window() { }
	Window::Window(Window&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Window& Window::operator=(Window&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Window::Update()
	{
		MSG msg;

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void Window::SetSize(const iVec2& size)
	{
		pimpl->Data.Width = (uint32_t)size.x;
		pimpl->Data.Height = (uint32_t)size.y;

		RECT rect = { 0, 0, (LONG)size.x, (LONG)size.y };
		AdjustWindowRectExForDpi(&rect, WS_OVERLAPPEDWINDOW, false, 0, GetDpiForWindow(pimpl->Window));
		SetWindowPos(pimpl->Window, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
	}

	iVec2 Window::GetSize() const { return { pimpl->Data.Width, pimpl->Data.Height }; }
	unsigned int Window::GetWidth() const { return pimpl->Data.Width; }
	unsigned int Window::GetHeight() const { return pimpl->Data.Height; }

	void Window::SetPosition(const iVec2& position)
	{
		RECT rect = { 0, 0, (LONG)position.x, (LONG)position.y };
		AdjustWindowRectExForDpi(&rect, WS_OVERLAPPEDWINDOW, false, 0, GetDpiForWindow(pimpl->Window));
		SetWindowPos(pimpl->Window, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}

	iVec2 Window::GetPosition() const
	{
		POINT pos;
		ClientToScreen(pimpl->Window, &pos);

		return { (uint32_t)pos.x, (uint32_t)pos.y };
	}

	iVec2 Window::GetMousePosition() const
	{
		POINT pos;
		GetCursorPos(&pos);
		ScreenToClient(pimpl->Window, &pos);

		return { (uint32_t)pos.x, (uint32_t)pos.y };
	}

	bool Window::IsKeyPressed(int keycode) const { return pimpl->Keys[keycode]; }
	bool Window::IsMouseButtonPressed(int button) const { return pimpl->MouseButtons[button]; }

	void Window::SetEventCallback(const EventCallbackFn& callback) { pimpl->Data.EventCallback = callback; }
	void* Window::GetNativeWindow() const { return pimpl->Window; }

	Impl<Window>::Impl(const WindowProps& props)
	{
		Data.Title = props.Title;
		Data.Width = props.Width;
		Data.Height = props.Height;

		if (!s_Initialized)
		{
			s_Instance = GetModuleHandle(0);

			WNDCLASSEXW wc = { sizeof(wc) };
			wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
			wc.lpfnWndProc = WindowProc;
			wc.hInstance = s_Instance;
			wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
			wc.lpszClassName = L"Cobra";
			wc.hIcon = (HICON)LoadImage(nullptr, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

			s_WindowClass = RegisterClassExW(&wc);
			s_Initialized = true;
		}

		DWORD style = WS_OVERLAPPEDWINDOW;
		RECT rect = { 0, 0, (LONG)props.Width, (LONG)props.Height };
		AdjustWindowRect(&rect, style, false);

		std::wstring wTitle(props.Title.size(), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, props.Title.data(), (int)props.Title.size(), wTitle.data(), (int)props.Title.size());

		Window = CreateWindowEx(0, MAKEINTATOM(s_WindowClass), wTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, s_Instance, nullptr);
		SetProp(Window, L"Cobra", this);
		ShowWindow(Window, SW_SHOW);
	}

	Impl<Window>::~Impl()
	{
		RemoveProp(Window, L"Cobra");
		DestroyWindow(Window);
	}

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		Impl<Window>* window = (Impl<Window>*)GetProp(hwnd, L"Cobra");
		if (!window) return DefWindowProc(hwnd, uMsg, wParam, lParam);
		if (!window->Data.EventCallback) return DefWindowProc(hwnd, uMsg, wParam, lParam);

		Impl<Window>::WindowData& data = window->Data;
		switch (uMsg)
		{
			case WM_SIZE:
			{
				data.Width = LOWORD(lParam);
				data.Height = HIWORD(lParam);

				WindowResizeEvent event(data.Width, data.Height);
				data.EventCallback(event);
				return S_OK;
			}
			case WM_MOUSEMOVE:
			{
				MouseMovedEvent event(LOWORD(lParam), HIWORD(lParam));
				data.EventCallback(event);
				return S_OK;
			}
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			{
				bool pressed = false;
				int button;

				if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MBUTTONDOWN) pressed = true;
				if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP) button = 0;
				else if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP) button = 1;
				else if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP) button = 2;

				if (pressed)
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
				}
				else
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
				}

				window->MouseButtons[button] = pressed;
				return S_OK;
			}
			case WM_MOUSEWHEEL:
			{
				MouseScrolledEvent event(0.0f, (short)HIWORD(wParam) / (float)WHEEL_DELTA);
				data.EventCallback(event);
				return S_OK;
			}
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			{
				bool pressed = !(HIWORD(lParam) & KF_UP);
				bool repeat = HIWORD(lParam) & KF_REPEAT;
				int scancode = HIWORD(lParam) & (KF_EXTENDED | 0xff);

				KeyCode key = Utils::WindowsKeyCodeToGA(scancode);
				if (pressed)
				{
					KeyPressedEvent event(key, repeat);
					data.EventCallback(event);
				}
				else
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
				}

				window->Keys[key] = pressed;
				break;
			}
			case WM_CHAR:
			case WM_SYSCHAR:
			{
				KeyTypedEvent event((WCHAR)wParam);
				data.EventCallback(event);
				return S_OK;
			}
			case WM_CLOSE:
			{
				WindowCloseEvent event;
				data.EventCallback(event);
				return S_OK;
			}
		}

		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

}