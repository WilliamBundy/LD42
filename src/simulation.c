/*
 * The big physics TODO(will)
 *		--	Body addition and removal
 *		--	Different body arrays for different classes:
 *			Dynamic/Active
 *			Sleeping
 *			Static
 *		--  Once we get world loading creation in, we need to load
 *			friction values for terrain in.
 *		--  Too, once we have a big world, we need to add a spacial
 *			hash/grid for perf/localized sim
 *		--	That requires the cell system from PA to be copied: we should
 *			probably use iterators internally too, even if they're slower
 *			just for understanding the terrible beast :)
 *		--	Raycasts! Get your raycasts here! (we'll need those, probs)
 *		--	Consider splitting up collision representation of bodies:
 *			Having the bounding box and shapes available for broadphase,
 *			and only resolving when we get to narrowphase might give us
 *			better cache perf and speed for high body counts
 *
 */

enum {
	Body_Normal,
	Body_Sensor = 1<<1,
};

enum {
	Shape_AABB,
	Shape_Circle,
	Shape_TriangleTL,
	Shape_TriangleTR,
	Shape_TriangleBR,
	Shape_TriangleBL,
};


#define bodyMinSide(body, axis) ((body)->boundingBox.min.e[axis])
#define bodyMaxSide(body, axis) ((body)->boundingBox.max.e[axis])
#ifndef WirmphtEnabled
struct SimContactPair
{
	SimBody* a;
	SimBody* b;
	i32 order;
};

struct SimContact
{
	Vec2 point;
	Vec2 normal;
	f32 overlap;
};

struct SimBody
{
	Vec2 pos, vel, acl;
	Vec2 size, correction;
	AABB boundingBox;

	f32 staticFriction, dynamicFriction;
	f32 groundFriction;
	f32 restitution;

	f32 invMass;
	u32 flags;
	i32 shape;
	i32 index;
};

struct SimWorld
{
	wMemoryArena* arena;
	wMemoryPool* bodyPool;
	SimBody** bodies;
	SimBody* bodyStorage;
	isize bodyCount, bodyCapacity;

	SimContactPair* contacts;
	isize contactCount;

	i32 iterations;
	f32 damping;
	i32 sortAxis;
};
#endif

void initBody(SimBody* body)
{
	SimBody null = {0};
	*body = null;
	body->invMass = 1;
	body->size = v2(8, 8);
	body->correction = v2(0, 0);

	body->staticFriction = 0.5;
	body->dynamicFriction = 0.3;
	body->restitution = 0.25;
	body->groundFriction = 1;
}



#define MassConstant (1024.0f)
#define GravityConstant  36
#define MaxVelocity 10000.0
#define MaxVelocitySq (MaxVelocity * MaxVelocity)
#define Sim_DoGroundFriction 0

SimBody* simAddBox(SimWorld* sim, Vec2 pos, Vec2 size)
{
	if(sim->bodyCount >= sim->bodyCapacity) return NULL;
	SimBody* a = wPoolRetrieve(sim->bodyPool);
	initBody(a);
	a->pos = pos;
	a->size = size;
	
	f32 mass = size.x * size.y;
	a->invMass = MassConstant / mass;

	sim->bodies[sim->bodyCount] = a;
	a->index = sim->bodyCount;
	sim->bodyCount++;
	
	return a;
}

SimBody* simAddCircle(SimWorld* sim, Vec2 pos, f32 diameter)
{
	if(sim->bodyCount >= sim->bodyCapacity) return NULL;
	SimBody* a = wPoolRetrieve(sim->bodyPool);
	initBody(a);
	a->pos = pos;
	a->shape = Shape_Circle;
	a->size = v2(diameter, diameter);

	// just.... don't ask
	f32 mass = 0.125 * diameter * diameter * Math_Tau;
	a->invMass = MassConstant / mass;

	sim->bodies[sim->bodyCount] = a;
	a->index = sim->bodyCount;
	sim->bodyCount++;
	
	return a;
}

