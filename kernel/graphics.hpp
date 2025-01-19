#pragma once

#include "frame_buffer_config.hpp"

struct PixelColor {
	uint8_t r, g, b;
};

class PixelWriter {
	public:
		// : config_{config} means config of parameter set to config_ of field
		PixelWriter(const FrameBufferConfig& config) : config_{config}{
		}
	
	// ~{class name} means destructor	
	virtual ~PixelWriter() = default;
	// {method} = 0 means this method is pure virtual function. it is method that have no contents for override
	virtual void Write(int x, int y, const PixelColor& c) = 0;

	protected:
		uint8_t* PixelAt(int x, int y) {
			// in this format, 1 pixel is 4 byte so 4* pixel_position
			return config_.frame_bufferptr + 4 * (config_.pixels_per_scan_line * y + x);
		}

	private:
		const FrameBufferConfig& config_;
};

class RGBResv8BitPerColorPixelWriter : public PixelWriter {
	public:
		// using {super class name}:{super class name} means constructor of super class
		using PixelWriter::PixelWriter;
		virtual void Write(int x, int y, const PixelColor& c) override;
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter {
	public:
		using PixelWriter::PixelWriter;
		virtual void Write(int x, int y, const PixelColor& c) override;
};

template <typename T>
struct Vector2D {
	T x, y;

	// this "template <typename U>" means in this method we will use U type
	template <typename U>
		// this object is left side. right side is parameter rhs
		Vector2D<T>& operator +=(const Vector2D<U>& rhs) {
			x += rhs.x;
			y += rhs.y;
			// "this" is pointer of this object, Vector2D. so *this is Vector2D 
			return *this;
		}
};

void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c);
void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c);
