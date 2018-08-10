static inline
i32 getMouseLeft(i32 just) 
{
	return just ?
		wMouseIsJustDown(game->input, wMouseLeft) :
		wMouseIsDown(game->input, wMouseLeft);
}

static inline
i32 getMouseLeftUp(i32 just) 
{
	return just ?
		wMouseIsJustUp(game->input, wMouseLeft) :
		wMouseIsUp(game->input, wMouseLeft);
}

static inline
i32 getMouseRight(i32 just) 
{
	return just ?
		wMouseIsJustDown(game->input, wMouseRight) :
		wMouseIsDown(game->input, wMouseRight);
}

static inline
i32 getKey(i32 key, i32 just)
{
	return just ? 
		wKeyIsJustDown(game->input, key) :
		wKeyIsDown(game->input, key);
}

static inline
i32 getKeyUp(i32 key, i32 just)
{
	return just ? 
		wKeyIsJustUp(game->input, key) :
		wKeyIsUp(game->input, key);
}