SimBody* simAddBody(SimWorld* sim, Vec2 pos, Vec2 size, i32 shape)
{
	if(sim->bodyCount >= sim->bodyCapacity) return NULL;
	SimBody* a = wPoolRetrieve(sim->bodyPool);
	initBody(a);
	a->pos = pos;
	a->size = size;
	a->shape = shape;

	f32 mass = 0;
	switch(shape) {
		case Shape_AABB:
			mass = size.x * size.y;
			break;
		case Shape_Circle:
			mass = 0.125 * size.x * size.x * Math_Tau;
			break;
		case Shape_TriangleTL:
		case Shape_TriangleBL:
		case Shape_TriangleTR:
		case Shape_TriangleBR:
			mass = size.x * size.y * 0.5;
			break;
		default:
			mass = 1;
			break;
	}

	a->invMass = MassConstant / mass;

	sim->bodies[sim->bodyCount] = a;
	a->index = sim->bodyCount;
	sim->bodyCount++;
	
	return a;

}



// Because the sim world has its own allocator,
// we don't provide a separate init function
SimWorld* createSimWorld(isize bodyCapacity)
{
	wMemoryArena* arena = wArenaBootstrap(game.memInfo, wArena_Normal);
	SimWorld* sim = wArenaPush(arena, sizeof(SimWorld));
	sim->arena = arena;

	sim->bodyPool = wPoolBootstrap(game.memInfo, sizeof(SimBody), wPool_Normal);

	sim->bodyStorage = sim->bodyPool->slots;
	sim->bodyCount = 0;
	sim->bodyCapacity = bodyCapacity;
	sim->bodies = wArenaPush(arena, sizeof(SimBody*) * bodyCapacity);

	sim->iterations = 16;
	sim->damping = 0.95f;
	sim->sortAxis = 0;
	return sim;
}

static inline
void updateBodyBounds(SimBody* body)
{
	Vec2 halfSize = v2Scale(body->size, 0.5);
	body->boundingBox = aabb(
			v2Sub(body->pos, halfSize),
			v2Add(body->pos, halfSize));
}

void sortBodies(SimBody** array, isize count, i32 axis)
{
	for(isize i = 1; i < count; ++i) {
		isize j = i - 1;

		f32 minSide = array[i]->boundingBox.min.e[axis];
		if(array[j]->boundingBox.min.e[axis] > minSide) {
			SimBody* temp = array[i];
			while((j >= 0) && (array[j]->boundingBox.min.e[axis] > minSide)) {
				array[j + 1] = array[j];
				j--;
			}
			array[j + 1] = temp;
		}
	}

	for(isize i = 0; i < count; ++i) {
		array[i]->index = i;
	}
}

void simAddContactPair(SimWorld* sim, SimBody* a, SimBody* b)
{
	wArenaPush(sim->arena, sizeof(SimContactPair));
	SimContactPair* c = sim->contacts + sim->contactCount++;
	c->a = a;
	c->b = b;
	c->order = a->shape + b->shape * 1000;
}

void collideBoxBox(SimWorld* sim, SimBody* a, SimBody* b, SimContact* c)
{
	f32 sx = (a->size.x + b->size.x) * 0.5 - fabsf(b->pos.x - a->pos.x);
	f32 sy = (a->size.y + b->size.y) * 0.5 - fabsf(b->pos.y - a->pos.y);
	f32 overlap = 0;
	Vec2 normal = v2(0, 0);
	if(sx > sy) {
		sx = 0;
		if(a->pos.y > b->pos.y) {
			sy *= -1;
		}
		overlap = fabsf(sy);
		normal.y = sy / overlap;
	} else {
		sy = 0;
		if(a->pos.x > b->pos.x) {
			sx *= -1;
		}
		overlap = fabsf(sx);
		normal.x = sx / overlap;
	}
	c->overlap = overlap;
	c->normal = normal;
}

void collideCircleBox(SimWorld* sim, SimBody* a, SimBody* b, SimContact* c)
{
	// b is the box
	AABB box = b->boundingBox;

	// closest point
	f32 x = clampf(box.min.x, box.max.x, a->pos.x);
	f32 y = clampf(box.min.y, box.max.y, a->pos.y);
	Vec2 closest = v2(x, y);

	Vec2 dist = v2Sub(closest, a->pos);
	f32 radius = a->size.x * 0.5;
	f32 rad2 = radius * radius;
	f32 dist2 = v2Mag2(dist);
	if(dist2 <= rad2) {
		f32 mag = dist2 == 0 ? 0.01f : sqrtf(dist2);
		dist.x /= mag;
		dist.y /= mag;
		c->normal = v2Sub(
				v2AddScaled(a->pos, dist, radius),
				closest);
		c->overlap = v2Mag(c->normal);
		v2ScaleP(&c->normal, 1.0 / c->overlap);
	}
}

