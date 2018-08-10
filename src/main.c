#define WirmphtEnabled
#define $(...)

$(exclude);
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "wpl/wpl.h"
#define WBTM_API static
#include "wpl/thirdparty/wb_tm.c"
#include "generated.h"
$(end);

Game* game = NULL;

#include "inputUtil.c"
#include "random.c"
#include "vectormath.c"
#include "simulation.c"
#include "sprite.c"
#include "ui.c"

#include "assets.c"

#ifndef WirmphtEnabled
struct Game
{
	wMemoryInfo memInfo;
	wMemoryArena* arena;
	wWindow* window;
	wState* state;
	wInputState* input;
	wMixer* mixer;

	wTextureAtlas* atlas;
	isize textureCount;
	string textureNames[32];
	wTextureSegmentGrid textureGrids[32];
	wHotFile* textures[32]; 
	wTexture* textureData[32];

	wHotFile* fragShader;
	wHotFile* vertShader;
	wShader* shader;
	wTexture* texture;

	wFontInfo* bodyFont;
	wFontInfo* monoFont;
	wFontInfo* titleFont;

	SpriteBatch* batch;
};
#endif

void load()
{
	assets_loadFonts();
}

void init()
{
	assets_initFonts();
}

i32 checkUpdateTimer = 60;
void update()
{
	{
		checkUpdateTimer--;
		if(checkUpdateTimer <= 0) {
			updateGraphicsDependencies();
			checkUpdateTimer += 60;
		}
	}

	drawTitleText(game->batch, v2(100, 100), "Hello Ludum Dare 42!!!", 1, 1);
	drawSprites(game->batch);

	// game loop here
}

int main(int argc, char** argv)
{
	{
		wMemoryInfo memInfo = wGetMemoryInfo();
		wMemoryArena* arena = wArenaBootstrap(memInfo, 0);
		game = wArenaPush(arena, sizeof(Game));
		game->arena = arena;
		game->window = wArenaPush(arena, sizeof(wWindow));
		game->state = wArenaPush(arena, sizeof(wState));
		game->input = wArenaPush(arena, sizeof(wInputState));
		game->mixer = wArenaPush(arena, sizeof(wMixer));
	}

	wWindowDef def = wDefineWindow("LUDUM DARE 42");
	
	wCreateWindow(&def, game->window);
	wInitState(game->window, game->state, game->input);
	wInitAudio(game->window, game->mixer, 32, game->arena);
	 
	load();

	createGraphicsDependencies();
	game->batch = createSpriteBatch(4096 * 4096, game->arena);
	
	init();

	while(!game->state->exitEvent) {
		wUpdate(game->window, game->state);
		update();
		wRender(game->window);
	}

	wQuit();
	return 0;
}

