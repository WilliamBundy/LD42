
static inline 
u64 splitmix64(u64* x)
{
	*x += UINT64_C(0x9E3779B97F4A7C15);
	u64 z = *x;
	z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
	return z ^ (z >> 31);	
}

static inline 
u64 rotateLeft(const u64 t, i64 k)
{
	return (t << k) | (t >> (64 - k));
}

#ifndef WirmphtEnabled
struct RandomState
{
	u64 x, y;
};
#endif

u64 u64rand(RandomState* r)
{
	u64 a = r->x;
	u64 b = r->y;
	u64 result = a + b;

	b ^= a;
	r->x = rotateLeft(a, 55) ^ b ^ (b << 14);
	r->y = rotateLeft(b, 36);
	return result;
}

void initRandXY(RandomState* r, u64 baseSeed, u64 seedX, u64 seedY) 
{
	splitmix64(&baseSeed);
	seedX *= baseSeed;
	seedY *= baseSeed;
	r->x = splitmix64(&seedX);
	r->y = splitmix64(&seedY);
	u64rand(r);
	u64rand(r);
	u64rand(r);
	u64rand(r);
	u64rand(r);
	u64rand(r);
	u64rand(r);
	u64rand(r);
}

void initRand(RandomState* r, u64 seed) 
{
	splitmix64(&seed);
	r->x = splitmix64(&seed);
	r->y = splitmix64(&seed);
	u64rand(r);
	u64rand(r);
	u64rand(r);
	u64rand(r);
	u64rand(r);
	u64rand(r);
	u64rand(r);
	u64rand(r);
}

f64 f64rand(RandomState* r)
{
	return (double)u64rand(r) / (double)UINT64_MAX;
}

f32 f32rand(RandomState* r)
{
	return (f32)f64rand(r);
}

//TODO(will) potentially implement with different rounding modes
isize randrange(RandomState* r, isize mn, isize mx)
{
	return (isize)(f64rand(r) * (f64)(mx - mn) + (f64)mn);
}

f64 f64randrange(RandomState* r, f64 mn, f64 mx)
{
	return (f64rand(r) * (mx - mn) + mn);
}

f32 f32randrange(RandomState* r, f64 mn, f64 mx)
{
	return (f32)(f64rand(r) * (mx - mn) + mn);
}

Vec2 vec2rand(RandomState* r, f32 s, Vec2 base)
{
	base.x += s * f32rand(r);
	base.y += s * f32rand(r);
	return base;
}