void collideCircleCircle(SimWorld* sim, SimBody* a, SimBody* b, SimContact* c)
{
	f32 radiusA = a->size.x * 0.5;
	f32 radiusB = b->size.x * 0.5;
	f32 radiusSum = radiusA + radiusB;
	radiusSum *= radiusSum;
	Vec2 diff = v2Sub(b->pos, a->pos);
	f32 dist = v2Mag2(diff);
	if(dist < radiusSum) {
		f32 mag = dist == 0 ? 0.01f : sqrtf(dist);
		c->overlap = radiusA + radiusB - mag;
		c->normal = v2Scale(diff, 1.0 / mag);
	}
}

static inline
Vec2 getTriangleHypot(i32 corner, Vec2 size)
{
	Vec2 normal = v2(0, 0);
	switch(corner) {
		case Shape_TriangleTL:
			normal = v2(size.y, size.x);
			break;
		case Shape_TriangleTR:
			normal = v2(-size.y, size.x);
			break;
		case Shape_TriangleBR:
			normal = v2(-size.y, -size.x);
			break;
		case Shape_TriangleBL:
			normal = v2(size.y, -size.x);
			break;
	}
	return v2Normalize(normal);
}

// NOTE(will): 0->1 is always hypotenuse
static inline 
void triGetCorners(SimBody* a, Vec2* corners)
{
	Vec2 hs = v2Scale(a->size, 0.5);
	// clockwise winding
	switch(a->shape) {
		case Shape_TriangleTL:
			corners[0] = v2AddXY(a->pos, hs.x, -hs.y);
			corners[1] = v2AddXY(a->pos, -hs.x, hs.y);
			corners[2] = v2Sub(a->pos, hs);
			break;
		case Shape_TriangleTR:
			corners[0] = v2AddXY(a->pos, hs.x, hs.y);
			corners[1] = v2Sub(a->pos, hs);
			corners[2] = v2AddXY(a->pos, hs.x, -hs.y);
			break;
		case Shape_TriangleBR:
			corners[0] = v2AddXY(a->pos, -hs.x, hs.y);
			corners[1] = v2AddXY(a->pos, hs.x, -hs.y);
			corners[2] = v2AddXY(a->pos, hs.x, hs.y);
			break;
		case Shape_TriangleBL:
			corners[0] = v2AddXY(a->pos, -hs.x, -hs.y);
			corners[1] = v2AddXY(a->pos, hs.x, hs.y);
			corners[2] = v2AddXY(a->pos, -hs.x, hs.y);
			break;
	}
}

void collideBoxTriangle(SimWorld* sim, SimBody* a, SimBody* b, SimContact* c)
{
	Vec2 hypotN = getTriangleHypot(b->shape, b->size);
	Vec2 diff = v2Sub(b->pos, a->pos);
	if(v2Dot(diff, hypotN) > 0) {
		collideBoxBox(sim, a, b, c);
		return;
	}

	Vec2 corners[3];
	triGetCorners(b, corners);
	Vec2 closest = lineClosestPoint(corners[0], corners[1], a->pos);
	Vec2 axis = hypotN;
	c->normal = v2Negate(axis);
	
	f32 triProj[3];
	triProj[0] = v2Dot(axis, corners[0]);
	triProj[1] = v2Dot(axis, corners[1]);
	triProj[2] = v2Dot(axis, corners[2]);
	Vec2 bShadow = v2(
			minf(minf(triProj[0], triProj[1]), triProj[2]),
			maxf(maxf(triProj[0], triProj[1]), triProj[2]));

	f32 rectProj[4];
	Vec2 hs = v2Scale(a->size, 0.5);
	rectProj[0] = v2Dot(axis, v2Sub(a->pos, hs)); 
	rectProj[1] = v2Dot(axis, v2Add(a->pos, hs)); 
	rectProj[2] = v2Dot(axis, v2AddXY(a->pos, -hs.x, hs.y)); 
	rectProj[3] = v2Dot(axis, v2AddXY(a->pos, hs.x, -hs.y)); 
	Vec2 aShadow = v2(
			minf(minf(minf(rectProj[0], rectProj[1]), rectProj[2]), rectProj[3]),
			maxf(maxf(maxf(rectProj[0], rectProj[1]), rectProj[2]), rectProj[3]));
	f32 total = bShadow.y - bShadow.x + aShadow.y - aShadow.x;
	f32 overlap = total - fabsf(
			(aShadow.x + aShadow.y) - (bShadow.x + bShadow.y));
	c->overlap = overlap * 0.5;
}

