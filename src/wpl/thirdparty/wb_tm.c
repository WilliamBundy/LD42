/* 
 * This file has been modified and extended by William Bundy
 * and differs significantly from the original.
 * Changes:
 * 	- Removed non-SSE2 path. All 64 bit computers have SSE2 now.
 * 	- Added wrappers making it easy to call with a single f32.
 * 	- Added a couple functions not present in the original.
 *	- Changed how constants are used; instead of storing them in global
 *		scope, we use _mm_set1_ps or _mm_set1_si32 instead
 *
 *	- TODO(Will) We are a few functions off from fully reimplementing math.h
 *		- missing: acos, asin
 *		- missing: cosh, sinh, tanh,
 *		- missing: ldexp, frexp, integer functions
 *		- missing: log10, modf
 *		- missing: ceil, floor, fmod
 *		- missing: fabs/abs... kinda
 *
 *	- TODO(will): C++11 (maybe C11 too?) also has some more functions
 *		- missing: acosh, asinh, atanh
 *		- missing a whole bunch of log stuff
 *		- missing: cbrt, hypot
 *		- missing a whole bunch of floating point manipulation stuff
 *		- 
*/

/* Original copyright and license headers
 * SIMD (SSE1+MMX or SSE2) implementation of sin, cos, exp and log

   Inspired by intel Approximate Math library, and based on the
   corresponding algorithms of the cephes math library

   The default is to use the SSE1 version. If you define USE_SSE2 the
   the SSE2 intrinsics will be used in place of the MMX intrinsics. Do
   not expect any significant performance improvement with SSE2.

   Copyright (C) 2007  Julien Pommier

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.

   (this is the zlib license)
*/

//#include <intrin.h>
#include <emmintrin.h>
#include <xmmintrin.h>

typedef __m128 vf128;
typedef __m128i vi128;
typedef float f32;
typedef int i32;

/*
#ifdef WB_STATIC_IMPLEMENTATION
#define WBTM_IMPLEMENTATION
#define WBTM_API static
#endif 

#ifndef WBTM_API
#ifndef WB_IMPLEMENTATION
#define WBTM_API extern
#else 
#define WBTM_API
#endif
#endif
*/

#ifndef WBTM_API
#define WBTM_API
#endif

WBTM_API vf128 wb_log_ps(vf128 x);
WBTM_API vf128 wb_exp_ps(vf128 x);
WBTM_API vf128 wb_sin_ps(vf128 x);
WBTM_API vf128 wb_cos_ps(vf128 x);
WBTM_API void wb_sincos_ps(vf128 x, vf128* s, vf128* c);
WBTM_API vf128 wb_atan2_ps(vf128 y, vf128 x);
WBTM_API vf128 wb_sqrt_ps(vf128 x);
WBTM_API vf128 wb_rsqrt_ps(vf128 x);
WBTM_API vf128 wb_ldexp_ps(vf128 x, vf128 e);
WBTM_API vf128 wb_pow_ps(vf128 x, vf128 e);
WBTM_API vf128 wb_min_ps(vf128 x, vf128 y);
WBTM_API vf128 wb_max_ps(vf128 x, vf128 y);
WBTM_API vf128 wb_clamp_ps(vf128 minx, vf128 maxx, vf128 value);
WBTM_API vf128 wb_abs_ps(vf128 x);

WBTM_API f32 wb_logf(f32 x);
WBTM_API f32 wb_expf(f32 x);
WBTM_API f32 wb_sinf(f32 x);
WBTM_API f32 wb_cosf(f32 x);
WBTM_API void wb_sincosf(f32 x, f32* s, f32* c);
WBTM_API f32 wb_tanf(f32 x);
WBTM_API f32 wb_atan2f(f32 y, f32 x);
WBTM_API f32 wb_rsqrtf(f32 x);
WBTM_API f32 wb_sqrtf(f32 x);
WBTM_API f32 wb_ldexpf(f32 x, int e);
WBTM_API f32 wb_ipowf(f32 x, int e);
WBTM_API f32 wb_powf(f32 x, f32 y);
WBTM_API i32 wb_floorf(f32 x);
WBTM_API i32 wb_ceilf(f32 x);
WBTM_API i32 wb_roundf(f32 x);
WBTM_API f32 wb_minf(f32 x, f32 y);
WBTM_API f32 wb_maxf(f32 x, f32 y);
WBTM_API f32 wb_clampf(f32 low, f32 high, f32 value);
WBTM_API f32 wb_absf(f32 x);
WBTM_API i32 wb_abs(i32 x);

