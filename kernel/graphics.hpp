#pragma once

#include <algorithm>
#include "frame_buffer_config.hpp"

struct PixelColor {
	uint8_t r, g, b;
};

// lhs is left hand side. rhs is right hand side.
inline bool operator==(const PixelColor& lhs, const PixelColor& rhs) {
	return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

inline bool operator!=(const PixelColor& lhs, const PixelColor& rhs) {
	return !(lhs==rhs);
}

template <typename T>
struct Vector2D {
	T x, y;

	// this "template <typename U>" means in this method we will use U type
	// in operator oberload as member method, we wil be gave right hand side as first parameter.
	// because we do't need left hand side, we can use left hand side such as *this. 
	template <typename U>
	// this object is left side. right side is parameter rhs
	Vector2D<T>& operator +=(const Vector2D<U>& rhs) {
		x += rhs.x;
		y += rhs.y;
		// "this" is pointer of this object, Vector2D. so *this is Vector2D 
		return *this;
	}
		
	template< typename U>
	Vector2D<T> operator +(const Vector2D<U> rhs) const {
		auto tmp = *this;
		tmp += rhs;
		return tmp;
	}	
	
	template< typename U>
	Vector2D<T> operator -=(const Vector2D<U> rhs) {
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}	

	template< typename U>
	Vector2D<T> operator -(const Vector2D<U> rhs) const {
		auto tmp = *this;
		tmp -= rhs;
		return tmp;
	}	
};

template <typename T>
Vector2D<T> ElementMax(const Vector2D<T>& vector1, const Vector2D<T>& vector2) {
	return {std::max(vector1.x, vector2.x), std::max(vector1.y, vector2.y)};
}

template <typename T>
Vector2D<T> ElementMin(const Vector2D<T>& vector1, const Vector2D<T>& vector2) {
	return {std::min(vector1.x, vector2.x), std::min(vector1.y, vector2.y)};
}

template <typename T>
struct Rectangle {
	Vector2D<T> pos, size;
};

template <typename T, typename U>
Rectangle<T> operator&(const Rectangle<T>& lhs, const Rectangle<U> rhs) {
	const auto lhs_end = lhs.pos + lhs.size;
	const auto rhs_end = rhs.pos + rhs.size;

	if (lhs_end.x < rhs.pos.x || lhs_end.y < rhs.pos.y ||
	    rhs_end.x < lhs.pos.x || rhs_end.y < lhs.pos.y) {
		return {{0, 0}, {0, 0}};
	}

	auto new_pos = ElementMax(lhs.pos, rhs.pos);
	auto new_size = ElementMin(lhs_end, rhs_end) - new_pos;
	return {new_pos, new_size};
}

class PixelWriter {
	public:
	// : config_{config} means config of parameter set to config_ of field
	// 	PixelWriter(const FrameBufferConfig& config) : config_{config}{
	//	}
	
	// ~{class name} means destructor	
	virtual ~PixelWriter() = default;
	// {method} = 0 means this method is pure virtual function. it is method that have no contents for override
	virtual void Write(Vector2D<int> pos, const PixelColor& c) = 0;
	virtual int Width() const = 0;
	virtual int Height() const = 0;
};

class FrameBufferWriter : public PixelWriter {
	public:
		FrameBufferWriter(const FrameBufferConfig& config) : config_{config} {}
		virtual ~FrameBufferWriter() = default;
		virtual int Width() const override { return config_.horizontal_resolution; }
		virtual int Height() const override { return config_.vertical_resolution; }

	protected:
		uint8_t* PixelAt(Vector2D<int> pos) {
			// in this format, 1 pixel is 4 byte so 4* pixel_position
			return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * pos.y + pos.x);
		}

	private:
		const FrameBufferConfig& config_;
};

class RGBResv8BitPerColorPixelWriter : public FrameBufferWriter {
	public:
		// using {super class name}:{super class name} means constructor of super class
		using FrameBufferWriter::FrameBufferWriter;
		virtual void Write(Vector2D<int> pos, const PixelColor& c) override;
};

class BGRResv8BitPerColorPixelWriter : public FrameBufferWriter {
	public:
		using FrameBufferWriter::FrameBufferWriter;
		virtual void Write(Vector2D<int> pos, const PixelColor& c) override;
};

void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c);
void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c);

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

void DrawDesktop(PixelWriter& writer);