void collideCircleTriangle(SimWorld* sim, SimBody* a, SimBody* b, SimContact* c)
{
	Vec2 hypotN = getTriangleHypot(b->shape, b->size);
	Vec2 diff = v2Sub(b->pos, a->pos);
	if(v2Dot(diff, hypotN) > 0) {
		collideCircleBox(sim, a, b, c);
		return;
	}
	
	Vec2 corners[3];
	triGetCorners(b, corners);
	Vec2 closest = lineClosestPoint(corners[0], corners[1], a->pos);
	Vec2 axis = v2Normalize(v2Sub(a->pos, closest));
	
	// Project triangle
	f32 proj[3];
	proj[0] = v2Dot(axis, corners[0]);
	proj[1] = v2Dot(axis, corners[1]);
	proj[2] = v2Dot(axis, corners[2]);
	Vec2 bShadow = v2(
			minf(minf(proj[0], proj[1]), proj[2]),
			maxf(maxf(proj[0], proj[1]), proj[2]));
	f32 circleDot = v2Dot(a->pos, axis);
	Vec2 aShadow = v2(circleDot - a->size.x * 0.5, circleDot + a->size.x * 0.5);
	f32 overlap = bShadow.y - bShadow.x + aShadow.y - aShadow.x;
	overlap -= fabsf((aShadow.x + aShadow.y) - (bShadow.x + bShadow.y));
	c->overlap = overlap * 0.5;
	c->normal = v2Negate(axis);
}

void simHandleContact(SimWorld* sim, SimBody* a, SimBody* b)
{
	SimContact c = {0};
	i32 doSwap = 0;
	switch(a->shape) {
		case Shape_AABB:
			switch(b->shape) {
				case Shape_AABB:
					collideBoxBox(sim, a, b, &c);
					break;
				case Shape_Circle:
					collideCircleBox(sim, b, a, &c);
					doSwap = 1;
					break;
				case Shape_TriangleTL:
				case Shape_TriangleTR:
				case Shape_TriangleBR:
				case Shape_TriangleBL:
					collideBoxTriangle(sim, a, b, &c);
					break;
				default:
					return;
			}
			break;
		case Shape_Circle:
			switch(b->shape) {
				case Shape_AABB:
					collideCircleBox(sim, a, b, &c);
					break;
				case Shape_Circle:
					collideCircleCircle(sim, a, b, &c);
					break;
				case Shape_TriangleTL:
				case Shape_TriangleTR:
				case Shape_TriangleBR:
				case Shape_TriangleBL:
					collideCircleTriangle(sim, a, b, &c);
					break;
				default:
					return;
			}
			break;
			break;
		case Shape_TriangleTL:
		case Shape_TriangleTR:
		case Shape_TriangleBR:
		case Shape_TriangleBL:
			switch(b->shape) {
				case Shape_AABB:
					collideBoxTriangle(sim, b, a, &c);
					doSwap = 1;
					break;
				case Shape_Circle:
					collideCircleTriangle(sim, b, a, &c);
					doSwap = 1;
					break;
				case Shape_TriangleTL:
				case Shape_TriangleTR:
				case Shape_TriangleBR:
				case Shape_TriangleBL:
					// I never got this working, and it's not part of the
					// spec I have for the engine; I'm leaving it out :)
					// collideTriangleTriangle(sim, a, b, &c);
					break;
				default:
					return;
			}
			break;
			break;
		default:
			return;
	}

	if(doSwap) {
		SimBody* t = b;
		b = a;
		a = t;
	}

	if(c.overlap <= 0 || c.overlap == FLT_MAX) return;
	if(v2Mag2(c.normal) < 0.001f) return;

	f32 invMassSum = a->invMass + b->invMass;
	if(invMassSum == 0) return;
	invMassSum = 1.0 / invMassSum;
	f32 massA = a->invMass == 0 ? 0 : 1.0 / a->invMass;
	f32 massB = b->invMass == 0 ? 0 : 1.0 / b->invMass;
	Vec2 separation = v2Scale(
			c.normal,
			maxf(c.overlap - 0.1f, 0) * (invMassSum) * 0.5);

	if(a->invMass == 0) {
		v2AddScaledP(&b->pos, separation, 1);
	} else {
		v2AddScaledP(&b->pos, separation, b->invMass);
	}

	if(b->invMass == 0) {
		v2AddScaledP(&a->pos, separation, -1);
	} else {
		v2AddScaledP(&a->pos, separation, -1 * a->invMass);
	}

	Vec2 relative = v2Sub(b->vel, a->vel);
	f32 velNormal = v2Dot(relative, c.normal);
	if(velNormal < 0) {
		f32 elasticity = minf(a->restitution, b->restitution);
		f64 mag = -1 * (1 + elasticity) * velNormal;
		mag *= invMassSum;
		Vec2 impulse = v2Scale(c.normal, mag);
		v2AddScaledP(&a->vel, impulse, -1 * a->invMass);
		v2AddScaledP(&b->vel, impulse, b->invMass);

		Vec2 tangent = v2AddScaled(relative, c.normal, -v2Dot(relative, c.normal));
		f32 tangentMag = v2Mag2(tangent);
		if(tangentMag < 0.1) {
			goto CollisionEndif;
		}
		tangentMag = sqrtf(tangentMag);
		v2ScaleP(&tangent, 1.0 / tangentMag);

		f32 frictionMag = -v2Dot(relative, tangent);
		frictionMag *= invMassSum;

		// static friction
		// TODO(will): store static friction per-body/material
		f32 sfA = a->staticFriction;
		f32 sfB = b->staticFriction;

		f32 staticFriction = sqrtf(sfA * sfA + sfB * sfB);
		Vec2 frictionImpulse = v2(0, 0);
		if(fabsf(frictionMag) < (mag * staticFriction)) {
			frictionImpulse = v2Scale(tangent, frictionMag);
		} else {
			f32 dfA = a->dynamicFriction;
			f32 dfB = b->dynamicFriction;
			f32 dynamicFriction = sqrtf(dfA * dfA + dfB * dfB);
			frictionImpulse = v2Scale(tangent, -mag * dynamicFriction);
		}


		v2AddScaledP(&a->vel, frictionImpulse, -1 * a->invMass);
		v2AddScaledP(&b->vel, frictionImpulse, b->invMass);
CollisionEndif:;
	}
}