#ifdef WBTM_CRT_REPLACE
#define cos wb_cosf
#define sin wb_sinf
#define atan2 wb_atan2
#define exp wb_expf
#define log wb_logf
#define ldexp wb_ldexpf
#define pow wb_powf
#define floor wb_floorf
#define ceil wb_ceilf
#define round wb_roundf
#define fabs wb_absf
#define clamp wb_clampf
#define sqrt wb_sqrtf
#define rsqrt wb_rsqrtf

#define cosf wb_cosf
#define sinf wb_sinf
#define expf wb_expf
#define atan2f wb_atan2f
#define logf wb_logf
#define ldexpf wb_ldexpf
#define powf wb_powf
#define floorf wb_floorf
#define ceilf wb_ceilf
#define roundf wb_roundf
#define fabsf wb_absf
#define clampf wb_clampf
#define minf wb_minf
#define maxf wb_maxf
#define sqrtf wb_sqrtf
#define rsqrtf wb_rsqrtf

//#define abs wb_abs
#endif

//castsi128 is just a typecast "reinterpret cast" style
#define ppf(x) _mm_set1_ps((f32)x)
#define ppi(x) _mm_set1_epi32((u32)x)
#define pfi(x) _mm_castsi128_ps(ppi(x))
#define WB_PI 3.141592653589793f
#define sse_select(a, b, cond) _mm_or_ps(_mm_and_ps(a, cond), _mm_andnot_ps(cond, b))
WBTM_API vf128 wb_abs_ps(vf128 x)
{
	return _mm_and_ps(x, pfi(~0x80000000));
}

WBTM_API vf128 wb_min_ps(vf128 x, vf128 y)
{
	return _mm_min_ps(x, y);
}

WBTM_API vf128 wb_max_ps(vf128 x, vf128 y)
{
	return _mm_max_ps(x, y);
}

WBTM_API vf128 wb_clamp_ps(vf128 minx, vf128 maxx, vf128 value)
{
	return _mm_min_ps(_mm_max_ps(value, minx), maxx);
}

WBTM_API vf128 wb_clamp11_ps(vf128 value)
{
	return _mm_min_ps(_mm_max_ps(value, _mm_set1_ps(-1.0f)), _mm_set1_ps(1.0f));
}

WBTM_API vf128 wb_clamp01_ps(vf128 value)
{
	return _mm_min_ps(_mm_max_ps(value, _mm_set1_ps(0.0f)), _mm_set1_ps(1.0f));
}

WBTM_API vf128 wb_rsqrt_ps(vf128 x)
{
	vf128 nr = _mm_rsqrt_ps(x);
	vf128 muls = _mm_mul_ps(_mm_mul_ps(x, nr), nr);
	return _mm_mul_ps(_mm_mul_ps(ppf(0.5), nr), _mm_sub_ps(ppf(3), muls));
}

WBTM_API vf128 wb_sqrt_ps(vf128 x)
{
	vf128 inv = wb_rsqrt_ps(x);
	return _mm_mul_ps(x, inv);
}

