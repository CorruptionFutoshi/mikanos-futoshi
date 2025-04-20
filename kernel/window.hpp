#pragma once

#include <vector>
#include <optional>
#include <string>
#include "graphics.hpp"
#include "frame_buffer.hpp"

enum class WindowRegion {
	kTitleBar,
	kCloseButton,
	kBorder,
	kOther,
};

class Window {
	public:
		class WindowWriter : public PixelWriter {
			public:
				WindowWriter(Window& window) : window_{window} {}

				virtual void Write(Vector2D<int> pos, const PixelColor& c) override {
					window_.Write(pos, c);
				}

				virtual int Width() const override { return window_.Width();}

				virtual int Height() const override { return window_.Height();}

			private:
				Window& window_;
		};

		Window(int width, int height, PixelFormat shadow_format);
		// default represent that explicit declare implicit declared method.
		virtual ~Window() = default;
		// delete represent that delete implicit declared method like copy constructor and = operator.
		Window(const Window& rhs) = delete;
		Window& operator=(const Window& rhs) = delete;

		void DrawTo(FrameBuffer& dst, Vector2D<int> pos, const Rectangle<int>& area);

		void SetTransparentColor(std::optional<PixelColor> c);

		WindowWriter* Writer();

		// one of the reason that why there are method with const and method without const is that in const method only can 
		// can call method with const.
		// PixelColor& At(int x, int y);
		const PixelColor& At(Vector2D<int> pos) const;
		void Write(Vector2D<int> pos, PixelColor c);

		int Width() const;
		int Height() const;
		Vector2D<int> Size() const;
		
		void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);

		virtual void Activate() {}
		virtual void Deactivate() {}
		virtual WindowRegion GetWindowRegion(Vector2D<int> pos);

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

		FrameBuffer shadow_buffer_{};
};

class ToplevelWindow : public Window {
	public:
		static constexpr Vector2D<int> kTopLeftMargin{4, 24};
		static constexpr Vector2D<int> kBottomRightMargin{4, 4};
		static constexpr int kMarginX = kTopLeftMargin.x + kBottomRightMargin.x;
		static constexpr int kMarginY = kTopLeftMargin.y + kBottomRightMargin.y;

		class InnerAreaWriter : public PixelWriter {
			public:
				InnerAreaWriter(ToplevelWindow& window) : window_{window} {}
				virtual void Write(Vector2D<int> pos, const PixelColor& c) override {
					window_.Write(pos + kTopLeftMargin, c);
				}

				virtual int Width() const override {
					return window_.Width() - kTopLeftMargin.x - kBottomRightMargin.x;
				}

				virtual int Height() const override {
					return window_.Height() - kTopLeftMargin.y - kBottomRightMargin.y;
				}

			private:
				ToplevelWindow& window_;
		};

		ToplevelWindow(int width, int height, PixelFormat shadow_format, const std::string& title);

		virtual void Activate() override;
		virtual void Deactivate() override;
		virtual WindowRegion GetWindowRegion(Vector2D<int> pos) override;

		InnerAreaWriter* InnerWriter() { return &inner_writer_; }
		Vector2D<int> InnerSize() const;
	
	private:
		std::string title_;
		InnerAreaWriter inner_writer_{*this};
};

void DrawWindow(PixelWriter& writer, const char* title);
void DrawTextbox(PixelWriter& writer, Vector2D<int> pos, Vector2D<int> size);
void DrawTerminal(PixelWriter& writer, Vector2D<int> pos, Vector2D<int> size);
void DrawWindowTitle(PixelWriter& writer, const char* title, bool active);
