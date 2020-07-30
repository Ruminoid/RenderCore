#include "ruminoid_rendercore.h"

#include <memory>
extern "C" {
#include <ass.h>
#include <lz4.h>
}

class ruminoid_libass_context
{
private:
	ASS_Library* _library;
	ASS_Renderer* _renderer = nullptr;
	ASS_Track* _ass_track = nullptr;
	ruminoid_rc_log_callback _log_callback = nullptr;

	size_t _current_size = 0;
	size_t _current_compress_capacity = 0;
	size_t _current_capacity = 0;
	std::unique_ptr<unsigned char[]> _frame_buf = nullptr;
	std::unique_ptr<unsigned char[]> _compress_buf = nullptr;
	int _stride = 0;

	ruminoid_image_t _out_image { 0, 0, 0, nullptr, 0 };

public:
	ruminoid_libass_context();
	~ruminoid_libass_context();

	bool load_subtitle(const char* content, size_t len);
	void render(int width, int height, int timeMs);
	ruminoid_image_t* result() { return &_out_image; }

	void prepare_buf(int width, int height)
	{
		_stride = ((width * 32 + 31) & ~31) / 8;
		_current_size = _stride * height;
		_current_compress_capacity = _current_size * 3;

		if (!_frame_buf || _current_capacity < _current_size)
		{
			_frame_buf.reset(new unsigned char[_current_size]);
			_compress_buf.reset(new unsigned char[_current_size]);
			_current_capacity = _current_size;
		}

		memset(_frame_buf.get(), 0x00, _current_size);

		_out_image.width = width;
		_out_image.height = height;
		_out_image.buffer = _compress_buf.get();
		_out_image.stride = _stride;
	}

	void attach_log_callback(ruminoid_rc_log_callback log_callback) { _log_callback = log_callback; }
	void log(int level, const char* msg) { if (_log_callback) _log_callback(level, msg); }
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

	_renderer = ass_renderer_init(_library);
	ass_set_font_scale(_renderer, 1.);
	ass_set_fonts(_renderer, nullptr, "Sans", 1, nullptr, true);
}

ruminoid_libass_context::~ruminoid_libass_context()
{
	if (_ass_track)
		ass_free_track(_ass_track);
	ass_renderer_done(_renderer);
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

#define _r(c) ((c)>>24)
#define _g(c) (((c)>>16)&0xFF)
#define _b(c) (((c)>>8)&0xFF)
#define _a(c) ((c)&0xFF)
void ruminoid_libass_context::render(int width, int height, int timeMs)
{
	prepare_buf(width, height);

	unsigned char* dst = _frame_buf.get();
	ass_set_frame_size(_renderer, width, height);
	ASS_Image* img = ass_render_frame(_renderer, _ass_track, timeMs, nullptr);

	for (; img; img = img->next) {
		int dstStride = img->stride;
		int dst_x = img->dst_x;
		int dst_y = img->dst_y;
		auto opacity = 0xFF - static_cast<unsigned char>(_a(img->color));
		auto r = (unsigned char)_r(img->color);
		auto g = (unsigned char)_g(img->color);
		auto b = (unsigned char)_b(img->color);

		for (int y = 0; y < img->h && y + img->dst_y < height; y++)
		{
			for (int x = 0; x < img->w && x + img->dst_x < width; x++)
			{
				int blank_current_pixel = (y + dst_y) * _stride + (x + dst_x) * 4;
				int ass_current_pixel = y * dstStride + x;
				unsigned int a = unsigned(img->bitmap[ass_current_pixel]);
				unsigned int k = a * opacity / 255;
				unsigned int ck = 255 - k;

				dst[blank_current_pixel] = (k * r + ck * dst[blank_current_pixel]) / 255;
				dst[blank_current_pixel + 1] = (k * g + ck * dst[blank_current_pixel + 1]) / 255;
				dst[blank_current_pixel + 2] = (k * b + ck * dst[blank_current_pixel + 2]) / 255;
				dst[blank_current_pixel + 3] = (k + ck * dst[blank_current_pixel + 3] / 255);
			}
		}
	}

	auto compressed_size = LZ4_compress_default(
		reinterpret_cast<const char*>(dst),
		reinterpret_cast<char*>(_compress_buf.get()),
		_current_size, _current_compress_capacity);
	_out_image.compressed_size = compressed_size;
}

extern "C" {

ruminoid_rc_context_t ruminoid_rc_new_context()
{
	return new ruminoid_libass_context;
}

void ruminoid_rc_destroy_context(ruminoid_rc_context_t context)
{
	delete reinterpret_cast<ruminoid_libass_context*>(context);
}

void ruminoid_rc_render_frame(ruminoid_rc_context_t opaque_context, int width, int height, int timeMs)
{
	auto context = reinterpret_cast<ruminoid_libass_context*>(opaque_context);
	context->render(width, height, timeMs);
}

bool ruminoid_rc_update_subtitle(ruminoid_rc_context_t opaque_context, const char* content, size_t length)
{
	auto context = reinterpret_cast<ruminoid_libass_context*>(opaque_context);
	return context->load_subtitle(content, length);
}

ruminoid_image_t* ruminoid_rc_get_result(ruminoid_rc_context_t opaque_context)
{
	auto context = reinterpret_cast<ruminoid_libass_context*>(opaque_context);
	return context->result();
}

void ruminoid_rc_attach_log_callback(ruminoid_rc_context_t opaque_context, ruminoid_rc_log_callback callback)
{
	auto context = reinterpret_cast<ruminoid_libass_context*>(opaque_context);
	context->attach_log_callback(callback);
}

}
