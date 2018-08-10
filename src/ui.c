#ifndef WirmphtEnabled
typedef enum {
	Gui_None,
	Gui_IncrementX = 1<<1, 
	Gui_IncrementY = 1<<2, 
	Gui_LabelBackground = 1<<3,
	Gui_FixedSize = 1<<4,
	Gui_Flow = 1<<5
} GuiFlags;

struct Gui
{
	SpriteBatch* batch;

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
#endif


void guiInit(Gui* gui, SpriteBatch* batch)
{
	gui->batch = batch;
	gui->anchor = Anchor_TopLeft;
	gui->padding = v2(8, 8);
	gui->borderColor = 0x666666FF;
	gui->bgColor = 0x444444FF;
	gui->hotColor = 0xAAAAAAFF;
	gui->activeColor = 0x222222FF;
	gui->textColor = 0xFFFFFFFF;
	gui->selectedColor = 0xFFFF00FF;
	gui->textScale = 1.0f;
}


void guiStart(Gui* gui)
{
	if(gui->isActive) {
		wLogError(0, "Warning: calling guiStart on an active gui:"
				" did you forget guiEnd?\n");
	}

	gui->isActive = 1;
	gui->mouse = v2(game->state->mouseX, game->state->mouseY);
	v2ScaleP(&gui->mouse, 1.0 / gui->batch->scale);
	v2AddP(&gui->mouse, gui->batch->pos);

	gui->mouseState = game->input->mouse[wMouseLeft];
}

void guiEnd(Gui* gui)
{
	if(!gui->isActive) {
		wLogError(0, "Warning: calling guiEnd on an inactive gui:"
				" call guiStart first!\n");
	}
	gui->isActive = 0;
	drawSprites(gui->batch);
}

AABB guiGetAABB(Gui* gui, Vec2 pos, Vec2 size)
{
	Vec2 anchor = v2(2*AnchorX[gui->anchor], 2*AnchorY[gui->anchor]);
	AABB box = aabbSafe(pos, v2Add(pos, v2Mul(size, anchor)));
	return box;
}

i32 guiCheckAABB(Gui* gui, AABB box)
{
	return aabbContains(box, gui->mouse);
}

// TODO(Will) add different anchor support: right now, this only works with 
// 			  Anchor_TopLeft
void guiLayout(Gui* gui, Vec2* pos, Vec2* size)
{
	//AABB 
}

void guiSetNextPos(Gui* gui, Vec2 size)
{
	if(gui->flags & Gui_IncrementX) {
		gui->pos.x += (size.x + gui->padding.x) * AnchorX[gui->anchor] * 2;
		if(gui->anchor == Anchor_TopLeft && gui->flags & Gui_Flow) {
			if(gui->pos.x - gui->startPos.x > gui->maxSize.x) {
				
			}
		}
	}

	if(gui->flags & Gui_IncrementY) {
		gui->pos.y += (size.y + gui->padding.y) * AnchorY[gui->anchor] * 2;
	}
}

void guiGetSizeFromText(Gui* gui, SpriteList* textSprites, 
		Vec2* sizeOut, Vec2* textSizeOut)
{
	Vec2 textSize = v2Sub(textSprites->box.max, textSprites->box.min);
	Vec2 size = v2(0, 0);

	if(gui->flags & Gui_FixedSize) {
		size = gui->size;
	} else {
		size = v2AddScaled(textSize, gui->padding, 2); 
	}

	if(sizeOut) {
		*sizeOut = size;
	}

	if(textSizeOut) {
		*textSizeOut = textSize;
	}

}

void guiWidgetWarn(Gui* gui)
{
	if(!gui->isActive) {
		wLogError(0, "Warning: creating a widget on inactive gui\n");
	}
}

void guiDrawBorder(Gui* gui, Vec2 pos, Vec2 size)
{
	addSprite(gui->batch, makePrimitive(
				pos, v2(size.x, 2), 
				gui->borderColor, Anchor_TopLeft));
	addSprite(gui->batch, makePrimitive(
				v2AddXY(pos, 0, size.y), v2(size.x, 2), 
				gui->borderColor, Anchor_BottomLeft));
	addSprite(gui->batch, makePrimitive(
				pos, v2(2, size.y), 
				gui->borderColor, Anchor_TopLeft));
	addSprite(gui->batch, makePrimitive(
				v2AddXY(pos, size.x, 0), v2(2, size.y), 
				gui->borderColor, Anchor_TopRight));
}

i32 guiBtn(Gui* gui, string text)
{
	guiWidgetWarn(gui);

	SpriteList textSprites = drawUiText(gui,
			v2(0, 0), 
			text,
			gui->textColor);
	Vec2 size, textSize;
	guiGetSizeFromText(gui, &textSprites, &size, &textSize);

	i32 state = 0;
	u32 color = gui->bgColor;
	AABB box = guiGetAABB(gui, gui->pos, size);
	if(guiCheckAABB(gui, box)) {
		if(gui->mouseState == Button_Down) {
			state = 2;
			color = gui->activeColor;
		} else if(gui->mouseState == Button_Up) {
			state = 1;
			color = gui->hotColor;
		} else if(gui->mouseState == Button_JustUp){
			state = 3;
			color = gui->hotColor;
		}
	}

	if(gui->flags & Gui_FixedSize) {
		//Vec2 textOffset = v2AddScaled(size, textSize, 0.5);
		Vec2 textOffset = v2Sub(size, textSize);
		v2ScaleP(&textOffset, 0.5);
		for(isize i = 0; i < textSprites.count; ++i) {
			Sprite* s = gui->batch->sprites + i + textSprites.start;
			v2AddP(&s->pos, v2AddScaled(box.min, textOffset, 1));
		}
	} else {
		for(isize i = 0; i < textSprites.count; ++i) {
			Sprite* s = gui->batch->sprites + i + textSprites.start;
			v2AddP(&s->pos, v2AddScaled(box.min, gui->padding, 1));
		}
	}

	Sprite bgs = makePrimitive(box.min, size, color, Anchor_TopLeft);
	Sprite* bg = getSprite(gui->batch);
	*bg = gui->batch->sprites[textSprites.start];
	gui->batch->sprites[textSprites.start] = bgs;
	guiDrawBorder(gui, box.min, size);

	guiSetNextPos(gui, size);

	return state == 3;
}

i32 guiIcoSelector(Gui* gui, Rect2i icon, i32 selected, f32 scale)
{
	guiWidgetWarn(gui);
	Vec2 size;
	Vec2 iconSize = v2(icon.w, icon.h);
	if(gui->flags & Gui_FixedSize) {
		size = gui->size;
	} else {
		size = iconSize; 
		size.x *= scale;
		size.y *= scale;
	}
	i32 state = 0;
	AABB box = guiGetAABB(gui, gui->pos, size);
	if(guiCheckAABB(gui, box)) {
		if(gui->mouseState == Button_Down) {
			state = 2;
		} else if(gui->mouseState == Button_Up) {
			state = 1;
		} else if(gui->mouseState == Button_JustUp){
			state = 3;
		}
	}

	Vec2 iconPos = v2(0, 0);
	
	Sprite s = makeSprite(box.min, size, icon);
	s.flags |= gui->anchor | Sprite_NoAA;
	addSprite(gui->batch, s);
	if(selected) {
		drawBorderedBoxThickness(gui->batch, box.min, size, 0, gui->selectedColor, 1);
	}
	guiSetNextPos(gui, size);

	return state == 3;
}

i32 guiIcoBtn(Gui* gui, Rect2i icon)
{
	guiWidgetWarn(gui);
	Vec2 size;
	Vec2 iconSize = v2(icon.w, icon.h);
	if(gui->flags & Gui_FixedSize) {
		size = gui->size;
	} else {
		size = v2AddScaled(iconSize, gui->padding, 2); 
	}
	i32 state = 0;
	u32 color = gui->bgColor;
	AABB box = guiGetAABB(gui, gui->pos, size);
	if(guiCheckAABB(gui, box)) {
		if(gui->mouseState == Button_Down) {
			state = 2;
			color = gui->activeColor;
		} else if(gui->mouseState == Button_Up) {
			state = 1;
			color = gui->hotColor;
		} else if(gui->mouseState == Button_JustUp){
			state = 3;
			color = gui->hotColor;
		}
	}

	Vec2 iconPos = v2(0, 0);
	if(gui->flags & Gui_FixedSize) {
		f32 aspect = iconSize.x / iconSize.y;
		if(gui->size.x < gui->size.y) {
			iconSize.x = gui->size.x - gui->padding.x * 2;
			iconSize.y = iconSize.x / aspect;
		} else {
			iconSize.y = gui->size.y - gui->padding.y * 2;
			iconSize.x = iconSize.y * aspect;
		}
		Vec2 offset = v2Scale(v2Sub(gui->size, iconSize), 0.5);
		iconPos = v2Add(box.min, offset);
	} else {
		iconPos = v2Add(box.min, gui->padding);
	}
	
	Sprite bgs = makePrimitive(box.min, size, color, Anchor_TopLeft | Sprite_NoAA);
	addSprite(gui->batch, bgs);
	Sprite s = makeSprite(iconPos, iconSize, icon);
	s.flags |= Anchor_TopLeft;
	addSprite(gui->batch, s);
	guiDrawBorder(gui, box.min, size);
	guiSetNextPos(gui, size);

	return state == 3;
}

void guiLabel(Gui* gui, string text) 
{
	//guiLabelEx(gui, text, game->bodyFont, 16 * gui->textScale, 0.95, 0);
}

void guiLabelEx(Gui* gui, string text, wFontInfo* font, 
		f32 pointSize, f32 tracking, i32 isMono) 
{

	SpriteList textSprites = renderText(gui->batch,
			font, 
			0, 0,
			text, -1,
			pointSize, 0, 
			gui->textColor, tracking, isMono); 
	Vec2 size, textSize;
	guiGetSizeFromText(gui, &textSprites, &size, &textSize);

	i32 state = 0;
	AABB box = guiGetAABB(gui, gui->pos, size);

	if(gui->flags & Gui_FixedSize) {
		//Vec2 textOffset = v2AddScaled(size, textSize, 0.5);
		Vec2 textOffset = v2Sub(size, textSize);
		v2ScaleP(&textOffset, 0.5);
		for(isize i = 0; i < textSprites.count; ++i) {
			Sprite* s = gui->batch->sprites + i + textSprites.start;
			v2AddP(&s->pos, v2AddScaled(box.min, textOffset, 1));
		}
	} else {
		Vec2 padding = v2(0, 0);
		if(gui->flags & Gui_LabelBackground) {
			padding = gui->padding;
		}  else {
			v2AddScaledP(&size, gui->padding, -2);
		}
		for(isize i = 0; i < textSprites.count; ++i) {
			Sprite* s = gui->batch->sprites + i + textSprites.start;
			v2AddP(&s->pos, v2AddScaled(box.min, padding, 1));
		}
	}

	if(gui->flags & Gui_LabelBackground) {
		Sprite bgs = makePrimitive(box.min, size, gui->bgColor, Anchor_TopLeft);
		Sprite* bg = getSprite(gui->batch);
		*bg = gui->batch->sprites[textSprites.start];
		gui->batch->sprites[textSprites.start] = bgs;
		guiDrawBorder(gui, box.min, size);
	}

	guiSetNextPos(gui, size);
}


//void guiLabelXY(Gui* gui, string text, Vec2 pos, Vec2 size);
//i32 guiBtnXY(Gui* gui, string text, Vec2 pos, Vec2 size);
//i32 guiIcoBtnXY(Gui* gui, Rect2i icon, Vec2 pos, Vec2 size);
//icoBtnState?
