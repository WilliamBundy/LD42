/* Problem: SDL and Windows have different keys for different things
 * I need a way to consistently expose the same keycodes to the client
 *
 * Solution: Windows uses a more sane virtual key-code method (at least, I hope)
 * So, have a couple arrays that map incomping SDLK codes to VK codes, and
 * store those in the same array as normal
 */



static
void wInputUpdate(wInputState* wInput)
{
	i8* keys;
	isize i;
	keys = wInput->keys;
	for(i = 0; i < 256; ++i) {
		if(keys[i] == Button_JustDown) {
			keys[i] = Button_Down;
		} else if(keys[i] == Button_JustUp) {
			keys[i] = Button_Up;
		}
	}

	keys = wInput->mouse;
	for(i = 0; i < 16; ++i) {
		if(keys[i] == Button_JustDown) {
			keys[i] = Button_Down;
		} else if(keys[i] == Button_JustUp) {
			keys[i] = Button_Up;
		}
	}

	wInput->mouseWheel = 0;
}

i64 wKeyIsDown(wInputState* wInput, i64 keycode)
{
	return wInput->keys[keycode] >= Button_Down;
}

i64 wKeyIsUp(wInputState* wInput, i64 keycode)
{
	return wInput->keys[keycode] <= Button_Up;
}

i64 wKeyIsJustDown(wInputState* wInput, i64 keycode)
{
	return wInput->keys[keycode] == Button_JustDown;
}

i64 wKeyIsJustUp(wInputState* wInput, i64 keycode)
{
	return wInput->keys[keycode] == Button_JustUp;
}

i64 wMouseIsDown(wInputState* wInput, i64 btn)
{
	return wInput->mouse[btn] >= Button_Down;
}

i64 wMouseIsUp(wInputState* wInput, i64 btn)
{
	return wInput->mouse[btn] <= Button_Up;
}

i64 wMouseIsJustDown(wInputState* wInput, i64 btn)
{
	return wInput->mouse[btn] == Button_JustDown;
}

i64 wMouseIsJustUp(wInputState* wInput, i64 btn)
{
	return wInput->mouse[btn] == Button_JustUp;
}

f32 wGetMouseWheel(wInputState* wInput) 
{
	return wInput->mouseWheel;
}
