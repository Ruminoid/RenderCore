#include "ruminoid_rendercore.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iostream>

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
	for (int i = 0; i < 120 * 60; i++)
	{
		ruminoid_rc_render_frame(rctxt, 1280, 720, i * 1000 / 60);
		total_siz += ruminoid_rc_get_result(rctxt)->compressed_size;
	}
	auto end = std::chrono::high_resolution_clock::now();
	auto tms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cerr << tms << "us, " << total_siz << " bytes" << std::endl;
	ruminoid_rc_destroy_render_context(rctxt);
	ruminoid_rc_destroy_context(ctxt);
	return 0;
}
