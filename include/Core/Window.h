#pragma once

#include <Core/Core.h>
#include <Events/Event.h>

#include <string>
#include <functional>
#include <memory>

namespace Cobra {

	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
		bool CustomTitlebar;

		WindowProps(const std::string& title = "Cobra",
			uint32_t width = 1600,
			uint32_t height = 900,
			bool customTitlebar = false)
			: Title(title), Width(width), Height(height), CustomTitlebar(customTitlebar)
		{ }
	};

	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;
		std::unique_ptr<Impl<Window>> pimpl;
	public:
		Window(const WindowProps& props = WindowProps());
		virtual ~Window();

		Window(const Window&) = delete;
		Window& operator=(Window& other) = delete;

		Window(Window&& other) noexcept;
		Window& operator=(Window&& other) noexcept;

		void Update();

		void SetSize(const iVec2& size);
		iVec2 GetSize() const;
		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		void SetPosition(const iVec2& position);
		iVec2 GetPosition() const;

		iVec2 GetMousePosition() const;
		bool IsKeyPressed(int keycode) const;
		bool IsMouseButtonPressed(int button) const;

		void SetEventCallback(const EventCallbackFn& callback);
		void* GetNativeWindow() const;
	};

}