WBTM_API vf128 wb_atan_ps(vf128 xx)
{
	vf128 mask, mask2, y = ppf(0);
	vf128 one = ppf(1.0f);
	vf128 signs = _mm_and_ps(xx, pfi(0x80000000));
	vf128 x = _mm_and_ps(xx, pfi(~0x80000000));

	{
		vf128 tx1, tx2;
		mask = _mm_cmpgt_ps(x, ppf(2.414213562373095f));
		tx1 = _mm_div_ps(ppf(-1.0f), x);

		mask2 = _mm_cmpgt_ps(x, ppf(0.4142135623730950f));
		tx2 = _mm_div_ps(
				_mm_sub_ps(x, one), 
				_mm_add_ps(x, one));

		x = _mm_or_ps(_mm_and_ps(tx1, mask), _mm_andnot_ps(mask,
					_mm_or_ps(
						_mm_and_ps(tx2, mask2), 
						_mm_andnot_ps(mask2, x))));
		y = _mm_or_ps(_mm_and_ps(_mm_set1_ps(WB_PI/2), mask), _mm_andnot_ps(mask,
					_mm_or_ps(
						_mm_and_ps(_mm_set1_ps(WB_PI/4), mask2),
						_mm_andnot_ps(mask2, y))));
	}

	vf128 z = _mm_mul_ps(x, x);
	vf128 u = _mm_sub_ps(_mm_mul_ps(ppf(8.05374449538e-2), z),
			ppf(1.38776856032E-1));
	u = _mm_add_ps(_mm_mul_ps(u, z), ppf(1.99777106478E-1));
	u = _mm_sub_ps(_mm_mul_ps(u, z), ppf(3.33329491539E-1));
	u = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(u, z), x), x);
	y = _mm_add_ps(y, u);
	return _mm_mul_ps(y, _mm_or_ps(signs, one));
}

WBTM_API vf128 wb_atan2_ps(vf128 y, vf128 x)
{
	vf128 zero = _mm_setzero_ps(), xlt0, ylt0, xeq0, yeq0;
	vf128 z = wb_atan_ps(_mm_div_ps(y, x));

	xlt0 = _mm_cmplt_ps(x, zero);
	xeq0 = _mm_cmpeq_ps(x, zero);
	ylt0 = _mm_cmplt_ps(y, zero);
	yeq0 = _mm_cmpeq_ps(y, zero);

	z = sse_select(
			sse_select(
				sse_select(
					zero,
					_mm_set1_ps(WB_PI/2),
					yeq0),
				_mm_set1_ps(-WB_PI/2),
				ylt0),
			z,
			xeq0);

	z = sse_select(
			sse_select(
				_mm_set1_ps(WB_PI/2),
				zero,
				xlt0),
			z, 
			yeq0);

	vf128 w = sse_select(
			sse_select(
				_mm_set1_ps(-WB_PI),
				_mm_set1_ps(WB_PI),
				ylt0
				),
			zero,
			xlt0);
	return _mm_add_ps(z, w);

}

/* natural logarithm computed for 4 simultaneous f32 
   return NaN for x <= 0
   */
WBTM_API vf128 wb_log_ps(vf128 x) 
{
	vi128 emm0;
	vf128 one = ppf(1.0f);

	vf128 invalid_mask = _mm_cmple_ps(x, _mm_setzero_ps());

	x = _mm_max_ps(x, pfi(0x00800000));

	emm0 = _mm_srli_epi32(_mm_castps_si128(x), 23);
	/* keep only the fractional part */
	x = _mm_and_ps(x, pfi(~0x7f800000));
	x = _mm_or_ps(x, ppf(0.5f));

	emm0 = _mm_sub_epi32(emm0, ppi(0x7f));
	vf128 e = _mm_cvtepi32_ps(emm0);

	e = _mm_add_ps(e, one);

	vf128 mask = _mm_cmplt_ps(x, ppf(0.707106781186547524));
	vf128 tmp = _mm_and_ps(x, mask);
	x = _mm_sub_ps(x, one);
	e = _mm_sub_ps(e, _mm_and_ps(one, mask));
	x = _mm_add_ps(x, tmp);


	vf128 z = _mm_mul_ps(x,x);

	vf128 y = ppf(7.0376836292E-2);
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(-1.1514610310E-1));
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(1.1676998740E-1));
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(-1.2420140846E-1));
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(1.4249322787E-1));
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(-1.6668057665E-1));
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(2.0000714765E-1));
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(-2.4999993993E-1));
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(3.3333331174E-1));
	y = _mm_mul_ps(y, x);

	y = _mm_mul_ps(y, z);

	tmp = _mm_mul_ps(e, ppf(-2.12194440e-4));
	y = _mm_add_ps(y, tmp);

	tmp = _mm_mul_ps(z, ppf(0.5));
	y = _mm_sub_ps(y, tmp);

	tmp = _mm_mul_ps(e, ppf(0.693359375));
	x = _mm_add_ps(x, y);
	x = _mm_add_ps(x, tmp);
	x = _mm_or_ps(x, invalid_mask); // negative arg will be NAN
	return x;
}

