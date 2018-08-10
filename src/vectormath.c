#ifndef WirmphtEnabled
union Vec2 
{
	struct {
		f32 x, y;
	};
	f32 e[2];
};

union Vec2i
{
	struct {
		i32 x, y;
	};
	i32 e[2];
};

struct AABB
{
	Vec2 min, max;
};

union Rect2f
{
	struct {Vec2 pos, size;};
	struct {f32 x, y, w, h;};
	f32 e[4];
};

struct Rect2i
{
	i16 x, y, w, h;
};

union Rect2l
{
	struct {i32 x, y, w, h;};
	struct {Vec2i pos, size;};
};
#endif

static inline
Vec2 v2(f32 x, f32 y)
{
	Vec2 v = {x, y};
	return v;
}

static inline
Vec2 v2s(f32 x, f32 y, f32 s)
{
	Vec2 v = {x * s, y * s};
	return v;
}

static inline
Vec2i v2i(i32 x, i32 y)
{
	Vec2i v = {x, y};
	return v;
}
static inline
Vec2i v2is(i32 x, i32 y, i32 s)
{
	Vec2i v = {x * s, y * s};
	return v;
}

static inline
Vec2i v2iSub(Vec2i a, Vec2i b)
{
	Vec2i v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	return v;
}

static inline
Vec2i v2iAdd(Vec2i a, Vec2i b)
{
	Vec2i v;
	v.x = a.x + b.x;
	v.y = a.y + b.y;
	return v;
}


static inline
Vec2i v2iAddP(Vec2i* a, Vec2i b)
{
	Vec2i v;
	v.x = a->x + b.x;
	v.y = a->y + b.y;
	*a = v;
	return v;
}



// TODO(will) determine whether we want floor or round here?
static inline
Vec2i v2iq(f32 x, f32 y, i32 s)
{
	i32 xx = roundf(x / (f32)s);	
	i32 yy = roundf(y / (f32)s);	
	Vec2i v = {xx * s, yy * s};
	return v;
}

static inline
Vec2i v2ii(f32 x, f32 y, i32 s)
{
	i32 xx = roundf(x / (f32)s);	
	i32 yy = roundf(y / (f32)s);	
	Vec2i v = {xx, yy};
	return v;
}


static inline
Vec2 v2f(Vec2i a)
{
	return v2(a.x, a.y);
}

static inline
Vec2 v2fs(Vec2i a, f32 scale)
{
	return v2(a.x * scale, a.y * scale);
}


static inline 
AABB aabb(Vec2 a, Vec2 b)
{
	AABB bb = {a, b};
	return bb;
}

static inline
AABB aabb4f(f32 minx, f32 miny, f32 maxx, f32 maxy)
{
	AABB bb = {
		{minx, miny},
		{maxx, maxy}
	};
	return bb;
}

static inline
Vec2 v2Add(Vec2 a, Vec2 b)
{
	Vec2 v;
	v.x = a.x + b.x;
	v.y = a.y + b.y;
	return v;
}

static inline
Vec2 v2AddXY(Vec2 a, f32 x, f32 y)
{
	Vec2 v;
	v.x = a.x + x;
	v.y = a.y + y;
	return v;
}

static inline
Vec2 v2AddScaled(Vec2 a, Vec2 b, f32 f)
{
	Vec2 v;
	v.x = a.x + b.x * f;
	v.y = a.y + b.y * f;
	return v;
}

static inline
Vec2 v2Negate(Vec2 a)
{
	Vec2 v;
	v.x = -a.x;
	v.y = -a.y;
	return v;
}

static inline
Vec2 v2Sub(Vec2 a, Vec2 b)
{
	Vec2 v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	return v;
}

static inline
Vec2i v2iAbs(Vec2i a)
{
	Vec2i v;
	v.x = abs(a.x);
	v.y = abs(a.y);
	return v;
}

static inline
Vec2 v2Scale(Vec2 a, f32 f)
{
	Vec2 v;
	v.x = a.x * f;
	v.y = a.y * f;
	return v;
}

static inline
Vec2 v2Mul(Vec2 a, Vec2 b)
{
	Vec2 v;
	v.x = a.x * b.x;
	v.y = a.y * b.y;
	return v;
}