void sortContacts(SimContactPair* array, isize count)
{
	for(isize i = 1; i < count; ++i) {
		isize j = i - 1;

		i32 order = array[i].order;
		if(array[j].order > order) {
			SimContactPair temp = array[i];
			while((j >= 0) && (array[j].order > order)) {
				array[j + 1] = array[j];
				j--;
			}
			array[j + 1] = temp;
		}
	}
}


void simUpdate(SimWorld* sim, f32 dt)
{
	dt /= sim->iterations;
	f32 damping = powf(sim->damping, dt);

	Vec2 variance = {0}, centerSum1 = {0}, centerSum2 = {0};
	
	// priming
	// potentially we should have a temporary
	// array for proxies, and sort/iterate over those
	// which might be faster for big n
	for(isize i = 0; i < sim->bodyCount; ++i) {
		SimBody* a = sim->bodies[i];
		updateBodyBounds(a);
	}

	sim->contacts = sim->arena->head;
	for(isize times = 0; times < sim->iterations; ++times) {
		sim->arena->head = sim->contacts;
		sim->contactCount = 0;
		// loop over bodies, find collisions
		sortBodies(sim->bodies, sim->bodyCount, sim->sortAxis);
		for(isize i = 0; i < sim->bodyCount; ++i) {
			SimBody* a = sim->bodies[i];

			v2AddP(&centerSum1, a->pos);
			v2AddP(&centerSum2, v2Mul(a->pos, a->pos));

			for(isize j = i + 1; j < sim->bodyCount; ++j) {
				SimBody* b = sim->bodies[j];

				if(bodyMinSide(b, sim->sortAxis) > bodyMaxSide(a, sim->sortAxis)) {
					break;
				}

				if(aabbTest(a->boundingBox, b->boundingBox)) {
					simAddContactPair(sim, a, b);
				}
			}
		}
		
		// resolve collisions
		

		//TODO(will): Profile this
		//	Pros: better iCache perf
		//	Cons: hard work.
		//	Might only be worth doing if contactCount is greater than, like 1000?
		sortContacts(sim->contacts, sim->contactCount);
		for(isize i = 0; i < sim->contactCount; ++i) {
			SimContactPair* c = sim->contacts + i;
			simHandleContact(sim, c->a, c->b);
		}
		
		// loop over bodies, integrate
		// potentially switch these
		for(isize i = 0; i < sim->bodyCount; ++i) {
			SimBody* a = sim->bodies[i];
			if(a->invMass == 0) {
				a->vel = v2(0, 0);
				a->acl = v2(0, 0);
				continue;
			}
			//Vec2 oldVel = a->vel;
			v2AddScaledP(&a->vel, a->acl, dt);
			//v2AddScaledP(&a->pos, v2Add(a->vel, oldVel), 0.5 * dt);
			v2AddScaledP(&a->pos, a->vel, 0.5 * dt);

#if Sim_DoGroundFriction
			// Ground Friction
			if(v2Mag2(a->vel) > 0.01f) {
				f32 gravityMag = -1 * GravityConstant;
				Vec2 tangent = v2Normalize(v2Negate(a->vel));
				f32 frictionMag = -v2Dot(v2Negate(a->vel), tangent) * a->invMass;
				f32 mu = a->staticFriction * a->groundFriction;
				Vec2 frictionImpulse;
				if(fabsf(frictionMag) < gravityMag * mu) {
					frictionImpulse = v2Scale(tangent, frictionMag);
				} else {
					f32 dyn = a->dynamicFriction * a->groundFriction;
					frictionImpulse = v2Scale(tangent, -gravityMag * dyn);
				}

				v2AddScaledP(&a->vel, frictionImpulse, a->invMass);
			}
#endif
			
		v2ScaleP(&a->vel, damping);

			updateBodyBounds(a);
		}
		// centerSum2 
		variance = v2Sub(
				centerSum2,
				v2Scale(v2Mul(centerSum1, centerSum1),
					1.0 / (f32)sim->bodyCount));
		if(variance.x > variance.y) {
			sim->sortAxis = 0;
		} else {
			sim->sortAxis = 1;
		}
	}

	for(isize i = 0; i < sim->bodyCount; ++i) {
		SimBody* a = sim->bodies[i];
		f32 velMag = v2Mag2(a->vel);
		if(velMag < 0.1) {
			a->vel = v2(0, 0);
		} else if (velMag > MaxVelocitySq) {
			f32 mag = v2Mag(a->vel);
			f32 ratio = MaxVelocity / mag;
			v2ScaleP(&a->vel, ratio);
		}
	}
}