WBTM_API vf128 wb_exp_ps(vf128 x) 
{
	vf128 tmp = _mm_setzero_ps(), fx;
	vi128 emm0;
	vf128 one = ppf(1);

	x = _mm_min_ps(x, ppf(88.3762626647949f));
	x = _mm_max_ps(x, ppf(-88.3762626647949f));

	/* express exp(x) as exp(g + n*log(2)) */
	fx = _mm_mul_ps(x, ppf(1.44269504088896341));
	fx = _mm_add_ps(fx, ppf(0.5));

	/* how to perform a floorf with SSE: just below */
	emm0 = _mm_cvttps_epi32(fx);
	tmp  = _mm_cvtepi32_ps(emm0);

	/* if greater, substract 1 */
	vf128 mask = _mm_cmpgt_ps(tmp, fx);    
	mask = _mm_and_ps(mask, one);
	fx = _mm_sub_ps(tmp, mask);

	tmp = _mm_mul_ps(fx, ppf(0.693359375));
	vf128 z = _mm_mul_ps(fx, ppf(-2.12194440e-4));
	x = _mm_sub_ps(x, tmp);
	x = _mm_sub_ps(x, z);

	z = _mm_mul_ps(x, x);

	vf128 y = ppf(1.9875691500E-4);
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(1.3981999507E-3));
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(8.3334519073E-3));
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(4.1665795894E-2));
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(1.6666665459E-1));
	y = _mm_mul_ps(y, x);
	y = _mm_add_ps(y, ppf(5.0000001201E-1));
	y = _mm_mul_ps(y, z);
	y = _mm_add_ps(y, x);
	y = _mm_add_ps(y, one);

	/* build 2^n */
	emm0 = _mm_cvttps_epi32(fx);
	emm0 = _mm_add_epi32(emm0, ppi(0x7f));
	emm0 = _mm_slli_epi32(emm0, 23);
	vf128 pow2n = _mm_castsi128_ps(emm0);
	y = _mm_mul_ps(y, pow2n);
	return y;
}

