#include <algorithm>

#define _r(c) ((c)>>24)
#define _g(c) (((c)>>16)&0xFF)
#define _b(c) (((c)>>8)&0xFF)
#define _a(c) ((c)&0xFF)

#define _re(c) ((c)&0xFF)
#define _ge(c) (((c)>>8)&0xFF)
#define _be(c) (((c)>>16)&0xFF)
#define _ae(c) ((c>>24)&0xFF)
void blend_single(unsigned int* dst, int dst_stride, int w, int h, int color, unsigned char* src, int src_stride, int dst_x, int dst_y, int src_w, int src_h)
{
	auto opacity = 0xFF - (unsigned char)(_a(color));
	auto r       =        (unsigned char)(_r(color));
	auto g       =        (unsigned char)(_g(color));
	auto b       =        (unsigned char)(_b(color));

	src_w = std::min(w - dst_x, src_w);
	src_h = std::min(h - dst_y, src_h);

	unsigned int* dstp = reinterpret_cast<unsigned*>(dst) + size_t(dst_y) * (dst_stride >> 2) + dst_x;
	unsigned char* srcp = reinterpret_cast<unsigned char*>(src);

	int src_stride2 = src_stride - src_w;
	int dst_stride2 = (dst_stride >> 2) - src_w;

	for (int y = 0; y < src_h; y++, dstp += dst_stride2, srcp += src_stride2)
	{
		for (int x = 0; x < src_w; x++, dstp++, srcp++)
		{
			unsigned int k = unsigned(*srcp) * opacity / 0xFF;
			unsigned int ck = 255 - k;
			unsigned int di = *dstp;

			unsigned char finr = (k * r    + ck * _re(di)) / 0xFF;
			unsigned char fing = (k * g    + ck * _ge(di)) / 0xFF;
			unsigned char finb = (k * b    + ck * _be(di)) / 0xFF;
			unsigned char fina = 0xFF - ck * (0xFF - _ae(di)) / 0xFF;
			*dstp = (((fina & 0xFF) << 24) + ((finb & 0xFF) << 16) + ((fing & 0xFF) << 8) + (finr & 0xFF));
		}
	}
}

void blend_single_avx2(unsigned char* dst, int dst_stride, int w, int h, int color, unsigned char* src, int src_stride, int dst_x, int dst_y, int src_w, int src_h)
{

}
