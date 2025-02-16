#include "frame_buffer.hpp"

namespace {
	int BytesPerPixel(PixelFormat format) {
		switch (format) {
			case kPixelRGBResv8BitPerColor: return 4;
			case kPixelBGRResv8BitPerColor: return 4;
		}

		return -1;
	}

	uint8_t* FrameBufferAddrAt(Vector2D<int> pos, const FrameBufferConfig& config) {
		return config.frame_buffer + BytesPerPixel(config.pixel_format) * (config.pixels_per_scan_line * pos.y + pos.x);
	}

	int BytesPerScanLine(const FrameBufferConfig& config) {
		return BytesPerPixel(config.pixel_format) * config.pixels_per_scan_line;
	}

	Vector2D<int> FrameBufferSize(const FrameBufferConfig& config) {
		return {static_cast<int>(config.horizontal_resolution), static_cast<int>(config.vertical_resolution)};
	}
}

// this class is become copy source when call this method without framebufferconfig from loader.
// this class is become copy destination when call this method with framebufferconfig from loader.
Error FrameBuffer::Initialize(const FrameBufferConfig& config) {
	config_ = config;

	const auto bytes_per_pixel = BytesPerPixel(config_.pixel_format);

	if (bytes_per_pixel <= 0) {
		return MAKE_ERROR(Error::kUnknownPixelFormat);
	}

	if (config_.frame_buffer) {
		// resize method expand vector to have elemnts of parameter count.
		// resize(0) represnt that delete all elements inside thsi vector.
		// buffer is used to set config_'s frame_buffer field. 
		// so if frame_buffer is already exist, buffer is not need. 
		buffer_.resize(0);
	} else {
		buffer_.resize(bytes_per_pixel * config_.horizontal_resolution * config_ .vertical_resolution);
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

// src represent copy source framebuffer that is shadow_buffer. 
// dst_pos represent that position of layer draw. it's position is based on framebufferconfig from loader.
// position of Rectangle represent top left position of Rectangle. 
// src_area.pos is based on shadow_buffer of src window.
Error FrameBuffer::Copy(Vector2D<int> dst_pos, const FrameBuffer& src, const Rectangle<int>& src_area) {
	// in this method, config_ represent framebufferconfig from loader and destination.
	if (config_.pixel_format != src.config_.pixel_format) {
		return MAKE_ERROR(Error::kUnknownPixelFormat);
	}

	// i don't know why parameter is not src.pixel_format.
	// config_.pixel_format is already checked in Initialize()
	const auto bytes_per_pixel = BytesPerPixel(config_.pixel_format);

	if (bytes_per_pixel <= 0) {
		return MAKE_ERROR(Error::kUnknownPixelFormat);
	}

	// declare three Rectangles with position based on FrameBufferConfig from Loader.
	const Rectangle<int> src_area_outline{dst_pos, src_area.size};
	// we want topleft position of srcWindow.
	// dst_pos is based on FrameBufferConfig from Loader. src_area.pos is based on shadow_buffer of src window.
	// src_area.pos = dst_pos - topleft position of srcWindow. 
	const Rectangle<int> srcWindow_outline{dst_pos - src_area.pos, FrameBufferSize(src.config_)};
	// dstWindow represent screen.
	const Rectangle<int> dstWindow_outline{{0, 0}, FrameBufferSize(config_)};
	// i wonder if srcWindow_outline is not need because src_area_outline is inside of srcWindow_outline.
	const auto copy_area = dstWindow_outline & srcWindow_outline & src_area_outline;
	// change what copy_area.pos based on from FrameBufferConfig from Loader to shadow_buffer of src window to use FrameAddrAt. 
	const auto copy_area_pos_src = copy_area.pos - (dst_pos - src_area.pos);

	uint8_t* dst_buf = FrameBufferAddrAt(copy_area.pos, config_);
	uint8_t* src_buf = FrameBufferAddrAt(copy_area_pos_src, src.config_);

	for (int y = 0; y < copy_area.size.y; y++) {
		memcpy(dst_buf, src_buf, bytes_per_pixel * copy_area.size.x);
		dst_buf += BytesPerScanLine(config_);
		src_buf += BytesPerScanLine(src.config_);
	}

	return MAKE_ERROR(Error::kSuccess);
}
	
void FrameBuffer::Move(Vector2D<int> dst_pos, const Rectangle<int>& src) {
	const auto bytes_per_pixel = BytesPerPixel(config_.pixel_format);
	const auto bytes_per_scan_line = BytesPerScanLine(config_);

	// if move down and partially overlap, src will be partially overlaped in this way. so use if-else.  
	if (dst_pos.y < src.pos.y) {
		uint8_t* dst_buf = FrameBufferAddrAt(dst_pos, config_);
		const uint8_t* src_buf = FrameBufferAddrAt(src.pos, config_);

		for (int y = 0; y < src.size.y; ++y) {
			// use bytes_per_pixel * size.x as parmeter of memcpy to copy only source area.
			memcpy(dst_buf, src_buf, bytes_per_pixel * src.size.x);
			// use bytes_per_scan_line to represent next line of frame buffer config.
			dst_buf += bytes_per_scan_line;
			src_buf += bytes_per_scan_line;
		}
	} else {
		// the reason why use src.size.y - 1 is that we want first address of last line, not end of rectangle.
		uint8_t* dst_buf = FrameBufferAddrAt(dst_pos + Vector2D<int>{0, src.size.y - 1}, config_);
		const uint8_t* src_buf = FrameBufferAddrAt(src.pos + Vector2D<int>{0, src.size.y - 1}, config_);

		for (int y =0; y < src.size.y; ++y) {
			memcpy(dst_buf, src_buf, bytes_per_pixel * src.size.x);
			dst_buf -= bytes_per_scan_line;
			src_buf -= bytes_per_scan_line;
		}	
	}
}
