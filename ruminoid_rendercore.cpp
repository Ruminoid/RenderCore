#include "ruminoid_rendercore.h"

#include <memory>
extern "C" {
#include <ass.h>
#include <lz4.h>
}

class ruminoid_libass_render_context;
class ruminoid_libass_context
{
	friend class ruminoid_libass_render_context;
private:
	ASS_Library* _library;
	ASS_Track* _ass_track = nullptr;
	ruminoid_rc_log_callback _log_callback = nullptr;

public:
	ruminoid_libass_context();
	~ruminoid_libass_context();

	bool load_subtitle(const char* content, size_t len);
	ruminoid_rc_render_context_t create_render_context();

	void attach_log_callback(ruminoid_rc_log_callback log_callback) { _log_callback = log_callback; }
	void log(int level, const char* msg) { if (_log_callback) _log_callback(level, msg); }
};

class ruminoid_libass_render_context
{
private:
	ruminoid_libass_context* _context;
	ASS_Renderer* _renderer = nullptr;

	size_t _current_size = 0;
	size_t _current_compress_capacity = 0;
	size_t _current_capacity = 0;
	std::unique_ptr<unsigned int[]> _frame_buf = nullptr;
	std::unique_ptr<unsigned char[]> _compress_buf = nullptr;
	int _stride = 0;

	ruminoid_image_t _out_image{ 0, 0, 0, nullptr, 0 };

	void prepare_buf(int width, int height);

public:
	ruminoid_libass_render_context(ruminoid_libass_context* context, ASS_Renderer* renderer);
	~ruminoid_libass_render_context();

	void blend(int width, int height, ASS_Image* image);
	void render(int width, int height, int timeMs);
	void set_limits(int glyph_max, int bitmap_max);
	ruminoid_image_t* result() { return &_out_image; }
};

void msg_callback(int level, const char* fmt, va_list args, void* opaque) {
	auto context = reinterpret_cast<ruminoid_libass_context*>(opaque);

	if (level >= 7) return;
	char buf[1024];
#ifdef _WIN32
	vsprintf_s(buf, sizeof(buf), fmt, args);
#else
	vsnprintf(buf, sizeof(buf), fmt, args);
#endif

	context->log(level, buf);
}

ruminoid_libass_context::ruminoid_libass_context()
{
	_library = ass_library_init();
	ass_set_message_cb(_library, msg_callback, this);
}

ruminoid_libass_context::~ruminoid_libass_context()
{
	if (_ass_track)
		ass_free_track(_ass_track);
	ass_library_done(_library);
}

bool ruminoid_libass_context::load_subtitle(const char* content, size_t len)
{
	if (_ass_track) ass_free_track(_ass_track);
	_ass_track = ass_read_memory(_library, const_cast<char*>(content), len, nullptr);
	if (!_ass_track)
		return false;
	return true;
}

ruminoid_rc_render_context_t ruminoid_libass_context::create_render_context()
{
	return new ruminoid_libass_render_context(this, ass_renderer_init(_library));
}

void blend_single(unsigned int* dst, int dst_stride, int w, int h, int color, unsigned char* src, int src_stride, int dst_x, int dst_y, int src_w, int src_h);

ruminoid_libass_render_context::ruminoid_libass_render_context(ruminoid_libass_context *context, ASS_Renderer* renderer)
	: _context(context), _renderer(renderer)
{
	ass_set_font_scale(_renderer, 1.);
	ass_set_fonts(_renderer, nullptr, "Sans", 1, nullptr, true);
}

ruminoid_libass_render_context::~ruminoid_libass_render_context()
{
	ass_renderer_done(_renderer);
}

#include <iostream>
void ruminoid_libass_render_context::blend(int width, int height, ASS_Image* image)
{
	auto dst = _frame_buf.get();
	while (image)
	{
		blend_single(
			dst, _stride, width, height,
			image->color,
			image->bitmap, image->stride, image->dst_x, image->dst_y, image->w, image->h);

		image = image->next;
	}
}

void ruminoid_libass_render_context::render(int width, int height, int timeMs)
{
	prepare_buf(width, height);

	ass_set_frame_size(_renderer, width, height);
	ASS_Image* img = ass_render_frame(_renderer, _context->_ass_track, timeMs, nullptr);

	blend(width, height, img);

	auto compressed_size = LZ4_compress_default(
		reinterpret_cast<const char*>(_frame_buf.get()),
		reinterpret_cast<char*>(_compress_buf.get()),
		_current_size, _current_compress_capacity);
	_out_image.compressed_size = compressed_size;
}

void ruminoid_libass_render_context::set_limits(int glyph_max, int bitmap_max)
{
	ass_set_cache_limits(_renderer, glyph_max, bitmap_max);
}

std::align_val_t buffer_align = static_cast<std::align_val_t>(32);
void ruminoid_libass_render_context::prepare_buf(int width, int height)
{
	_stride = ((width * 32 + 31) & ~31) / 8;
	_current_size = _stride * height;
	_current_compress_capacity = size_t(_current_size * 1.5);

	if (!_frame_buf || _current_capacity < _current_size)
	{
		_frame_buf.reset(new unsigned int[_current_size >> 2]);
		_compress_buf.reset(new unsigned char[_current_compress_capacity]);
		_current_capacity = _current_size;
	}

	memset(_frame_buf.get(), 0x00, _current_size);

	_out_image.width = width;
	_out_image.height = height;
	_out_image.buffer = _compress_buf.get();
	_out_image.stride = _stride;
}

//#include <iostream>

extern "C" {

ruminoid_rc_context_t ruminoid_rc_new_context()
{
	return new ruminoid_libass_context;
}

void ruminoid_rc_destroy_context(ruminoid_rc_context_t context)
{
	delete reinterpret_cast<ruminoid_libass_context*>(context);
	//std::cerr << "Blend took " << std::chrono::duration_cast<std::chrono::microseconds>(dur).count() << " us";
}

bool ruminoid_rc_update_subtitle(ruminoid_rc_context_t opaque_context, const char* content, size_t length)
{
	auto context = reinterpret_cast<ruminoid_libass_context*>(opaque_context);
	return context->load_subtitle(content, length);
}

void ruminoid_rc_attach_log_callback(ruminoid_rc_context_t opaque_context, ruminoid_rc_log_callback callback)
{
	auto context = reinterpret_cast<ruminoid_libass_context*>(opaque_context);
	context->attach_log_callback(callback);
}

ruminoid_rc_render_context_t ruminoid_rc_new_render_context(ruminoid_rc_context_t opaque_context)
{
	auto context = reinterpret_cast<ruminoid_libass_context*>(opaque_context);
	return context->create_render_context();
}

void ruminoid_rc_destroy_render_context(ruminoid_rc_render_context_t render_context)
{
	delete reinterpret_cast<ruminoid_libass_render_context*>(render_context);
}

void ruminoid_rc_render_frame(ruminoid_rc_render_context_t opaque_context, int width, int height, int timeMs)
{
	auto context = reinterpret_cast<ruminoid_libass_render_context*>(opaque_context);
	context->render(width, height, timeMs);
}

ruminoid_image_t* ruminoid_rc_get_result(ruminoid_rc_render_context_t opaque_context)
{
	auto context = reinterpret_cast<ruminoid_libass_render_context*>(opaque_context);
	return context->result();
}

void ruminoid_rc_set_cache_limits(ruminoid_rc_render_context_t opaque_context, int glyph_max, int bitmap_max)
{
	auto context = reinterpret_cast<ruminoid_libass_render_context*>(opaque_context);
	context->set_limits(glyph_max, bitmap_max);
}

}