WBTM_API vf128 wb_sin_ps(vf128 x) 
{ 
	vf128 xmm1, xmm2 = _mm_setzero_ps(), xmm3, sign_bit, y;

	vi128 emm0, emm2;

	sign_bit = x;
	/* take the absolute value */
	x = _mm_and_ps(x, pfi(~0x80000000));
	/* extract the sign bit (upper one) */
	sign_bit = _mm_and_ps(sign_bit, pfi(0x80000000));

	/* scale by 4/Pi */
	y = _mm_mul_ps(x, ppf(1.27323954473516));

	/* store the i32eger part of y in mm0 */
	emm2 = _mm_cvttps_epi32(y);
	/* j=(j+1) & (~1) (see the cephes sources) */
	emm2 = _mm_add_epi32(emm2, ppi(1));
	emm2 = _mm_and_si128(emm2, ppi(~1));
	y = _mm_cvtepi32_ps(emm2);

	/* get the swap sign flag */
	emm0 = _mm_and_si128(emm2, ppi(4));
	emm0 = _mm_slli_epi32(emm0, 29);
	/* get the polynom selection mask 
	   there is one polynom for 0 <= x <= Pi/4
	   and another one for Pi/4<x<=Pi/2

	   Both branches will be computed.
	   */
	emm2 = _mm_and_si128(emm2, ppi(2));
	emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());

	vf128 swap_sign_bit = _mm_castsi128_ps(emm0);
	vf128 poly_mask = _mm_castsi128_ps(emm2);
	sign_bit = _mm_xor_ps(sign_bit, swap_sign_bit);

	/* The magic pass: "Extended precision modular arithmetic" 
	   x = ((x - y * DP1) - y * DP2) - y * DP3; */
	xmm1 = ppf(-0.78515625);
	xmm2 = ppf(-2.4187564849853515625e-4);
	xmm3 = ppf(-3.77489497744594108e-8);
	xmm1 = _mm_mul_ps(y, xmm1);
	xmm2 = _mm_mul_ps(y, xmm2);
	xmm3 = _mm_mul_ps(y, xmm3);
	x = _mm_add_ps(x, xmm1);
	x = _mm_add_ps(x, xmm2);
	x = _mm_add_ps(x, xmm3);

	/* Evaluate the first polynom  (0 <= x <= Pi/4) */
	y = ppf(2.443315711809948E-005);
	vf128 z = _mm_mul_ps(x,x);

	y = _mm_mul_ps(y, z);
	y = _mm_add_ps(y, ppf(-1.388731625493765E-003));
	y = _mm_mul_ps(y, z);
	y = _mm_add_ps(y, ppf(4.166664568298827E-002));
	y = _mm_mul_ps(y, z);
	y = _mm_mul_ps(y, z);
	vf128 tmp = _mm_mul_ps(z, ppf(0.5));
	y = _mm_sub_ps(y, tmp);
	y = _mm_add_ps(y, ppf(1));

	/* Evaluate the second polynom  (Pi/4 <= x <= 0) */

	vf128 y2 = ppf(-1.9515295891E-4);
	y2 = _mm_mul_ps(y2, z);
	y2 = _mm_add_ps(y2, ppf(8.3321608736E-3));
	y2 = _mm_mul_ps(y2, z);
	y2 = _mm_add_ps(y2, ppf(-1.6666654611E-1));
	y2 = _mm_mul_ps(y2, z);
	y2 = _mm_mul_ps(y2, x);
	y2 = _mm_add_ps(y2, x);

	/* select the correct result from the two polynoms */  
	xmm3 = poly_mask;
	y2 = _mm_and_ps(xmm3, y2); //, xmm3);
	y = _mm_andnot_ps(xmm3, y);
	y = _mm_add_ps(y,y2);
	/* update the sign */
	y = _mm_xor_ps(y, sign_bit);
	return y;
}

/* almost the same as sin_ps */
WBTM_API vf128 wb_cos_ps(vf128 x) 
{
	vf128 xmm1, xmm2 = _mm_setzero_ps(), xmm3, y;
	vi128 emm0, emm2;
	/* take the absolute value */
	x = _mm_and_ps(x, pfi(~0x80000000));

	/* scale by 4/Pi */
	y = _mm_mul_ps(x, ppf(1.27323954473516));

	/* store the i32eger part of y in mm0 */
	emm2 = _mm_cvttps_epi32(y);
	/* j=(j+1) & (~1) (see the cephes sources) */
	emm2 = _mm_add_epi32(emm2, ppi(1));
	emm2 = _mm_and_si128(emm2, ppi(~1));
	y = _mm_cvtepi32_ps(emm2);

	emm2 = _mm_sub_epi32(emm2, ppi(2));

	/* get the swap sign flag */
	emm0 = _mm_andnot_si128(emm2, ppi(4));
	emm0 = _mm_slli_epi32(emm0, 29);
	/* get the polynom selection mask */
	emm2 = _mm_and_si128(emm2, ppi(2));
	emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());

	vf128 sign_bit = _mm_castsi128_ps(emm0);
	vf128 poly_mask = _mm_castsi128_ps(emm2);

	/* The magic pass: "Extended precision modular arithmetic" 
	   x = ((x - y * DP1) - y * DP2) - y * DP3; */
	xmm1 = ppf(-0.78515625);
	xmm2 = ppf(-2.4187564849853515625e-4);
	xmm3 = ppf(-3.77489497744594108e-8);
	xmm1 = _mm_mul_ps(y, xmm1);
	xmm2 = _mm_mul_ps(y, xmm2);
	xmm3 = _mm_mul_ps(y, xmm3);
	x = _mm_add_ps(x, xmm1);
	x = _mm_add_ps(x, xmm2);
	x = _mm_add_ps(x, xmm3);

	/* Evaluate the first polynom  (0 <= x <= Pi/4) */
	y = ppf(2.443315711809948E-005);
	vf128 z = _mm_mul_ps(x,x);

	y = _mm_mul_ps(y, z);
	y = _mm_add_ps(y, ppf(-1.388731625493765E-003));
	y = _mm_mul_ps(y, z);
	y = _mm_add_ps(y, ppf(4.166664568298827E-002));
	y = _mm_mul_ps(y, z);
	y = _mm_mul_ps(y, z);
	vf128 tmp = _mm_mul_ps(z, ppf(0.5));
	y = _mm_sub_ps(y, tmp);
	y = _mm_add_ps(y, ppf(1));

	/* Evaluate the second polynom  (Pi/4 <= x <= 0) */

	vf128 y2 = ppf(-1.9515295891E-4);
	y2 = _mm_mul_ps(y2, z);
	y2 = _mm_add_ps(y2, ppf(8.3321608736E-3));
	y2 = _mm_mul_ps(y2, z);
	y2 = _mm_add_ps(y2, ppf(-1.6666654611E-1));
	y2 = _mm_mul_ps(y2, z);
	y2 = _mm_mul_ps(y2, x);
	y2 = _mm_add_ps(y2, x);

	/* select the correct result from the two polynoms */  
	xmm3 = poly_mask;
	y2 = _mm_and_ps(xmm3, y2); //, xmm3);
	y = _mm_andnot_ps(xmm3, y);
	y = _mm_add_ps(y,y2);
	/* update the sign */
	y = _mm_xor_ps(y, sign_bit);

	return y;
}

