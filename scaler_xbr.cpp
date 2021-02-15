
#include "scaler.h"

static const bool kLut = false;

static bool _rgb_yuv_init;
static uint32_t _rgb_yuv[1 << 18];

static uint8_t _yuv[256 * 3];

static uint32_t yuv_diff(uint32_t x, uint32_t y) {
	if (1) {
		const int dy = _yuv[x * 3]     - _yuv[y * 3];
		const int du = _yuv[x * 3 + 1] - _yuv[y * 3 + 1];
		const int dv = _yuv[x * 3 + 2] - _yuv[y * 3 + 2];
		return ABS(dy) + ABS(du) + ABS(dv);
	}
	if (kLut) {
		const int yuv1 = _rgb_yuv[((x >> 2) & 0x3F) | ((x >> 4) & (0x3F << 6)) | ((x >> 6) & (0x3F << 12))];
		const int yuv2 = _rgb_yuv[((y >> 2) & 0x3F) | ((y >> 4) & (0x3F << 6)) | ((y >> 6) & (0x3F << 12))];
		return ABS((yuv1 >> 16) - (yuv2 >> 16)) + ABS(((yuv1 >> 8) & 255) - ((yuv2 >> 8) & 255)) + ABS((yuv1 & 255) - (yuv2 & 255));
	}
	int r, g, b;
	r = (x >> 16) & 255;
	g = (x >>  8) & 255;
	b =  x        & 255;
	int y1 = ( 299 * r + 587 * g + 114 * b) / 1000;
	int u1 = (-169 * r - 331 * g + 500 * b) / 1000; // + 128;
	int v1 = ( 500 * r - 419 * g -  81 * b) / 1000; // + 128;
	r = (y >> 16) & 255;
	g = (y >>  8) & 255;
	b =  y        & 255;
	int y2 = ( 299 * r + 587 * g + 114 * b) / 1000;
	int u2 = (-169 * r - 331 * g + 500 * b) / 1000; // + 128;
	int v2 = ( 500 * r - 419 * g -  81 * b) / 1000; // + 128;
	return ABS(y1 - y2) + ABS(u1 - u2) + ABS(v1 - v2);
}

template <int M, int S>
static uint32_t interpolate(uint32_t a, uint32_t b) {
	static const uint32_t kMask = 0xFF00FF;

	const uint32_t m1 = (((((b & kMask) - (a & kMask)) * M) >> S) + (a & kMask)) & kMask;
	a >>= 8;
	b >>= 8;
	const uint32_t m2 = (((((b & kMask) - (a & kMask)) * M) >> S) + (a & kMask)) & kMask;
	return m1 | (m2 << 8);
}

#define COLOR(x) palette[x]

#define ALPHA_BLEND_32_W(a, b)  interpolate<1,3>(a, b)
#define ALPHA_BLEND_64_W(a, b)  interpolate<1,2>(a, b)
#define ALPHA_BLEND_128_W(a, b) interpolate<1,1>(a, b)
#define ALPHA_BLEND_192_W(a, b) interpolate<3,2>(a, b)
#define ALPHA_BLEND_224_W(a, b) interpolate<7,3>(a, b)

#define df(A, B) yuv_diff(A, B)
#define eq(A, B) (df(A, B) < 155)

#define filt2b(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
    if (COLOR(PE) != COLOR(PH) && COLOR(PE) != COLOR(PF)) {                                                                     \
        const unsigned e = df(PE,PC) + df(PE,PG) + df(PI,H5) + df(PI,F4) + (df(PH,PF)<<2);          \
        const unsigned i = df(PH,PD) + df(PH,I5) + df(PF,I4) + df(PF,PB) + (df(PE,PI)<<2);          \
        if (e <= i) {                                                                               \
            const unsigned px = df(PE,PF) <= df(PE,PH) ? COLOR(PF) : COLOR(PH);                                   \
            if (e < i && ( (!eq(PF,PB) && !eq(PH,PD)) || (eq(PE,PI) && (!eq(PF,I4) && !eq(PH,I5))) || eq(PE,PG) || eq(PE,PC)) ) { \
                const unsigned ke = df(PF,PG);                                                      \
                const unsigned ki = df(PH,PC);                                                      \
                const bool left   = ke<<1 <= ki && PE != PG && PD != PG;                            \
                const bool up     = ke >= ki<<1 && PE != PC && PB != PC;                            \
                if (left && up) {                                                                   \
                    E[N3] = ALPHA_BLEND_224_W(E[N3], px);                                           \
                    E[N2] = ALPHA_BLEND_64_W( E[N2], px);                                           \
                    E[N1] = E[N2];                                                                  \
                } else if (left) {                                                                  \
                    E[N3] = ALPHA_BLEND_192_W(E[N3], px);                                           \
                    E[N2] = ALPHA_BLEND_64_W( E[N2], px);                                           \
                } else if (up) {                                                                    \
                    E[N3] = ALPHA_BLEND_192_W(E[N3], px);                                           \
                    E[N1] = ALPHA_BLEND_64_W( E[N1], px);                                           \
                } else {                                                                            \
                    E[N3] = ALPHA_BLEND_128_W(E[N3], px);                                           \
                }                                                                                   \
            } else {                                                                                \
                E[N3] = ALPHA_BLEND_128_W(E[N3], px);                                               \
            }                                                                                       \
        }                                                                                           \
    }                                                                                               \
} while (0)