static inline
f32 v2Mag2(Vec2 a)
{
	return a.x * a.x + a.y * a.y;
}

static inline
f32 v2Mag(Vec2 a)
{
	return sqrtf(v2Mag2(a));
}

static inline
f32 v2Dot(Vec2 a, Vec2 b)
{
	return a.x * b.x + a.y * b.y;
}

static inline
f32 v2Cross(Vec2 a, Vec2 b)
{
	f64 p1 = (f64)a.x * (f64)b.y;
	f64 p2 = (f64)b.x * (f64)a.y;
	return (f32)(p1 - p2);
	//return (a.x * b.y) - (a.y * b.x);
}

static inline
f32 v2CrossOrigin(Vec2 a, Vec2 b, Vec2 o)
{
	a = v2Sub(a, o);
	b = v2Sub(b, o);
	return a.x * b.y - a.y * b.x;
}

static inline
Vec2 v2Normalize(Vec2 a)
{
	Vec2 v;
	f32 mag = v2Mag(a);
	v.x = a.x / mag;
	v.y = a.y / mag;
	return v;
}

static inline
Vec2 v2Orient(Vec2 where, Vec2 to)
{
	return v2Normalize(v2Sub(to, where));
}

static inline
Vec2 v2FromAngle(f32 angle, f32 mag)
{
	Vec2 v;
	v.x = cosf(angle) * mag;
	v.y = sinf(angle) * mag;
	return v;
}

static inline
f32 v2ToAngle(Vec2 a)
{
	return atan2f(a.y, a.x);
}

static inline
f32 v2DistToAngle(Vec2 a, Vec2 b)
{
	Vec2 v = v2Sub(b, a);
	return atan2f(v.y, v.x);
}

static inline
f32 v2Dist2(Vec2 a, Vec2 b)
{
	Vec2 v = v2Sub(b, a);
	return v2Dot(v, v);
}

f32 v2Dist(Vec2 a, Vec2 b)
{
	Vec2 v = v2Sub(b, a);
	return sqrtf(v2Dot(v, v));
}

static inline 
Vec2 v2Perpendicular(Vec2 a)
{
	Vec2 v;
	v.x = -a.y;
	v.y = a.x;
	return v;
}

static inline
Vec2 v2Abs(Vec2 a)
{
	Vec2 v;
	v.x = fabsf(a.x);
	v.y = fabsf(a.y);
	return v;
}

static inline
Vec2 v2Min(Vec2 a, Vec2 b)
{
	Vec2 v;
	v.x = minf(a.x, b.x);
	v.y = minf(a.y, b.y);
	return v;
}

static inline
Vec2 v2Max(Vec2 a, Vec2 b)
{
	Vec2 v;
	v.x = maxf(a.x, b.x);
	v.y = maxf(a.y, b.y);
	return v;
}



static inline
Vec2 v2SwapXY(Vec2 a)
{
	Vec2 v;
	v.x = a.y;
	v.y = a.x;
	return v;
}

static inline
Vec2 v2Rotate(Vec2 a, Vec2 rot)
{
	Vec2 v;
	v.x = rot.x * a.x + rot.y * a.y;
	v.y = -rot.y * a.x + rot.x * a.y;
	return v;
}

static inline
Vec2 v2RotateOrigin(Vec2 a, Vec2 rot, Vec2 orig)
{
	Vec2 v;
	a.x -= orig.x;
	a.y -= orig.y;
	v.x = rot.x * a.x + rot.y * a.y;
	v.y = -rot.y * a.x + rot.x * a.y;
	return v2Add(v, orig);
}


static inline
Vec2 v2Floor(Vec2 a)
{
	Vec2 v;
	v.x = floorf(a.x);
	v.y = floorf(a.y);
	return v;
}

static inline
i32 v2iEq(Vec2i a, Vec2i b)
{
	return a.x == b.x && a.y == b.y;
}
static inline
i32 v2Eq(Vec2 a, Vec2 b)
{
	return a.x == b.x && a.y == b.y;
}

static inline
i32 v2AlmostEq(Vec2 a, Vec2 b, f32 amt)
{
	vf32x4 absab = {wb_abs_ps(_mm_setr_ps(a.x, a.y, b.x, b.y))};
	f32 dx = absab.f[2] - absab.f[0];
	if(dx * dx < amt) {
		f32 dy = absab.f[3] - absab.f[1];
		if(dy * dy < amt) {
			return 1;
		}
	} 
	return 0;
}

