typedef struct RandomState RandomState;
typedef union Vec2 Vec2;
typedef union Vec2i Vec2i;
typedef struct AABB AABB;
typedef union Rect2f Rect2f;
typedef struct Rect2i Rect2i;
typedef union Rect2l Rect2l;
typedef struct SimContactPair SimContactPair;
typedef struct SimContact SimContact;
typedef struct SimBody SimBody;
typedef struct SimWorld SimWorld;
typedef struct Sprite Sprite;
typedef struct SpriteHandle SpriteHandle;
typedef struct SpriteList SpriteList;
typedef struct SpriteBatch SpriteBatch;
typedef struct Gui Gui;
typedef struct Game Game;
typedef enum SpriteFlags SpriteFlags;
typedef enum SpriteFlags{ Anchor_Center, Anchor_TopLeft, Anchor_Top, Anchor_TopRight, Anchor_Right, Anchor_BottomRight, Anchor_Bottom, Anchor_BottomLeft, Anchor_Left, Sprite_Disabled=1<<4, Sprite_NoTexture=1<<5, Sprite_RotateCW=1<<6, Sprite_RotateCCW=1<<7, Sprite_FlipVert=1<<9, Sprite_FlipHoriz=1<<8, Sprite_Circle=1<<10, Sprite_SDF=1<<11, Sprite_NoAA=1<<12,} SpriteFlags;
typedef enum{ Gui_None, Gui_IncrementX=1<<1, Gui_IncrementY=1<<2, Gui_LabelBackground=1<<3, Gui_FixedSize=1<<4, Gui_Flow=1<<5} GuiFlags;

struct RandomState
{
	u64 x;
	u64 y;
};

union Vec2
{
	struct {
		f32 x;
		f32 y;
	};
	f32 e[2];
};

union Vec2i
{
	struct {
		i32 x;
		i32 y;
	};
	i32 e[2];
};

struct AABB
{
	Vec2 min;
	Vec2 max;
};

union Rect2f
{
	struct {
		Vec2 pos;
		Vec2 size;
	};
	struct {
		f32 x;
		f32 y;
		f32 w;
		f32 h;
	};
	f32 e[4];
};

struct Rect2i
{
	i16 x;
	i16 y;
	i16 w;
	i16 h;
};

union Rect2l
{
	struct {
		i32 x;
		i32 y;
		i32 w;
		i32 h;
	};
	struct {
		Vec2i pos;
		Vec2i size;
	};
};

struct SimContactPair
{
	SimBody *a;
	SimBody *b;
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
	Vec2 pos;
	Vec2 vel;
	Vec2 acl;
	Vec2 size;
	Vec2 correction;
	AABB boundingBox;
	f32 staticFriction;
	f32 dynamicFriction;
	f32 groundFriction;
	f32 restitution;
	f32 invMass;
	u32 flags;
	i32 shape;
	i32 index;
};

struct SimWorld
{
	wMemoryArena *arena;
	wMemoryPool *bodyPool;
	SimBody **bodies;
	SimBody *bodyStorage;
	isize bodyCount;
	isize bodyCapacity;
	SimContactPair *contacts;
	isize contactCount;
	i32 iterations;
	f32 damping;
	i32 sortAxis;
};