#define filt3a(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3, N4, N5, N6, N7, N8) do { \
    if (COLOR(PE) != COLOR(PH) && COLOR(PE) != COLOR(PF)) {                                                                     \
        const unsigned e = df(PE,PC) + df(PE,PG) + df(PI,H5) + df(PI,F4) + (df(PH,PF)<<2);          \
        const unsigned i = df(PH,PD) + df(PH,I5) + df(PF,I4) + df(PF,PB) + (df(PE,PI)<<2);          \
        if (e <= i) {                                                                               \
            const unsigned px = df(PE,PF) <= df(PE,PH) ? COLOR(PF) : COLOR(PH);                                   \
            if (e < i && ( (!eq(PF,PB) && !eq(PF,PC)) || (!eq(PH,PD) && !eq(PH,PG)) || ((eq(PE,PI) && (!eq(PF,F4) && !eq(PF,I4))) || (!eq(PH,H5) && !eq(PH,I5))) || eq(PE,PG) || eq(PE,PC)) ) {\
                const unsigned ke = df(PF,PG);                                                      \
                const unsigned ki = df(PH,PC);                                                      \
                const bool left   = ke<<1 <= ki && COLOR(PE) != COLOR(PG) && COLOR(PD) != COLOR(PG);                            \
                const bool up     = ke >= ki<<1 && COLOR(PE) != COLOR(PC) && COLOR(PB) != COLOR(PC);                            \
                if (left && up) {                                                                   \
                    E[N7] = ALPHA_BLEND_192_W(E[N7], px);                                           \
                    E[N6] = ALPHA_BLEND_64_W( E[N6], px);                                           \
                    E[N5] = E[N7];                                                                  \
                    E[N2] = E[N6];                                                                  \
                    E[N8] = px;                                                                     \
                } else if (left) {                                                                  \
                    E[N7] = ALPHA_BLEND_192_W(E[N7], px);                                           \
                    E[N5] = ALPHA_BLEND_64_W( E[N5], px);                                           \
                    E[N6] = ALPHA_BLEND_64_W( E[N6], px);                                           \
                    E[N8] = px;                                                                     \
                } else if (up) {                                                                    \
                    E[N5] = ALPHA_BLEND_192_W(E[N5], px);                                           \
                    E[N7] = ALPHA_BLEND_64_W( E[N7], px);                                           \
                    E[N2] = ALPHA_BLEND_64_W( E[N2], px);                                           \
                    E[N8] = px;                                                                     \
                } else {                                                                            \
                    E[N8] = ALPHA_BLEND_224_W(E[N8], px);                                           \
                    E[N5] = ALPHA_BLEND_32_W( E[N5], px);                                           \
                    E[N7] = ALPHA_BLEND_32_W( E[N7], px);                                           \
                }                                                                                   \
            } else {                                                                                \
                E[N8] = ALPHA_BLEND_128_W(E[N8], px);                                               \
            }                                                                                       \
        }                                                                                           \
    }                                                                                               \
} while (0)

