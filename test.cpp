#include "ruminoid_rendercore.h"
#include <fstream>
#include <filesystem>

int main()
{
    auto ctxt = ruminoid_rc_new_context();
	auto length = std::filesystem::file_size("E:\\A_Main.ass");
	char* rdb = new char[length];
    std::ifstream ifs("E:\\A_Main.ass");
	ifs.read(rdb, length);
	ruminoid_rc_update_subtitle(ctxt, rdb, length);
	ruminoid_rc_render_frame(ctxt, 1920, 1080, 46000);
	auto result = ruminoid_rc_get_result(ctxt);
	ruminoid_rc_destroy_context(ctxt);
	return 0;
}
