#include "include/asNuklearImplimentation.h"

#if ASTRENGINE_NUKLEAR
#define NK_IMPLEMENTATION
#include "nuklear/nuklear.h"

#include "SDL_events.h"

#define AS_NK_MAXMEMORY 500000
struct nk_context nkContext;
void* nkMemory;

struct nk_font_atlas fontAtlas;
asTextureHandle_t fontTexture;

ASEXPORT struct nk_context* asGetNuklearContextPtr()
{
	return &nkContext;
}

ASEXPORT void asInitNk()
{
	/*Font*/
	const void* img;
	int w, h;
	nk_font_atlas_init_default(&fontAtlas);
	img = nk_font_atlas_bake(&fontAtlas, &w, &h, NK_FONT_ATLAS_RGBA32);
	/*Create texture*/
	{
		asTextureDesc_t desc = asTextureDesc_Init();
		desc.usageFlags = AS_TEXTUREUSAGE_SAMPLED;
		desc.cpuAccess = AS_GPURESOURCEACCESS_DEVICE;
		desc.format = AS_COLORFORMAT_RGBA8_UNORM;
		desc.width = w;
		desc.height = h;
		desc.initialContentsBufferSize = w * h * 4;
		desc.initialContentsRegionCount = 1;
		asTextureContentRegion_t region = (asTextureContentRegion_t) { 0 };
		region.bufferStart = 0;
		region.extent[0] = desc.width;
		region.extent[1] = desc.height;
		region.extent[2] = 1;
		region.layerCount = 1;
		region.layer = 0;
		region.mipLevel = 0;
		desc.pInitialContentsRegions = &region;
		desc.pInitialContentsBuffer = img;
		desc.pDebugLabel = "NuklearFont";
		fontTexture = asCreateTexture(&desc);
	}
	/*Create material*/
	{
		/*Todo...*/
	}
	nk_font_atlas_end(&fontAtlas, nk_handle_id(asHandle_toInt(fontTexture)) /*Replace with Material Handle*/, 0);

	/*Context*/
	nkMemory = asMalloc(AS_NK_MAXMEMORY);
	memset(nkMemory, 0, AS_NK_MAXMEMORY);
	nk_init_fixed(&nkContext, nkMemory, AS_NK_MAXMEMORY, &fontAtlas.default_font->handle);
}

ASEXPORT void asNkDraw()
{
#if ASTRENGINE_VK
#endif
	nk_clear(&nkContext);
}

ASEXPORT void asNkPushEvent(void *pEvent)
{
	/*Yes... I lifted this straight off of the demos... Not recreating this mess*/
	SDL_Event *evt = (SDL_Event*)pEvent;
	struct nk_context* ctx = &nkContext;
	if (ctx->input.mouse.grab) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
		ctx->input.mouse.grab = 0;
	}
	else if (ctx->input.mouse.ungrab) {
		int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_WarpMouseInWindow(asGetMainWindowPtr(), x, y);
		ctx->input.mouse.ungrab = 0;
	}
	if (evt->type == SDL_KEYUP || evt->type == SDL_KEYDOWN) {
		/* key events */
		int down = evt->type == SDL_KEYDOWN;
		const Uint8* state = SDL_GetKeyboardState(0);
		SDL_Keycode sym = evt->key.keysym.sym;
		if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
			nk_input_key(ctx, NK_KEY_SHIFT, down);
		else if (sym == SDLK_DELETE)
			nk_input_key(ctx, NK_KEY_DEL, down);
		else if (sym == SDLK_RETURN)
			nk_input_key(ctx, NK_KEY_ENTER, down);
		else if (sym == SDLK_TAB)
			nk_input_key(ctx, NK_KEY_TAB, down);
		else if (sym == SDLK_BACKSPACE)
			nk_input_key(ctx, NK_KEY_BACKSPACE, down);
		else if (sym == SDLK_HOME) {
			nk_input_key(ctx, NK_KEY_TEXT_START, down);
			nk_input_key(ctx, NK_KEY_SCROLL_START, down);
		}
		else if (sym == SDLK_END) {
			nk_input_key(ctx, NK_KEY_TEXT_END, down);
			nk_input_key(ctx, NK_KEY_SCROLL_END, down);
		}
		else if (sym == SDLK_PAGEDOWN) {
			nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
		}
		else if (sym == SDLK_PAGEUP) {
			nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
		}
		else if (sym == SDLK_z)
			nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_r)
			nk_input_key(ctx, NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_c)
			nk_input_key(ctx, NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_v)
			nk_input_key(ctx, NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_x)
			nk_input_key(ctx, NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_b)
			nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_e)
			nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_UP)
			nk_input_key(ctx, NK_KEY_UP, down);
		else if (sym == SDLK_DOWN)
			nk_input_key(ctx, NK_KEY_DOWN, down);
		else if (sym == SDLK_LEFT) {
			if (state[SDL_SCANCODE_LCTRL])
				nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else nk_input_key(ctx, NK_KEY_LEFT, down);
		}
		else if (sym == SDLK_RIGHT) {
			if (state[SDL_SCANCODE_LCTRL])
				nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else nk_input_key(ctx, NK_KEY_RIGHT, down);
		}
	}
	else if (evt->type == SDL_MOUSEBUTTONDOWN || evt->type == SDL_MOUSEBUTTONUP) {
		/* mouse button */
		int down = evt->type == SDL_MOUSEBUTTONDOWN;
		const int x = evt->button.x, y = evt->button.y;
		if (evt->button.button == SDL_BUTTON_LEFT) {
			if (evt->button.clicks > 1)
				nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down);
			nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
		}
		else if (evt->button.button == SDL_BUTTON_MIDDLE)
			nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
		else if (evt->button.button == SDL_BUTTON_RIGHT)
			nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
		return;
	}
	else if (evt->type == SDL_MOUSEMOTION) {
		/* mouse motion */
		if (ctx->input.mouse.grabbed) {
			int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
			nk_input_motion(ctx, x + evt->motion.xrel, y + evt->motion.yrel);
		}
		else nk_input_motion(ctx, evt->motion.x, evt->motion.y);
		return;
	}
	else if (evt->type == SDL_TEXTINPUT) {
		/* text input */
		nk_glyph glyph;
		memcpy(glyph, evt->text.text, NK_UTF_SIZE);
		asDebugLog(glyph);
		nk_input_glyph(ctx, glyph);
		return;
	}
	else if (evt->type == SDL_MOUSEWHEEL) {
		/* mouse wheel */
		nk_input_scroll(ctx, nk_vec2((float)evt->wheel.x, (float)evt->wheel.y));
		return;
	}
}

ASEXPORT void asShutdownNk()
{
	asReleaseTexture(fontTexture);
	nk_font_atlas_clear(&fontAtlas);
	asFree(nkMemory);
}
#endif