#define filt4b(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N15, N14, N11, N3, N7, N10, N13, N12, N9, N6, N2, N1, N5, N8, N4, N0) do { \
    if (COLOR(PE) != COLOR(PH) && COLOR(PE) != COLOR(PF)) {                                                                     \
        const unsigned e = df(PE,PC) + df(PE,PG) + df(PI,H5) + df(PI,F4) + (df(PH,PF)<<2);          \
        const unsigned i = df(PH,PD) + df(PH,I5) + df(PF,I4) + df(PF,PB) + (df(PE,PI)<<2);          \
        if (e <= i) {                                                                               \
            const unsigned px = df(PE,PF) <= df(PE,PH) ? COLOR(PF) : COLOR(PH);                                   \
            if (e < i && ( (!eq(PF,PB) && !eq(PH,PD)) || (eq(PE,PI) && (!eq(PF,I4) && !eq(PH,I5))) || eq(PE,PG) || eq(PE,PC)) ) {\
                const unsigned ke = df(PF,PG);                                                      \
                const unsigned ki = df(PH,PC);                                                      \
                const bool left   = ke<<1 <= ki && PE != PG && PD != PG;                            \
                const bool up     = ke >= ki<<1 && PE != PC && PB != PC;                            \
                if (left && up) {                                                                   \
                    E[N13] = ALPHA_BLEND_192_W(E[N13], px);                                         \
                    E[N12] = ALPHA_BLEND_64_W( E[N12], px);                                         \
                    E[N15] = E[N14] = E[N11] = px;                                                  \
                    E[N10] = E[N3]  = E[N12];                                                       \
                    E[N7]  = E[N13];                                                                \
                } else if (left) {                                                                  \
                    E[N11] = ALPHA_BLEND_192_W(E[N11], px);                                         \
                    E[N13] = ALPHA_BLEND_192_W(E[N13], px);                                         \
                    E[N10] = ALPHA_BLEND_64_W( E[N10], px);                                         \
                    E[N12] = ALPHA_BLEND_64_W( E[N12], px);                                         \
                    E[N14] = px;                                                                    \
                    E[N15] = px;                                                                    \
                } else if (up) {                                                                    \
                    E[N14] = ALPHA_BLEND_192_W(E[N14], px);                                         \
                    E[N7 ] = ALPHA_BLEND_192_W(E[N7 ], px);                                         \
                    E[N10] = ALPHA_BLEND_64_W( E[N10], px);                                         \
                    E[N3 ] = ALPHA_BLEND_64_W( E[N3 ], px);                                         \
                    E[N11] = px;                                                                    \
                    E[N15] = px;                                                                    \
                } else {                                                                            \
                    E[N11] = ALPHA_BLEND_128_W(E[N11], px);                                         \
                    E[N14] = ALPHA_BLEND_128_W(E[N14], px);                                         \
                    E[N15] = px;                                                                    \
                }                                                                                   \
            } else {                                                                                \
                E[N15] = ALPHA_BLEND_128_W(E[N15], px);                                             \
            }                                                                                       \
        }                                                                                           \
    }                                                                                               \
} while (0)

