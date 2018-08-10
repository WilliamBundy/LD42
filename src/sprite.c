

float AnchorX[] = {0.0, 0.5, 0.0, -0.5, -0.5, -0.5,  0.0,  0.5, 0.5};
float AnchorY[] = {0.0, 0.5, 0.5,  0.5,  0.0, -0.5, -0.5, -0.5, 0.0}; 

#ifndef WirmphtEnabled
typedef enum SpriteFlags
{
	Anchor_Center,
	Anchor_TopLeft,
	Anchor_Top,
	Anchor_TopRight,
	Anchor_Right,
	Anchor_BottomRight,
	Anchor_Bottom,
	Anchor_BottomLeft,
	Anchor_Left,
	Sprite_Disabled = 1<<4,
	Sprite_NoTexture = 1<<5,
	Sprite_RotateCW = 1<<6,
	Sprite_RotateCCW = 1<<7,
	Sprite_FlipVert = 1<<9,
	Sprite_FlipHoriz = 1<<8,
	Sprite_Circle = 1<<10,
	Sprite_SDF = 1<<11,
	Sprite_NoAA = 1<<12,
	
	// Various lighting things
	//Sprite_NoTint
	//Sprite_Shadow
	//Sprite_
} SpriteFlags;

struct Sprite
{
	u32 flags;
	u32 color;
	union {
		struct {f32 x, y;};
		Vec2 pos;
	};
	f32 z;
	f32 angle;
	union {
		struct {f32 w, h;};
		Vec2 size;
	};
	union {
		struct {f32 cx, cy;};
		Vec2 center;
	};
	union {
		struct{i16 tx, ty, tw, th;};
		Rect2i texture;
	};
};


struct SpriteHandle
{
	f32 sort;
	i32 id;
};

struct SpriteList
{
	isize start, count;
	union {
		struct {f32 l, t, r, b;};
		AABB box;
	};
}

struct SpriteBatch
{
	wRenderBatch batch;
	SpriteHandle* handles;
	Sprite* sprites;
	Sprite* sprites2;
	isize count, capacity;

	union {
		struct {f32 x, y;};
		Vec2 pos;
	};
	f32 vw, vh;
	f32 scale;
	u32 tint;
	f32 itw, ith;
};
#endif

void initSprite(Sprite* s,
		f32 flags, u32 color, 
		f32 x, f32 y, f32 z,
		f32 angle, f32 w, f32 h, f32 cx, f32 cy, 
		i16 tx, i16 ty, i16 tw, i16 th)
{
	s->flags = flags;
	s->color = color;
	s->x = x;
	s->y = y;
	s->z = z;
	s->angle = angle;
	s->w = w;
	s->h = h;
	s->cx = cx;
	s->cy = cy;
	s->tx = tx;
	s->ty = ty;
	s->tw = tw;
	s->th = th;
}

Sprite makeSprite(Vec2 pos, Vec2 size, Rect2i texture)
{
	Sprite s;
	initSprite(&s, 
			0, 0xFFFFFFFF,
			pos.x, pos.y, 0,
			0, 
			size.x, size.y,
			0, 0,
			texture.x, texture.y, texture.w, texture.h);
	return s;
}

Sprite makePrimitive(Vec2 pos, Vec2 size, u32 color, i32 flags)
{
	Sprite s;
	initSprite(&s, 
			Sprite_NoTexture | flags, color,
			pos.x, pos.y, 0,
			0, 
			size.x, size.y,
			0, 0,
			0, 0, 0, 0);
	return s;
}

wTextureSegmentGrid makeWholeFileGrid(string name)
{
	wTextureSegmentGrid g = {0, 0, 0, 0, 0, (string*)name, 0, 0};
	return g;
}