/* since sin_ps and cos_ps are almost identical, sincos_ps could replace both of them..
   it is almost as fast, and gives you a free cosine with your sine */
WBTM_API void wb_sincos_ps(vf128 x, vf128 *s, vf128 *c) 
{
	vf128 xmm1, xmm2, xmm3 = _mm_setzero_ps(), sign_bit_sin, y;
	vi128 emm0, emm2, emm4;
	sign_bit_sin = x;
	/* take the absolute value */
	x = _mm_and_ps(x, pfi(~0x80000000));
	/* extract the sign bit (upper one) */
	sign_bit_sin = _mm_and_ps(sign_bit_sin, pfi(0x80000000));

	/* scale by 4/Pi */
	y = _mm_mul_ps(x, ppf(1.27323954473516));

	/* store the i32eger part of y in emm2 */
	emm2 = _mm_cvttps_epi32(y);

	/* j=(j+1) & (~1) (see the cephes sources) */
	emm2 = _mm_add_epi32(emm2, ppi(1));
	emm2 = _mm_and_si128(emm2, ppi(~1));
	y = _mm_cvtepi32_ps(emm2);

	emm4 = emm2;

	/* get the swap sign flag for the sine */
	emm0 = _mm_and_si128(emm2, ppi(4));
	emm0 = _mm_slli_epi32(emm0, 29);
	vf128 swap_sign_bit_sin = _mm_castsi128_ps(emm0);

	/* get the polynom selection mask for the sine*/
	emm2 = _mm_and_si128(emm2, ppi(2));
	emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());
	vf128 poly_mask = _mm_castsi128_ps(emm2);

	/* The magic pass: "Extended precision modular arithmetic" 
	   x = ((x - y * DP1) - y * DP2) - y * DP3; */
	xmm1 = ppf(-0.78515625);
	xmm2 = ppf(-2.4187564849853515625e-4);
	xmm3 = ppf(-3.77489497744594108e-8);
	xmm1 = _mm_mul_ps(y, xmm1);
	xmm2 = _mm_mul_ps(y, xmm2);
	xmm3 = _mm_mul_ps(y, xmm3);
	x = _mm_add_ps(x, xmm1);
	x = _mm_add_ps(x, xmm2);
	x = _mm_add_ps(x, xmm3);

	emm4 = _mm_sub_epi32(emm4, ppi(2));
	emm4 = _mm_andnot_si128(emm4, ppi(4));
	emm4 = _mm_slli_epi32(emm4, 29);
	vf128 sign_bit_cos = _mm_castsi128_ps(emm4);

	sign_bit_sin = _mm_xor_ps(sign_bit_sin, swap_sign_bit_sin);


	/* Evaluate the first polynom  (0 <= x <= Pi/4) */
	vf128 z = _mm_mul_ps(x,x);
	y = ppf(2.443315711809948E-005);

	y = _mm_mul_ps(y, z);
	y = _mm_add_ps(y, ppf(1.388731625493765E-003));
	y = _mm_mul_ps(y, z);
	y = _mm_add_ps(y, ppf(4.166664568298827E-002));
	y = _mm_mul_ps(y, z);
	y = _mm_mul_ps(y, z);
	vf128 tmp = _mm_mul_ps(z, ppf(0.5));
	y = _mm_sub_ps(y, tmp);
	y = _mm_add_ps(y, ppf(1));

	/* Evaluate the second polynom  (Pi/4 <= x <= 0) */

	vf128 y2 = ppf(-1.9515295891E-4);
	y2 = _mm_mul_ps(y2, z);
	y2 = _mm_add_ps(y2, ppf(8.3321608736E-3));
	y2 = _mm_mul_ps(y2, z);
	y2 = _mm_add_ps(y2, ppf(-1.6666654611E-1));
	y2 = _mm_mul_ps(y2, z);
	y2 = _mm_mul_ps(y2, x);
	y2 = _mm_add_ps(y2, x);

	/* select the correct result from the two polynoms */  
	xmm3 = poly_mask;
	vf128 ysin2 = _mm_and_ps(xmm3, y2);
	vf128 ysin1 = _mm_andnot_ps(xmm3, y);
	y2 = _mm_sub_ps(y2,ysin2);
	y = _mm_sub_ps(y, ysin1);

	xmm1 = _mm_add_ps(ysin1,ysin2);
	xmm2 = _mm_add_ps(y,y2);

	/* update the sign */
	*s = _mm_xor_ps(xmm1, sign_bit_sin);
	*c = _mm_xor_ps(xmm2, sign_bit_cos);
}