/*
void collideTriangleTriangle(SimWorld* sim, SimBody* a, SimBody* b, SimContact* c)
{
	// This is fairly vanilla SAT as far as I can tell
	// but it still doesn't work >:(
	// Solution: never have dynamic triangles in the engine, okay?
	Vec2 cornersA[3], cornersB[3];
	triGetCorners(a, cornersA);
	triGetCorners(b, cornersB);
	c->overlap = FLT_MAX;
	Vec2 axes[6];
	printf("\n");
	for(isize i = 0; i < 3; ++i) {
		axes[i] = v2Normalize(v2Perpendicular(v2Sub(cornersA[i], cornersA[(i+1)%3])));
		axes[i+3] = v2Normalize(v2Perpendicular(v2Sub(cornersB[i], cornersB[(i+1)%3])));
		printf("%f %f | %f %f\n", axes[i].x, axes[i].y, axes[i+3].x, axes[i+3].y);
	}


	for(isize i = 0; i < 6; ++i) {
		Vec2 axis = axes[i];
		f32 proj[3];
		proj[0] = v2Dot(axis, cornersA[0]);
		proj[1] = v2Dot(axis, cornersA[1]);
		proj[2] = v2Dot(axis, cornersA[2]);
		Vec2 aShadow = v2(
				minf(minf(proj[0], proj[1]), proj[2]),
				maxf(maxf(proj[0], proj[1]), proj[2]));
		proj[0] = v2Dot(axis, cornersB[0]);
		proj[1] = v2Dot(axis, cornersB[1]);
		proj[2] = v2Dot(axis, cornersB[2]);
		Vec2 bShadow = v2(
				minf(minf(proj[0], proj[1]), proj[2]),
				maxf(maxf(proj[0], proj[1]), proj[2]));
		f32 overlap = fabsf(aShadow.y - aShadow.x) + fabsf(bShadow.y - bShadow.x);
		overlap -= fabsf((aShadow.x + aShadow.y) - (bShadow.x + bShadow.y));
		overlap *= 0.5;
		if(overlap < 0) {
			//no intersection
			c->overlap = 0;
			return;
		} else if(overlap < c->overlap) {
			//if(v2Dot(c->normal, v2Sub(b->pos, a->pos)) > 0) continue;
			c->overlap = overlap;
			c->normal = axis;
		}
	}
}
*/
