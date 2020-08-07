#include "ruminoid_rendercore.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <libpng16/png.h>
#include <lz4.h>

typedef struct image_s {
	int width, height, stride;
	unsigned char* buffer;      // RGBA32
} image_t;

static void write_png(const char* fname, image_t* img)
{
	FILE* fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_byte** row_pointers;
	int k;

	png_ptr =
		png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	fp = NULL;

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return;
	}

	errno_t err = fopen_s(&fp, fname, "wb");
	if (fp == NULL) {
		printf("PNG Error opening %s for writing!\n", fname);
		return;
	}

	png_init_io(png_ptr, fp);
	png_set_compression_level(png_ptr, 0);

	png_set_IHDR(png_ptr, info_ptr, img->width, img->height,
		8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	//png_set_bgr(png_ptr);

	row_pointers = (png_byte**)malloc(img->height * sizeof(png_byte*));
	for (k = 0; k < img->height; k++)
		row_pointers[k] = img->buffer + img->stride * k;

	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(row_pointers);

	fclose(fp);
}

int main()
{
    auto ctxt = ruminoid_rc_new_context();
	auto length = std::filesystem::file_size("E:\\A_Main.ass");
	char* rdb = new char[length];
    std::ifstream ifs("E:\\A_Main.ass");
	ifs.read(rdb, length);
	ruminoid_rc_update_subtitle(ctxt, rdb, length);

	auto rctxt = ruminoid_rc_new_render_context(ctxt);
	ruminoid_rc_set_cache_limits(rctxt, 512, 32);

	auto start = std::chrono::high_resolution_clock::now();
	size_t total_siz = 0;
	//ruminoid_rc_render_frame(rctxt, 1280, 720, 42000);

	//for (int i = 0; i < 160; i++)
	//{
	//	ruminoid_rc_render_frame(rctxt, 1280, 720, i * 500);
	//}
	ruminoid_rc_render_frame(rctxt, 1280, 720, 80000);
	auto rst = ruminoid_rc_get_result(rctxt);
	auto rstbmp = new unsigned char[rst->height * rst->stride];
	LZ4_decompress_safe(reinterpret_cast<char*>(rst->buffer), reinterpret_cast<char*>(rstbmp), rst->compressed_size, rst->height * rst->stride);
	image_t im{ rst->width, rst->height, rst->stride, rstbmp };
	write_png("test.png", &im);
	auto end = std::chrono::high_resolution_clock::now();
	auto tms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cerr << tms << "us, " << total_siz << " bytes" << std::endl;
	ruminoid_rc_destroy_render_context(rctxt);
	ruminoid_rc_destroy_context(ctxt);
	return 0;
}