WBTM_API vf128 wb_pow_ps(vf128 x, vf128 e)
{
	x = wb_log_ps(x);
	x = _mm_mul_ps(x, e);
	return wb_exp_ps(x);
}

WBTM_API vf128 wb_ldexp_ps(vf128 x, vf128 e)
{
	//vi128 b = _mm_slli_epi32(_mm_set_epi32(1, 1, 1, 1), e);
	e = wb_pow_ps(_mm_set1_ps(2), e);
	return _mm_mul_ps(x, e);
}

WBTM_API f32 wb_logf(f32 x)
{
	vf128 v = _mm_set_ss(x);
	return _mm_cvtss_f32(wb_log_ps(v));
}

WBTM_API f32 wb_expf(f32 x)
{
	vf128 v = _mm_set_ss(x);
	return _mm_cvtss_f32(wb_exp_ps(v));
}

WBTM_API f32 wb_sinf(f32 x)
{
	vf128 v = _mm_set_ss(x);
	return _mm_cvtss_f32(wb_sin_ps(v));
}

WBTM_API f32 wb_cosf(f32 x)
{
	vf128 v = _mm_set_ss(x);
	return _mm_cvtss_f32(wb_cos_ps(v));
}

WBTM_API void wb_sincosf(f32 x, f32* s, f32* c)
{
	vf128 v = _mm_set_ss(x);
	vf128 sin, cos;
	wb_sincos_ps(v, &sin, &cos);
	*s = _mm_cvtss_f32(sin);
	*c = _mm_cvtss_f32(cos);
}

WBTM_API f32 wb_tanf(f32 x)
{
	vf128 v = _mm_set_ss(x);
	vf128 sin, cos;
	wb_sincos_ps(v, &sin, &cos);
	return _mm_cvtss_f32(_mm_div_ps(sin, cos));
}