template <int N>
static void scaleImpl(uint32_t *dst, int dstPitch, const uint8_t *src, int srcPitch, int w, int h, const uint32_t *palette) {
	const int nl = dstPitch;
	const int nl1 = nl + nl;
	const int nl2 = nl1 + nl;

	for (int y = 0; y < h; ++y) {

		uint32_t *E = dst + y * dstPitch * N;

		const uint8_t *sa2 = src + y * srcPitch - 2;
		const uint8_t *sa1 = sa2 - srcPitch;
		const uint8_t *sa0 = sa1 - srcPitch;
		const uint8_t *sa3 = sa2 + srcPitch;
		const uint8_t *sa4 = sa3 + srcPitch;

		if (y <= 1) {
			sa0 = sa1;
			if (y == 0) {
				sa0 = sa1 = sa2;
			}
		} else if (y >= h - 2) {
			sa4 = sa3;
			if (y == h - 1) {
				sa4 = sa3 = sa2;
			}
		}

		for (int x = 0; x < w; ++x) {

			//    A1 B1 C1
			// A0 PA PB PC C4
			// D0 PD PE PF F4
			// G0 PG PH PI I4
			//    G5 H5 I5

            const uint32_t B1 = sa0[2];
            const uint32_t PB = sa1[2];
            const uint32_t PE = sa2[2];
            const uint32_t PH = sa3[2];
            const uint32_t H5 = sa4[2];

            const int pprev = 2 - (x > 0);
            const uint32_t A1 = sa0[pprev];
            const uint32_t PA = sa1[pprev];
            const uint32_t PD = sa2[pprev];
            const uint32_t PG = sa3[pprev];
            const uint32_t G5 = sa4[pprev];

            const int pprev2 = pprev - (x > 1);
            const uint32_t A0 = sa1[pprev2];
            const uint32_t D0 = sa2[pprev2];
            const uint32_t G0 = sa3[pprev2];

            const int pnext = 3 - (x == w - 1);
            const uint32_t C1 = sa0[pnext];
            const uint32_t PC = sa1[pnext];
            const uint32_t PF = sa2[pnext];
            const uint32_t PI = sa3[pnext];
            const uint32_t I5 = sa4[pnext];

            const int pnext2 = pnext + 1 - (x >= w - 2);
            const uint32_t C4 = sa1[pnext2];
            const uint32_t F4 = sa2[pnext2];
            const uint32_t I4 = sa3[pnext2];

			for (int j = 0; j < N; ++j) {
				for (int i = 0; i < N; ++i) {
					*(E + j * dstPitch + i) = COLOR(PE);
				}
			}


            if (N == 2) {
                filt2b(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, 0, 1, nl, nl+1);
                filt2b(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, nl, 0, nl+1, 1);
                filt2b(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, nl+1, nl, 1, 0);
                filt2b(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, 1, nl+1, 0, nl);
            } else if (N == 3) {
                filt3a(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, 0, 1, 2, nl, nl+1, nl+2, nl1, nl1+1, nl1+2);
                filt3a(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, nl1, nl, 0, nl1+1, nl+1, 1, nl1+2, nl+2, 2);
                filt3a(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, nl1+2, nl1+1, nl1, nl+2, nl+1, nl, 2, 1, 0);
                filt3a(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, 2, nl+2, nl1+2, 1, nl+1, nl1+1, 0, nl, nl1);
            } else if (N == 4) {
                filt4b(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, nl2+3, nl2+2, nl1+3, 3, nl+3, nl1+2, nl2+1, nl2, nl1+1, nl+2, 2, 1, nl+1, nl1, nl, 0);
                filt4b(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, 3, nl+3, 2, 0, 1, nl+2, nl1+3, nl2+3, nl1+2, nl+1, nl, nl1, nl1+1, nl2+2, nl2+1, nl2);
                filt4b(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, 0, 1, nl, nl2, nl1, nl+1, 2, 3, nl+2, nl1+1, nl2+1, nl2+2, nl1+2, nl+3, nl1+3, nl2+3);
                filt4b(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, nl2, nl1, nl2+1, nl2+3, nl2+2, nl1+1, nl, 0, nl+1, nl1+2, nl1+3, nl+3, nl+2, 1, 2, 3);
            }

			++sa0;
			++sa1;
			++sa2;
			++sa3;
			++sa4;

			E += N;
		}
	}
}

static void scale_xbr(int factor, uint32_t *dst, int dstPitch, const uint8_t *src, int srcPitch, int w, int h, const uint32_t *palette) {
	if (kLut && !_rgb_yuv_init) {
		for (int r = 0; r < 0x40; ++r) {
			for (int g = 0; g < 0x40; ++g) {
				for (int b = 0; b < 0x40; ++b) {
					const int y = ( 299 * r + 587 * g + 114 * b) * 4 / 1000;
					const int u = (-169 * r - 331 * g + 500 * b) * 4 / 1000 + 128;
					const int v = ( 500 * r - 419 * g -  81 * b) * 4 / 1000 + 128;
					_rgb_yuv[(r << 12) | (g << 6) | b] = (y << 16) | (u << 8) | v;
				}
			}
		}
		_rgb_yuv_init = true;
	}
	switch (factor) {
	case 2:
		scaleImpl<2>(dst, dstPitch, src, srcPitch, w, h, palette);
		break;
	case 3:
		scaleImpl<3>(dst, dstPitch, src, srcPitch, w, h, palette);
		break;
	case 4:
		scaleImpl<4>(dst, dstPitch, src, srcPitch, w, h, palette);
		break;
	}
}

static void palette_xbr(const uint32_t *palette) {
	for (int i = 0; i < 256; ++i) {
		const int r = (palette[i] >> 16) & 255;
		const int g = (palette[i] >>  8) & 255;
		const int b =  palette[i]        & 255;
		_yuv[i * 3]     = ( 299 * r + 587 * g + 114 * b) / 1000;
		_yuv[i * 3 + 1] = (-169 * r - 331 * g + 500 * b) / 1000 + 128;
		_yuv[i * 3 + 2] = ( 500 * r - 419 * g -  81 * b) / 1000 + 128;
	}
}

const Scaler scaler_xbr = {
	"xbr",
	2, 4,
	scale_xbr,
	palette_xbr
};
