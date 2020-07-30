#pragma once

using ruminoid_image_t = struct {
	int width, height, stride;
	unsigned char* buffer;
};

using ruminoid_rendercore_t = void*;