struct Sprite
{
	u32 flags;
	u32 color;
	union {
		struct {
			f32 x;
			f32 y;
		};
		Vec2 pos;
	};
	f32 z;
	f32 angle;
	union {
		struct {
			f32 w;
			f32 h;
		};
		Vec2 size;
	};
	union {
		struct {
			f32 cx;
			f32 cy;
		};
		Vec2 center;
	};
	union {
		struct {
			i16 tx;
			i16 ty;
			i16 tw;
			i16 th;
		};
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
	isize start;
	isize count;
	union {
		struct {
			f32 l;
			f32 t;
			f32 r;
			f32 b;
		};
		AABB box;
	};
};

struct SpriteBatch
{
	wRenderBatch batch;
	SpriteHandle *handles;
	Sprite *sprites;
	Sprite *sprites2;
	isize count;
	isize capacity;
	union {
		struct {
			f32 x;
			f32 y;
		};
		Vec2 pos;
	};
	f32 vw;
	f32 vh;
	f32 scale;
	u32 tint;
	f32 itw;
	f32 ith;
};

struct Gui
{
	SpriteBatch *batch;
	Vec2 mouse;
	i32 mouseState;
	Vec2 pos;
	Vec2 size;
	Vec2 startPos;
	Vec2 maxSize;
	f32 textScale;
	Vec2 padding;
	u32 bgColor;
	u32 hotColor;
	u32 activeColor;
	u32 textColor;
	u32 borderColor;
	u32 selectedColor;
	i32 anchor;
	i32 isActive;
	u32 flags;
};

struct Game
{
	wMemoryInfo memInfo;
	wMemoryArena *arena;
	wWindow *window;
	wState *state;
	wInputState *input;
	wMixer *mixer;
	wHotFile *fragShader;
	wHotFile *vertShader;
	wShader *shader;
	wTexture *texture;
	wFontInfo *bodyFont;
	wFontInfo *monoFont;
	wFontInfo *titleFont;
};

u64 u64rand(RandomState* r);
void initRandXY(RandomState* r, u64 baseSeed, u64 seedX, u64 seedY);
void initRand(RandomState* r, u64 seed);
f64 f64rand(RandomState* r);
f32 f32rand(RandomState* r);
isize randrange(RandomState* r, isize mn, isize mx);
f64 f64randrange(RandomState* r, f64 mn, f64 mx);
f32 f32randrange(RandomState* r, f64 mn, f64 mx);
Vec2 vec2rand(RandomState* r, f32 s, Vec2 base);
f32 v2Dist(Vec2 a, Vec2 b);
Vec2 lineClosestPoint(Vec2 a, Vec2 b, Vec2 p);
Vec2 triClosestPoint(Vec2 a, Vec2 b, Vec2 c, Vec2 p);
void initBody(SimBody* body);
SimBody* simAddBox(SimWorld* sim, Vec2 pos, Vec2 size);
SimBody* simAddCircle(SimWorld* sim, Vec2 pos, f32 diameter);
SimBody* simAddBody(SimWorld* sim, Vec2 pos, Vec2 size, i32 shape);
SimWorld* createSimWorld(isize bodyCapacity);
void sortBodies(SimBody** array, isize count, i32 axis);
void simAddContactPair(SimWorld* sim, SimBody* a, SimBody* b);
void collideBoxBox(SimWorld* sim, SimBody* a, SimBody* b, SimContact* c);
void collideCircleBox(SimWorld* sim, SimBody* a, SimBody* b, SimContact* c);
void collideCircleCircle(SimWorld* sim, SimBody* a, SimBody* b, SimContact* c);
void collideBoxTriangle(SimWorld* sim, SimBody* a, SimBody* b, SimContact* c);
void collideCircleTriangle(SimWorld* sim, SimBody* a, SimBody* b, SimContact* c);
void simHandleContact(SimWorld* sim, SimBody* a, SimBody* b);
void sortContacts(SimContactPair* array, isize count);
void simUpdate(SimWorld* sim, f32 dt);
void initSprite(Sprite* s, f32 flags, u32 color, f32 x, f32 y, f32 z, f32 angle, f32 w, f32 h, f32 cx, f32 cy, i16 tx, i16 ty, i16 tw, i16 th);
Sprite makeSprite(Vec2 pos, Vec2 size, Rect2i texture);
Sprite makePrimitive(Vec2 pos, Vec2 size, u32 color, i32 flags);
wTextureSegmentGrid makeWholeFileGrid(string name);
void updateGraphicsDependencies();
void addTextureToGame(string file, wTextureSegmentGrid grid);
Rect2i getSegment(string name);
Sprite* getSprite(SpriteBatch* batch);
Sprite* addSprite(SpriteBatch* batch, Sprite sprite);
void createGraphicsDependencies();
SpriteBatch* createSpriteBatch(isize cap, wMemoryArena* arena);
void drawSprites(SpriteBatch* batch);
SpriteList drawMonoText(SpriteBatch* batch, Vec2 pos, string text, i32 color);
SpriteList drawUiText(Gui* gui, Vec2 pos, string text, i32 color);
SpriteList drawBodyText(SpriteBatch* batch, Vec2 pos, string text, i32 color);
SpriteList drawTitleText(SpriteBatch* batch, Vec2 pos, string text, i32 color, f32 scale);
SpriteList renderText(SpriteBatch* batch, wFontInfo* info, f32 x, f32 y, string text, isize count, f32 pointSize, i32 flags, u32 color, f32 tracking, i32 isMono);
void drawBorderedBox(SpriteBatch* batch, Vec2 pos, Vec2 size, u32 bg, u32 border);
void drawBorderedBoxThickness(SpriteBatch* batch, Vec2 pos, Vec2 size, u32 bg, u32 border, f32 thickness);
void internalSpriteHandleSort(SpriteHandle* handles, isize count);
void sortSpritesInBatch(SpriteBatch* batch, isize start, isize count);
void sortSprites(Sprite* array, isize count);
void guiInit(Gui* gui, SpriteBatch* batch);
void guiStart(Gui* gui);
void guiEnd(Gui* gui);
AABB guiGetAABB(Gui* gui, Vec2 pos, Vec2 size);
i32 guiCheckAABB(Gui* gui, AABB box);
void guiLayout(Gui* gui, Vec2* pos, Vec2* size);
void guiSetNextPos(Gui* gui, Vec2 size);
void guiGetSizeFromText(Gui* gui, SpriteList* textSprites, Vec2* sizeOut, Vec2* textSizeOut);
void guiWidgetWarn(Gui* gui);
void guiDrawBorder(Gui* gui, Vec2 pos, Vec2 size);
i32 guiBtn(Gui* gui, string text);
i32 guiIcoSelector(Gui* gui, Rect2i icon, i32 selected, f32 scale);
i32 guiIcoBtn(Gui* gui, Rect2i icon);
void guiLabel(Gui* gui, string text);
void guiLabelEx(Gui* gui, string text, wFontInfo* font, f32 pointSize, f32 tracking, i32 isMono);
void load();
void init();
void update();
int main(int argc, char** argv);
