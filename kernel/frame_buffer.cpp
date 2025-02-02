#include "frame_buffer.hpp"

// this class is become copy source when call this method without framebufferconfig from loader.
// this class is become copy destination when call this method with framebufferconfig from loader.
Error FrameBuffer::Initialize(const FrameBufferConfig& config) {
	config_ = config;

	const auto bits_per_pixel = BitsPerPixel(config_.pixel_format);

	if (bits_per_pixel <= 0) {
		return MAKE_ERROR(Error::kUnknownPixelFormat);
	}

	if (config_.frame_buffer) {
		// resize method expand vector to have elemnts of parameter count.
		// resize(0) represnt that delete all elements inside thsi vector.
		// buffer is used to set config_'s frame_buffer field. 
		// so if frame_buffer is already exist, buffer is not need. 
		buffer_.resize(0);
	} else {
		buffer_.resize(((bits_per_pixel + 7) / 8) * config_.horizontal_resolution * config_ .vertical_resolution);
		// dota method return head pointer of this vector.
		config_.frame_buffer = buffer_.data();
		config_.pixels_per_scan_line = config_.horizontal_resolution;
	}

	switch (config_.pixel_format) {
		case kPixelRGBResv8BitPerColor:
			writer_ = std::make_unique<RGBResv8BitPerColorPixelWriter>(config_);
			break;
		case kPixelBGRResv8BitPerColor:
			writer_ = std::make_unique<BGRResv8BitPerColorPixelWriter>(config_);
			break;
		default:
			return MAKE_ERROR(Error::kUnknownPixelFormat);
	}

	return MAKE_ERROR(Error::kSuccess);
}

// src represent copy source framebufferconfig
// pos represent that position of layer draw. 
Error FrameBuffer::Copy(Vector2D<int> pos, const FrameBuffer& src) {
	// in this method, config_ represent framebufferconfig from loader and destination.
	if (config_.pixel_format != src.config_.pixel_format) {
		return MAKE_ERROR(Error::kUnknownPixelFormat);
	}

	// i don't know why parameter is not src.pixel_format.
	// config_.pixel_format is already checked in Initialize()
	const auto bits_per_pixel = BitsPerPixel(config_.pixel_format);

	if (bits_per_pixel <= 0) {
		return MAKE_ERROR(Error::kUnknownPixelFormat);
	}

	const auto dst_width = config_.horizontal_resolution;
	const auto dst_height = config_.vertical_resolution;
	const auto src_width = src.config_.horizontal_resolution;
	const auto src_height = src.config_.vertical_resolution;

	// x of left side.
	const int copy_start_dst_x = std::max(pos.x, 0);
	// y of top side.
	const int copy_start_dst_y = std::max(pos.y, 0);
	// x of right side.
	const int copy_end_dst_x = std::min(pos.x + src_width, dst_width);
	// y of bottom side.
	const int copy_end_dst_y = std::min(pos.y + src_height, dst_height);
	
	const auto bytes_per_pixel = (bits_per_pixel + 7) / 8;
	const auto bytes_per_copy_line = bytes_per_pixel * (copy_end_dst_x - copy_start_dst_x);

	// dst_buf is head pointer of copy destination.
	// config_.frame_buffer represent {0, 0} of this window.
	uint8_t* dst_buf = config_.frame_buffer + bytes_per_pixel * 
		(config_.pixels_per_scan_line * copy_start_dst_y + copy_start_dst_x);
	uint8_t* src_buf = src.config_.frame_buffer;

	for (int dy = 0; dy < copy_end_dst_y - copy_start_dst_y; ++dy) {
		memcpy(dst_buf, src_buf, bytes_per_copy_line);
		dst_buf += bytes_per_pixel * config_.pixels_per_scan_line;
		src_buf += bytes_per_pixel * src.config_.pixels_per_scan_line;
	}

	return MAKE_ERROR(Error::kSuccess);
}

int BitsPerPixel(PixelFormat format) {
	switch (format) {
		case kPixelRGBResv8BitPerColor: return 32;
		case kPixelBGRResv8BitPerColor: return 32;
	}

	return -1;
}
