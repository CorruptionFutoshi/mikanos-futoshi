#pragma once

#include <vector>
#include <optional>
#include "graphics.hpp"

class Window {
	public:
		class WindowWriter : public PixelWriter {
			public:
				WindowWriter(Window& window) : window_{window} {}

				virtual void Write(int x, int y, const PixelColor& c) override {
					window_.At(x, y) = c;
				}

				virtual int Width() const override { return window_.Width();}

				virtual int Height() const override { return window_.Height();}

			private:
				Window& window_;
		};

		Window(int width, int height);
		// default represent that explicit declare implicit declared method.
		~Window() = default;
		// delete represent that delete implicit declared method like copy constructor and = operator.
		Window(const Window& rhs) = delete;
		Window& operator=(const Window& rhs) = delete;

		void DrawTo(PixelWriter& writer, Vector2D<int> position);

		void SetTransparentColor(std::optional<PixelColor> c);

		WindowWriter* Writer();

		// one of the reason that why there are method with const and method without const is that in const method only can 
		// can call method with const.
		PixelColor& At(int x, int y);
		const PixelColor& At(int x, int y) const;

		int Width() const;
		int Height() const;

	private:
		int width_, height_;
		// the reason why there is a difference between this and framebufferwriter is that framebuffer is array.
		// and framebufferconfig's pixelat() should return address of pixel represented by framebuffer.
		// but this windowwriter's write() is not update framebuffer and window's drawto() use framebufferwriter's write().
		// so we can use easy to understand model here.
		std::vector<std::vector<PixelColor>> data_{};
		WindowWriter writer_{*this};
		// optional<> is like c#'s nullable type. and it have implicit bool type conversion.
		std::optional<PixelColor> transparent_color_{std::nullopt};
};