WBTM_API f32 wb_ldexpf(f32 x, i32 e)
{
	vf128 v = _mm_set_ps1(x);
	vf128 b = _mm_set_ps1((i32)e);
	return _mm_cvtss_f32(wb_ldexp_ps(v, b));
}

WBTM_API f32 wb_rsqrtf(f32 x)
{
	vf128 v = _mm_set_ss(x);
	v = wb_rsqrt_ps(v);
	return _mm_cvtss_f32(v);
}

WBTM_API f32 wb_sqrtf(f32 x)
{
	vf128 v = _mm_set_ss(x);
	vf128 s = wb_sqrt_ps(v);
	return _mm_cvtss_f32(s);
}

WBTM_API f32 wb_ipowf(f32 x, i32 e)
{
	f32 r = 1;
	while(e) {
		if(e & 1) {
			r *= x;
			e--;
		}
		x *= x;
		e /= 2;
	}
	return r;
}

WBTM_API f32 wb_powf(f32 x, f32 y)
{
	vf128 a = _mm_set_ss(x);
	vf128 b = _mm_set_ss(y);
	return _mm_cvtss_f32(wb_pow_ps(a, b));
}

WBTM_API i32 wb_floorf(f32 x)
{
	return (int)x + (x < 0 ? -1 : 0);
}

WBTM_API i32 wb_ceilf(f32 x)
{
	return (int)x + (x < 0 ? 0 : 1);
}

WBTM_API f32 wb_minf(f32 x, f32 y)
{
	vf128 a = _mm_set_ss(x);
	vf128 b = _mm_set_ss(y);
	return _mm_cvtss_f32(wb_min_ps(a, b));
}

WBTM_API f32 wb_maxf(f32 x, f32 y)
{
	vf128 a = _mm_set_ss(x);
	vf128 b = _mm_set_ss(y);
	return _mm_cvtss_f32(wb_max_ps(a, b));
}

WBTM_API f32 wb_clampf(f32 low, f32 high, f32 value)
{
	vf128 minx = _mm_set_ss(low);
	vf128 maxx = _mm_set_ss(high);
	vf128 val = _mm_set_ss(value);
	return _mm_cvtss_f32(wb_clamp_ps(minx, maxx, val));
}


WBTM_API f32 wb_clamp11f(f32 value)
{
	vf128 val = _mm_set_ss(value);
	return _mm_cvtss_f32(wb_clamp11_ps(val));
}

WBTM_API f32 wb_clamp01f(f32 value)
{
	vf128 val = _mm_set_ss(value);
	return _mm_cvtss_f32(wb_clamp01_ps(val));
}

WBTM_API i32 wb_abs(i32 v)
{
	// This isn't a good abs implementation, but it's functional
	return v < 0 ? -1 * v : v;
}

WBTM_API f32 wb_absf(f32 x)
{
	vf128 v = _mm_set_ss(x);
	return _mm_cvtss_f32(wb_abs_ps(v));
}

WBTM_API f32 wb_atanf(f32 x)
{
	vf128 v = _mm_set_ss(x);
	vf128 w = wb_atan_ps(v);
	f32 q = _mm_cvtss_f32(w);
	return q;
}

WBTM_API i32 wb_roundf(f32 x)
{
	f32 fx = wb_floorf(x);
	f32 frac = x - fx;
	if(frac < 0.5) 
		return (i32)fx;
	else
		return (i32)fx + 1;
}

WBTM_API f32 wb_atan2f(f32 y, f32 x)
{
	int code = 0;
	if(x < 0) {
		code = 2;
	}
	if(y < 0) {
		code |= 1;
	}

	if(x == 0) {
		if(code & 1) {
			return -WB_PI / 2;
		}
		if(y == 0) {
			return 0;
		}
		return WB_PI / 2;
	}

	if(y == 0) {
		if(code & 2) {
			return WB_PI;
		}
		return 0;
	}

	f32 w = 0;
	if(x < 0) {
		if(y < 0) {
			w = -WB_PI;
		} else {
			w = WB_PI;
		}
	}

	return w + wb_atanf(y/x);
}
