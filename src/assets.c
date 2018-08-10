
void assets_loadFonts()
{

	addTextureToGame("assets/sspr.png", makeWholeFileGrid("SourceSansProFont"));
	addTextureToGame("assets/inconsolata.png", makeWholeFileGrid("InconsolataFont"));
	game->monoFont = wArenaPush(game->arena, sizeof(wFontInfo));
	wLoadLocalSizedFile(game->window, 
			"assets/inconsolata.wfi", 
			(void*)game->monoFont, 
			sizeof(wFontInfo));
	game->bodyFont = wArenaPush(game->arena, sizeof(wFontInfo));
	wLoadLocalSizedFile(game->window, 
			"assets/sspr.wfi", 
			(void*)game->bodyFont, 
			sizeof(wFontInfo));
	game->titleFont = game->bodyFont;
}


void assets_initFonts()
{
	Rect2i mono = getSegment("InconsolataFont");
	game->monoFont->atlasX = mono.x;
	game->monoFont->atlasY = mono.y;
	Rect2i sspr = getSegment("SourceSansProFont");
	game->bodyFont->atlasX = sspr.x;
	game->bodyFont->atlasY = sspr.y;
}


