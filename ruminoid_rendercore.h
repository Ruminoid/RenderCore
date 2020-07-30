#pragma once

#ifdef RUMINOID_RC_DLL
#define RUMINOID_API __declspec(dllexport)
#else
#define RUMINOID_API __declspec(dllimport)
#endif

#define RUMINOID_CALL __stdcall

#ifdef __cplusplus
extern "C" {
#endif
using ruminoid_image_t = struct {
	int width, height, stride;
	unsigned char* buffer;
	size_t compressed_size;
};

using ruminoid_rc_context_t = void*;
RUMINOID_API ruminoid_rc_context_t RUMINOID_CALL ruminoid_rc_new_context();
RUMINOID_API void                  RUMINOID_CALL ruminoid_rc_destroy_context(ruminoid_rc_context_t context);

RUMINOID_API void                  RUMINOID_CALL ruminoid_rc_render_frame(ruminoid_rc_context_t context, int width, int height, int timeMs);
RUMINOID_API bool                  RUMINOID_CALL ruminoid_rc_update_subtitle(ruminoid_rc_context_t context, const char* content, size_t length);
RUMINOID_API ruminoid_image_t*     RUMINOID_CALL ruminoid_rc_get_result(ruminoid_rc_context_t context);

using ruminoid_rc_log_callback = void (*)(int, const char*);
RUMINOID_API void                  RUMINOID_CALL ruminoid_rc_attach_log_callback(ruminoid_rc_context_t context, ruminoid_rc_log_callback callback);
#ifdef __cplusplus
}
#endif