void updateGraphicsDependencies()
{
	i32 rebuildShaders = 0;
	if(wCheckHotFile(game.fragShader) || wCheckHotFile(game.vertShader)) {
		wLogError(0, "Updated shaders\n");
		wUpdateHotFile(game.vertShader);
		wUpdateHotFile(game.fragShader);
		wLogError(0, "vert size %d\n", game.vertShader->size);
		wLogError(0, "fragSize size %d\n", game.fragShader->size);
		wDeleteShaderProgram(game.shader);
		wAddSourceToShader(game.shader, game.vertShader->data, wShader_Vertex);
		wAddSourceToShader(game.shader, game.fragShader->data, wShader_Frag);
		wFinalizeShader(game.shader);
	}
}

void addTextureToGame(string file, wTextureSegmentGrid grid)
{
	game.textureNames[game.textureCount] = file;
	game.textureGrids[game.textureCount++] = grid;
}

Rect2i getSegment(string name)
{
	wTextureSegment* seg = wFindSegment(&game.atlas, name);
	return r2i(seg->x, seg->y, seg->w, seg->h);
}
	
Sprite* getSprite(SpriteBatch* batch)
{
	if(batch->count >= batch->capacity) return NULL;
	Sprite* local = batch->sprites + batch->count++;
	return local;
}

Sprite* addSprite(SpriteBatch* batch, Sprite sprite)
{
	if(batch->count >= batch->capacity) return NULL;
	Sprite* local = batch->sprites + batch->count++;
	*local = sprite;
	return local;
}

void createGraphicsDependencies()
{
	wInitAtlas(&game.atlas, 2048, game.arena);
	for(isize i = 0; i < game.textureCount; ++i) {
		game.textures[i] = wCreateHotFile(&game.window, game.textureNames[i]);
		wHotFile* texture = game.textures[i];
		game.textureData[i] = wArenaPush(game.arena, sizeof(wTexture));
		wInitTexture(game.textureData[i], texture->data, texture->size);
		wTextureSegmentGrid* g = game.textureGrids + i;
		wAddSegmentGrid(&game.atlas, game.textureData[i], g);
	}

	wAtlasCreateTexture(&game.atlas, 2048, game.arena);
	wArenaStartTemp(game.arena);
	wFinalizeAtlas(&game.atlas, game.arena);
	wArenaEndTemp(game.arena);
	wUploadTexture(&game.atlas.texture);
	game.texture = &game.atlas.texture;


#if 0
	isize textureSize = 0;
	game.textureFile = wCreateHotFile(&game.window, "assets/art.png");
	game.texture = wArenaPush(game.arena, sizeof(wTexture));
	wInitTexture(game.texture, game.textureFile->data, game.textureFile->size);
	wUploadTexture(game.texture);
#endif

	game.vertShader = wCreateHotFile(&game.window, "assets/GL33_vert.glsl");
	game.fragShader = wCreateHotFile(&game.window, "assets/GL33_frag.glsl");
	game.vertShader->replaceBadSpaces = 1;
	game.fragShader->replaceBadSpaces = 1;
	wLogError(0, "frag size: %zd\nvert size: %zd\ntex size %zd\n",
			game.fragShader->size,
			game.vertShader->size,
			0);

	game.shader = wArenaPush(game.arena, sizeof(wShader));
	wInitShader(game.shader, sizeof(Sprite));
	game.shader->defaultDivisor = 1;

	wCreateAttrib(game.shader,
			"vFlags", wShader_Int, 1, offsetof(Sprite, flags));
	wCreateAttrib(game.shader, 
			"vColor", wShader_NormalizedByte, 4, offsetof(Sprite, color));
	wCreateAttrib(game.shader,
			"vPos", wShader_Float, 3, offsetof(Sprite, x));
	wCreateAttrib(game.shader,
			"vAngle", wShader_Float, 1, offsetof(Sprite, angle));
	wCreateAttrib(game.shader,
			"vSize", wShader_Float, 2, offsetof(Sprite, w));
	wCreateAttrib(game.shader, 
			"vCenter", wShader_Float, 2, offsetof(Sprite, cx));
	wCreateAttrib(game.shader, 
			"vTexture", wShader_FloatShort, 4, offsetof(Sprite, tx));

	wCreateUniform(game.shader, 
			"uOffset", wShader_Float, 2, offsetof(SpriteBatch, x));
	wCreateUniform(game.shader, 
			"uViewport", wShader_Float, 2, offsetof(SpriteBatch, vw));
	wCreateUniform(game.shader, 
			"uScale", wShader_Float, 1, offsetof(SpriteBatch, scale));
	wCreateUniform(game.shader, 
			"uTint", wShader_NormalizedByte, 4, offsetof(SpriteBatch, tint));
	wCreateUniform(game.shader, 
			"uInvTextureSize", wShader_Float, 2, offsetof(SpriteBatch, itw));


	wAddSourceToShader(game.shader, game.vertShader->data, wShader_Vertex);
	wAddSourceToShader(game.shader, game.fragShader->data, wShader_Frag);

	wFinalizeShader(game.shader);
}

