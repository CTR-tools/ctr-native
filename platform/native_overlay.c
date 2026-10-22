// MultiTrack: F12 debug overlay (see include/platform/native_overlay.h). The
// text is rasterised on the CPU with SDL's public domain 8x8 debug font and
// drawn as one quad by NativeRenderer_DrawOverlay.

#include "platform/native_overlay.h"
#include "platform/native_rtx.h"

#include <SDL3/SDL.h>

#include "../externals/SDL/src/render/SDL_render_debug_font.h"

#include <stdio.h>
#include <string.h>

extern int g_dbg_wireframeMode;
extern int g_dbg_texturelessMode;
extern int g_cfg_bilinearFiltering;

#define OVERLAY_COLS 64
#define OVERLAY_ROWS 40
#define OVERLAY_PAD  4

static struct
{
	int open;
	u64 lastFrameNs;
	float frameMs; // smoothed time between presented frames
	u8 pixels[(OVERLAY_ROWS * 8 + 2 * OVERLAY_PAD) * (OVERLAY_COLS * 8 + 2 * OVERLAY_PAD) * 4];
} s_overlay;

int Overlay_IsOpen(void)
{
	return s_overlay.open;
}

void Overlay_Toggle(void)
{
	s_overlay.open ^= 1;
}

int Overlay_HandleKey(int scancode, int down, int shift)
{
	if (!s_overlay.open || (scancode < SDL_SCANCODE_1) || (scancode > SDL_SCANCODE_8))
	{
		return 0;
	}
	if (down)
	{
		Rtx_OverlayKey(scancode - SDL_SCANCODE_1 + 1, shift);
	}
	return 1;
}

static const char *Overlay_OnOff(int v)
{
	return v ? "on" : "off";
}

static void Overlay_BuildText(char *buf, int size)
{
	SDL_Window *window = SDL_GL_GetCurrentWindow();
	int fullscreen = window && ((SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0);
	float fps = (s_overlay.frameMs > 0.0f) ? 1000.0f / s_overlay.frameMs : 0.0f;
	int n = snprintf(buf, size,
	                 "CTR MultiTrack debug          F12 close   %5.1f fps\n"
	                 "F1  wireframe                 %s\n"
	                 "F2  textureless               %s\n"
	                 "F3  GL bilinear filtering     %s\n"
	                 "F4  keyboard -> next player\n"
	                 "F5  RT lighting               %s\n"
	                 "F6  RT debug view             %s\n"
	                 "F7  ray tracing               %s\n"
	                 "F8  load state     Shift+F5 save state\n"
	                 "F9  replay start   F10 replay stop\n"
	                 "F11 fullscreen                %s\n"
	                 "Shift+F6 gamepad -> next player\n"
	                 "Shift+F7 save VRAM.TGA   PrtSc SCREENSHOT.BMP\n",
	                 fps, Overlay_OnOff(g_dbg_wireframeMode), Overlay_OnOff(g_dbg_texturelessMode), Overlay_OnOff(g_cfg_bilinearFiltering),
	                 Rtx_KeyStatus(SDL_SCANCODE_F5), Rtx_KeyStatus(SDL_SCANCODE_F6), Rtx_KeyStatus(SDL_SCANCODE_F7), Overlay_OnOff(fullscreen));
	if ((n > 0) && (n < size))
	{
		Rtx_OverlayText(buf + n, size - n);
	}
}

const u8 *Overlay_Rasterize(int *w, int *h)
{
	u64 now = SDL_GetTicksNS();
	if (s_overlay.lastFrameNs != 0)
	{
		float ms = (float)(now - s_overlay.lastFrameNs) / 1e6f;
		s_overlay.frameMs = (s_overlay.frameMs > 0.0f) ? 0.95f * s_overlay.frameMs + 0.05f * ms : ms;
	}
	s_overlay.lastFrameNs = now;
	if (!s_overlay.open)
	{
		return NULL;
	}

	char text[OVERLAY_ROWS * (OVERLAY_COLS + 1) + 1];
	Overlay_BuildText(text, sizeof(text));

	// size to the text: columns of the longest line, rows used
	int cols = 0, rows = 0, col = 0;
	for (const char *c = text; *c; c++)
	{
		if (*c == '\n')
		{
			rows++;
			col = 0;
			continue;
		}
		col++;
		cols = (col > cols) ? col : cols;
	}
	rows += (col > 0);
	cols = (cols > OVERLAY_COLS) ? OVERLAY_COLS : cols;
	rows = (rows > OVERLAY_ROWS) ? OVERLAY_ROWS : rows;
	const int pw = cols * 8 + 2 * OVERLAY_PAD, ph = rows * 8 + 2 * OVERLAY_PAD;

	// translucent dark panel, white text with a drop shadow
	u32 *px = (u32 *)s_overlay.pixels;
	for (int i = 0; i < pw * ph; i++)
	{
		px[i] = 0xb4000000u;
	}
	int x = 0, y = 0;
	for (const char *c = text; *c && (y < rows); c++)
	{
		if (*c == '\n')
		{
			x = 0;
			y++;
			continue;
		}
		unsigned char ch = (unsigned char)*c;
		if ((x < cols) && (ch > 32) && (ch < 127))
		{
			const Uint8 *glyph = SDL_RenderDebugTextFontData + (ch - 33) * 8;
			for (int gy = 0; gy < 8; gy++)
			{
				for (int gx = 0; gx < 8; gx++)
				{
					if ((glyph[gy] & (1 << gx)) == 0)
					{
						continue;
					}
					int ox = OVERLAY_PAD + x * 8 + gx, oy = OVERLAY_PAD + y * 8 + gy;
					px[oy * pw + ox] = 0xffffffffu;
					if ((ox + 1 < pw) && (oy + 1 < ph) && (px[(oy + 1) * pw + ox + 1] != 0xffffffffu))
					{
						px[(oy + 1) * pw + ox + 1] = 0xff000000u;
					}
				}
			}
		}
		x++;
	}
	*w = pw;
	*h = ph;
	return s_overlay.pixels;
}
