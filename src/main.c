#define WirmphtEnabled
#define $(...)

$(exclude);
//#include <float.h>
#define FLT_MAX 3.402823e+38 
#include "wpl/wpl.h"
#define WBTM_API static
#define WBTM_CRT_REPLACE
#include "wpl/thirdparty/wb_tm.c"
$(end);

Game game = {0};

#include "inputUtil.c"
#include "random.c"
#include "vectormath.c"
#include "simulation.c"
#include "sprite.c"
#include "ui.c"

#ifndef WirmphtEnabled
struct Game
{
	wMemoryInfo memInfo;
	wMemoryArena* arena;
	wWindow window;
	wState state;
	wInputState input;
	wMixer mixer;
};
#endif

void load()
{
}


void init()
{
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

	// game loop here
}

void GameMain()
{
	printf("");
	game.memInfo = wGetMemoryInfo();
	game.arena = wArenaBootstrap(game.memInfo, 0);

	wWindowDef def = wDefineWindow("LUDUM DARE 42");
	
	wCreateWindow(&def, &game.window);
	wInitState(&game.window, &game.state, &game.input);
	wInitAudio(&game.window, &game.mixer, 32, game.arena);
	 
	load();

	createGraphicsDependencies();
	game.batch = createSpriteBatch(4096 * 4096, game.arena);
	
	init();

	i32 running = 1;
	while(!game.state.exitEvent) {
		wUpdate(&game.window, &game.state);
		update();
		wRender(&game.window);
	}

	wQuit();
}

int main(int argc, char** argv)
{
	GameMain();
	return 0;
}