SpriteBatch* createSpriteBatch(isize cap, wMemoryArena* arena)
{
	SpriteBatch* batch = wArenaPush(arena, sizeof(SpriteBatch));
	batch->handles = wArenaPush(arena, sizeof(SpriteHandle) * cap);
	batch->sprites = wArenaPush(arena, sizeof(Sprite) * cap);
	batch->sprites2 = wArenaPush(arena, sizeof(Sprite) * cap);
	batch->capacity = cap;
	wInitBatch(&batch->batch,
			game.texture, game.shader,
			wRenderBatch_ArraysInstanced, wRenderBatch_TriangleStrip,
			sizeof(Sprite), 4,
			batch->sprites, NULL);
	wConstructBatchGraphicsState(&batch->batch);

	batch->scale = 1.0;
	batch->tint = 0xFFFFFFFF;
	batch->batch.blend = wRenderBatch_BlendPremultiplied;
	return batch;
}

void drawSprites(SpriteBatch* batch)
{
	wRenderBatch* rb = &batch->batch;
	rb->elementCount = batch->count;
	batch->vw = game.state.width;
	batch->vh = game.state.height;
	batch->itw = 1.0f / rb->texture->w;
	batch->ith = 1.0f / rb->texture->h;

	wDrawBatch(&game.state, rb, batch);

	batch->count = 0;
}

void addTriangle(f32 x, f32 y, f32 sx, f32 sy, i32 corner)
{
	corner -= Shape_TriangleTL;
	i32 lut[] = {
		0, 32, 32, 0,
		0, 0, 32, 32
	};
	Sprite s = {0};
	initSprite(&s, 
			0, 0xFFFFFFAA,
			x, y, 0, 0,
			sx, sy,
			0, 0,
			48 + lut[corner], 16 + lut[corner + 4], 32, 32);
	game.batch->sprites[game.batch->count++] = s;
}

Sprite* addSquare(f32 x, f32 y, f32 sx, f32 sy, u32 flags)
{
	Sprite s = {0};
	initSprite(&s, 
			flags, 0xFFFFFFAA,
			x, y, 0, 0,
			sx, sy,
			0, 0,
			0, 0, 32, 32);
	game.batch->sprites[game.batch->count++] = s;
	return game.batch->sprites + game.batch->count - 1;
}

void addPoint(f32 x, f32 y, u32 c)
{
	Sprite s = {0};
	initSprite(&s, 
			Sprite_NoTexture | Sprite_Circle, c,
			x, y, 0, 0,
			4 / game.batch->scale, 4 / game.batch->scale,
			0, 0,
			8, 8, 8, 8);
	game.batch->sprites[game.batch->count++] = s;

}


SpriteList drawMonoText(SpriteBatch* batch, Vec2 pos, string text, i32 color)
{
	return renderText(batch, game.monoFont, pos.x, pos.y, text, -1, 
			14.0 / batch->scale, 0, color ? 0xFFFFFFFF : 0xFF, 1, 1);
}

SpriteList drawUiText(Gui* gui, Vec2 pos, string text, i32 color)
{
	return renderText(gui->batch, game.bodyFont, pos.x, pos.y, text, -1, 
			16.0  * gui->textScale / gui->batch->scale, 0, color ? 0xFFFFFFFF : 0xFF, 0.95, 0);
}