static inline
void v2AddP(Vec2* a, Vec2 b)
{
	a->x += b.x;
	a->y += b.y;
}

static inline
void v2AddScaledP(Vec2* a, Vec2 b, f32 f)
{
	a->x += b.x * f;
	a->y += b.y * f;
}

static inline
void v2NegateP(Vec2* a)
{
	a->x *= -1;
	a->y *= -1;
}

static inline
void v2SubP(Vec2* a, Vec2 b)
{
	a->x -= b.x;
	a->y -= b.y;
}

static inline
void v2ScaleP(Vec2* a, f32 f)
{
	a->x *= f;
	a->y *= f;
}

static inline
void v2MulP(Vec2* a, Vec2 b)
{
	a->x *= b.x;
	a->y *= b.y;
}

static inline
void v2NormalizeP(Vec2* a)
{
	f32 mag = v2Mag(*a);
	a->x /= mag;
	a->y /= mag;
}

static inline
void v2PerpendicularP(Vec2* a)
{
	f32 x = a->x;
	a->x = -a->y;
	a->y = x;
}

static inline
void v2RotateP(Vec2* b, Vec2 rot)
{
	Vec2 a = *b;
	b->x = rot.x * a.x + rot.y * a.y;
	b->y = -rot.y * a.x + rot.x * a.y;
}

static inline
void v2FloorP(Vec2* a)
{
	a->x = floorf(a->x);
	a->y = floorf(a->y);
}

static inline
Vec2 v2WorldToLocal(Vec2 pt, Vec2 offset, f32 scale, Vec2 hWindow)
{
	v2SubP(&pt, offset);
	v2ScaleP(&pt, scale);
	v2SubP(&pt, hWindow);
	return pt;
}
static inline

Vec2 v2LocalToWorld(Vec2 pt, Vec2 offset, f32 scale, Vec2 hWindow)
{
	v2SubP(&offset, hWindow);
	v2AddP(&pt, offset);
	v2ScaleP(&pt, 1.0f / scale);
	return pt;
}

static inline
i32 r2fTest(Rect2f a, Rect2f b)
{
	if((a.x + a.w) < b.x || a.x > (b.x + b.w)) return 0;
	if((a.y + a.h) < b.y || a.y > (b.y + b.h)) return 0;
	return 1;
}

static inline
i32 r2fContains(Rect2f b, Vec2 p)
{
	if(p.x < b.x || p.x > (b.x + b.w)) return 0;
	if(p.y < b.y || p.y > (b.y + b.h)) return 0;
	return 1;
}

static inline
i32 aabbContains(AABB b, Vec2 p)
{
	if(p.x < b.min.x || p.x > b.max.x) return 0;
	if(p.y < b.min.y || p.y > b.max.y) return 0;
	return 1;
}
static inline
i32 aabbTest(AABB a, AABB b)
{
	if(a.max.x < b.min.x || a.min.x > b.max.x) return 0;
	if(a.max.y < b.min.y || a.min.y > b.max.y) return 0;
	return 1;
}

static inline 
AABB aabbUnion(AABB a, AABB b)
{
	// TODO(will): this will prefer B if A is tiny,
	// 		but I can't think of a better way to do it
	// 		so here we are
	if(v2Mag2(v2Sub(a.max, a.min)) == 0) {
		return b;
	} else if(v2Mag2(v2Sub(b.max, b.min)) == 0) {
		return a;
	}

	AABB c;
	c.min = v2Min(a.min, b.min);
	c.max = v2Max(a.max, b.max);
	return c;
}

static inline
AABB aabbClip(AABB a, AABB b)
{
	AABB c;
	c.min = v2Max(a.min, b.min);
	c.max = v2Min(a.max, b.max);
	if(c.min.x > c.max.x) {
		c.max.x = c.min.x;
	}
	if(c.min.y > c.max.y) {
		c.max.y = c.min.y;
	}
	return c;
}