SpriteList drawBodyText(SpriteBatch* batch, Vec2 pos, string text, i32 color)
{
	return renderText(batch, game.bodyFont, pos.x, pos.y, text, -1, 
			16.0 / batch->scale, 0, color ? 0xFFFFFFFF : 0xFF, 0.95, 0);
}

SpriteList drawTitleText(SpriteBatch* batch, Vec2 pos, string text, i32 color, f32 scale)
{
	return renderText(batch, game.titleFont, pos.x, pos.y, text, -1, 
			32.0 * scale / batch->scale, 0, color ? 0xFFFFFFFF : 0xFF, 0.85, 0);
}

#define DPI (72.0)
SpriteList renderText(SpriteBatch* batch,
		wFontInfo* info,
		f32 x, f32 y,
		string text, isize count,
		f32 pointSize, i32 flags, 
		u32 color, f32 tracking, i32 isMono)
{
	if(count == -1) count = strlen(text);
	f32 ox = 0.0f, oy = 0.0f;
	char last = 0;

	f32 pixelSize = (pointSize * DPI) / 72.0f;
	f32 padding = (f32)info->pxRange;
	f32 fontScale = info->scale;
	//f32 tracking = 0.85;

	wGlyphImage* a = info->images + ('A'-32);
	wGlyph* g = info->glyphs + ('A'-32);
	
	f32 glyphHeight = fabsf(g->t - g->b);
	f32 scaledHeight = glyphHeight * fontScale;
	f32 scaledRatio = pixelSize / scaledHeight;
	f32 heightRatio = pixelSize / glyphHeight;

	f32 maxX = 0;
	f32 widthP = 0;

	SpriteList l;
	l.start = batch->count;

	f32 yoffset = a->bby * scaledRatio * 0.5;

	i32 newLine = 1;
	for(isize i = 0; i < count; ++i) {
		char c = text[i];
		switch(c) {
			case '\r':
				continue;

			case '\n':
				if(ox > maxX) maxX = ox;
				ox = 0;
				oy += info->lineSpacing * heightRatio;
				newLine = 1;
				continue;

			case '\t':
				ox += info->glyphs[0].advance * heightRatio * 8;
				continue;

			case ' ':
				ox += info->glyphs[0].advance * heightRatio;
				continue;
		}
		if(c <= 32 || c >= 127) continue;
		a = info->images + (c-32);
		g = info->glyphs + (c-32);

		if(last > 32 && last < 127) {
			f32 k = info->kerning[last-32][c-32] * pixelSize * fontScale * 0.5;
			ox += k * tracking;
		}

		f32 gx = (a->bbx - padding) * scaledRatio;
		
		if(!isMono && newLine) {
			//TODO(will) figure out a good way to properly align text
			//	because right now we end up with weird offset issues due 
			//	to padding at the left margin.
			//
			//	Oh, and maybe more complex text layout stuff so we don't
			//	have left-align everything?
			if(i == 0) widthP += gx;
			ox -= gx;// * 1.25;
			newLine = 0;
		}
		
		Sprite s;
		initSprite(&s, flags | Sprite_SDF | Anchor_TopLeft, color,
				x + ox + gx, y + oy - yoffset, 0, 0,
				a->w * scaledRatio, a->h * scaledRatio,
				0, 0,
				a->x + info->atlasX, a->y + info->atlasY,
				a->w, a->h);
		addSprite(batch, s);
		ox += g->advance * heightRatio * tracking;
		if(ox > maxX) maxX = ox;
		last = c;
	}
	
	l.count = batch->count - l.start;
	f32 gx = (a->bbx - padding) * scaledRatio;
	l.l = x + gx;
	l.t = y + yoffset * 2;
	l.r = x + maxX + widthP + info->images[(text[count-1]-32)].w * scaledRatio - gx * 0.5;
	l.b = y + oy + a->h * scaledRatio + yoffset;
	
	return l;
}

void drawBorderedBox(SpriteBatch* batch, Vec2 pos, Vec2 size, u32 bg, u32 border)
{
	addSprite(batch, makePrimitive(
				pos, size,
				bg, Anchor_TopLeft));
	addSprite(batch, makePrimitive(
				pos, v2(size.x, 2), 
				border, Anchor_TopLeft));
	addSprite(batch, makePrimitive(
				v2AddXY(pos, 0, size.y), v2(size.x, 2), 
				border, Anchor_BottomLeft));
	addSprite(batch, makePrimitive(
				pos, v2(2, size.y), 
				border, Anchor_TopLeft));
	addSprite(batch, makePrimitive(
				v2AddXY(pos, size.x, 0), v2(2, size.y), 
				border, Anchor_TopRight));
}
void drawBorderedBoxThickness(SpriteBatch* batch, Vec2 pos, Vec2 size, u32 bg, u32 border, f32 thickness)
{
	addSprite(batch, makePrimitive(
				pos, size,
				bg, Anchor_TopLeft));
	addSprite(batch, makePrimitive(
				pos, v2(size.x, thickness), 
				border, Anchor_TopLeft));
	addSprite(batch, makePrimitive(
				v2AddXY(pos, 0, size.y), v2(size.x, thickness), 
				border, Anchor_BottomLeft));
	addSprite(batch, makePrimitive(
				pos, v2(thickness, size.y), 
				border, Anchor_TopLeft));
	addSprite(batch, makePrimitive(
				v2AddXY(pos, size.x, 0), v2(thickness, size.y), 
				border, Anchor_TopRight));
}


void internalSpriteHandleSort(SpriteHandle* handles, isize count)
{
	if(count <= 1) return;

	if(count > 140) {
		isize pivot = count / 3;
		SpriteHandle tmp = handles[pivot];
		handles[pivot] = handles[0];
		handles[0] = tmp;
		pivot = 0;
		for(isize i = 1; i < count; ++i) {
			if(handles[i].sort < handles[0].sort) {
				tmp = handles[++pivot];
				handles[pivot] = handles[i];
				handles[i] = tmp;
			}
		}
		tmp = handles[0];
		handles[0] = handles[pivot];
		handles[pivot] = tmp;
		internalSpriteHandleSort(handles, pivot);
		internalSpriteHandleSort(handles + pivot + 1, count - (pivot + 1));
	} else {
		for(isize i = 1; i < count; ++i) {
			isize j = i - 1;

			f32 base = handles[i].sort;
			if(handles[j].sort > base) {
				SpriteHandle temp = handles[i];
				while((j >= 0) && (handles[j].sort > base)) {
					handles[j + 1] = handles[j];
					j--;
				}
				handles[j + 1] = temp;
			}
		}
	}
}

void sortSpritesInBatch(SpriteBatch* batch, isize start, isize count)
{
	SpriteHandle* handles = batch->handles;
	Sprite* array = batch->sprites + start;
	for(isize i = 0; i < count; ++i) {
		handles[i].sort = array[i].y - array[i].z;
		handles[i].id = i;
	}
		
	internalSpriteHandleSort(handles, count);

	/*
	for(isize i = 1; i < count; ++i) {
		isize j = i - 1;

		f32 base = handles[i].sort;
		if(handles[j].sort > base) {
			SpriteHandle temp = handles[i];
			while((j >= 0) && (handles[j].sort > base)) {
				handles[j + 1] = handles[j];
				j--;
			}
			handles[j + 1] = temp;
		}
	}
	*/

	Sprite* array2 = batch->sprites2 + start;
	for(isize i = 0; i < count; ++i) {
		array2[i] = array[handles[i].id];
	}
	memcpy(array, array2, count);
	//array = batch->sprites;
	//batch->sprites = batch->sprites2;
	//batch->sprites2 = array;
}

void sortSprites(Sprite* array, isize count)
{

	for(isize i = 1; i < count; ++i) {
		isize j = i - 1;

		f32 base = array[i].y - array[i].z;
		if((array[j].y - array[j].z) > base) {
			Sprite temp = array[i];
			while((j >= 0) && ((array[j].y - array[j].z) > base)) {
				array[j + 1] = array[j];
				j--;
			}
			array[j + 1] = temp;
		}
	}

}