Vec2 lineClosestPoint(Vec2 a, Vec2 b, Vec2 p)
{
	Vec2 ab = v2Sub(b, a);
	f32 t = v2Dot(v2Sub(p, a), ab);
	if(t <= 0) {
		return a;
	} else {
		f32 denom = v2Dot(ab, ab);
		if(t >= denom) {
			return b;
		} else {
			return v2AddScaled(a, ab, t / denom);
		}

	}
}

Vec2 triClosestPoint(Vec2 a, Vec2 b, Vec2 c, Vec2 p)
{
	// NOTE(will): this is included for completeness; we don't need to use it
	// because all our triangles are axis-aligned.
	Vec2 ab = v2Sub(b, a);
	Vec2 ac = v2Sub(c, a);
	Vec2 ap = v2Sub(p, a);
	f32 d1 = v2Dot(ab, ap);
	f32 d2 = v2Dot(ac, ap);
	if(d1 <= 0 && d2 <= 0) return a;

	Vec2 bp = v2Sub(p, b);
	f32 d3 = v2Dot(ab, bp);
	f32 d4 = v2Dot(ac, bp);
	if(d3 >= 0 && d4 <= d3) return b;

	f32 vc = d1 * d4 - d3 * d2;
	if(vc <= 0 && d1 >= 0 && d3 < 0) {
		return v2AddScaled(a, ab, (d1 / (d1 - d3)));
	}

	Vec2 cp = v2Sub(p, c);
	f32 d5 = v2Dot(ab, cp);
	f32 d6 = v2Dot(ac, cp);
	if(d6 >= 0 && d5 <= d6) return c;

	f32 vb = d5 * d2 - d1 * d6;
	if(vb <= 0 && d2 > 0 && d6 < 0) {
		return v2AddScaled(a, ac, (d2 / (d2 - d6)));
	}

	f32 va = d3 * d6 - d5 * d4;
	if(va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
		f32 w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		return v2AddScaled(b, v2Sub(c, b), w);
	}

	f32 denom = 1.0 / (va + vb + vc);
	f32 v = vb * denom;
	f32 w = vc * denom;
	return v2Add(a, v2AddScaled(v2Scale(ab, v), ac, w));
}

static inline
Rect2f r2fv(Vec2 pos, Vec2 size)
{
	Rect2f r;
	r.pos = pos;
	r.size = size;
	return r;
}

static inline
Rect2f r2f(f32 x, f32 y, f32 w, f32 h)
{
	Rect2f r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	return r;
}
static inline
Rect2i r2i(i16 x, i16 y, i16 w, i16 h)
{
	Rect2i r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	return r;
}

static inline
Rect2l r2l(i32 x, i32 y, i32 w, i32 h)
{
	Rect2l r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	return r;
}

static inline
Rect2l r2lv(Vec2i pos, Vec2i size)
{
	Rect2l r;
	r.x = pos.x;
	r.y = pos.y;
	r.w = size.x;
	r.h = size.y;
	return r;
}
static inline
Rect2l r2lvSafe(Vec2i pos, Vec2i size)
{
	Vec2i a = pos;
	Vec2i b = v2iAdd(pos, size);
	Vec2i min = v2i(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y);
	Vec2i max = v2i(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y);
	Rect2l r = r2lv(min, v2iSub(max, min));
	return r;
}
static inline
i32 r2lContains(Rect2l big, Rect2l small)
{
	return small.x >= big.x && small.y >= big.y &&
	small.x + small.w < (big.x + big.w) &&
	small.y + small.h < (big.y + big.h);
}

static inline
i32 r2lIsOutside(Rect2l big, Rect2l small)
{
	i32 isContained = small.x > big.x && small.y > big.y &&
	small.x + small.w < (big.x + big.w) &&
	small.y + small.h < (big.y + big.h);
	if(isContained) return 0;
	
	i32 side = 0;
	if(small.x < big.x) {
		side |= 4;	
	}

	if(small.y < big.y) {
		side |= 1;	
	}

	if(small.x + small.w >= (big.x + big.w)) {
		side |= 8;	
	}

	if(small.y + small.h >= (big.y + big.h)) {
		side |= 2;	
	}
	return side;
}

static inline 
AABB aabbSafe(Vec2 a, Vec2 b)
{
	Vec2 a1 = v2Min(a, b);
	Vec2 b1 = v2Max(a, b);
	AABB bb = {a1, b1};
	return bb;
}



