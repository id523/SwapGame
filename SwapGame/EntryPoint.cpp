#include <iostream>
#include <memory>
#include <cmath>
#include <cstdint>
#include <unordered_set>

#include <SDL.h>
#include <SDL_image.h>


#include "SDLError.h"
#include "SDLi.h"
#include "IMGi.h"
#include "TTFi.h"

// "Conversion, possible loss of data"
#pragma warning(disable: 4244)

using namespace std;

#define GAME_STATE int64_t
#define STATE_BIT(n) (1LL << n)

#define BOARD_WIDTH 6
#define BOARD_HEIGHT 6
#define BOARD_CELLS (BOARD_WIDTH * BOARD_HEIGHT)
#define SQUARE_SIZE 80
#define START_X 160
#define START_Y 0
#define ANIM_SPEED 0.4f

#define DELETER_CLASS(c, d) \
struct c##_Deleter { void operator()(c* r) { if (r) d(r); } }

DELETER_CLASS(SDL_Renderer, SDL_DestroyRenderer);
DELETER_CLASS(SDL_Window, SDL_DestroyWindow);
DELETER_CLASS(SDL_Surface, SDL_FreeSurface);
DELETER_CLASS(SDL_Texture, SDL_DestroyTexture);

GAME_STATE PerformSwap(GAME_STATE s, int sp1, bool vertical) {
	int sp2 = sp1 + (vertical ? BOARD_WIDTH : 1);
	bool bit1 = !!(s & STATE_BIT(sp1));
	bool bit2 = !!(s & STATE_BIT(sp2));
	s &= ~STATE_BIT(sp1);
	s &= ~STATE_BIT(sp2);
	if (bit1) s |= STATE_BIT(sp2);
	if (bit2) s |= STATE_BIT(sp1);
	return s;
}

void GetScreenPos(int pos, int& x, int& y) {
	x = START_X + SQUARE_SIZE * (pos % BOARD_WIDTH);
	y = START_Y + SQUARE_SIZE * (pos / BOARD_WIDTH);
}

bool GetMoveFromPos(int mx, int my, int& swapPos, bool& vertical) {
	mx -= START_X;
	my -= START_Y;
	int gx = mx / SQUARE_SIZE;
	int gy = my / SQUARE_SIZE;
	if (gx < 0 || gy < 0 || gx >= BOARD_WIDTH || gy >= BOARD_HEIGHT) {
		return false;
	}
	int rx = mx - gx * SQUARE_SIZE;
	int ry = my - gy * SQUARE_SIZE;
	if (rx >= ry) {
		if (rx >= SQUARE_SIZE - ry) {
			// Quadrant Right
			if (gx < BOARD_WIDTH - 1) {
				swapPos = gy * BOARD_WIDTH + gx;
				vertical = false;
				return true;
			} else {
				return false;
			}
		} else {
			// Quadrant Up
			if (gy > 0) {
				swapPos = (gy - 1) * BOARD_WIDTH + gx;
				vertical = true;
				return true;
			} else {
				return false;
			}
		}
	} else {
		if (rx >= SQUARE_SIZE - ry) {
			// Quadrant Down
			if (gy < BOARD_HEIGHT - 1) {
				swapPos = gy * BOARD_WIDTH + gx;
				vertical = true;
				return true;
			} else {
				return false;
			}
		} else {
			// Quadrant Left
			if (gx > 0) {
				swapPos = gy * BOARD_WIDTH + gx - 1;
				vertical = false;
				return true;
			} else {
				return false;
			}
		}
	}
}

float lerp(float a, float b, float x) {
	return a * (1 - x) + b * x;
}

void SDLmain(int argc, char** argv)
{
	SDL_Window* window = SDL_CreateWindow("Swap Game",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 480, 0);
	if (!window) throw SDLError("SDL_CreateWindow");
	unique_ptr<SDL_Window, SDL_Window_Deleter> window_P(window);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RendererFlags::SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) throw SDLError("SDL_CreateRenderer");
	unique_ptr<SDL_Renderer, SDL_Renderer_Deleter> renderer_P(renderer);

	SDL_Surface* imgsurf = IMG_Load("SwapGameTex.png");
	if (!imgsurf) throw SDLError("IMG_Load");
	unique_ptr<SDL_Surface, SDL_Surface_Deleter> imgsurf_P(imgsurf);
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, imgsurf);
	if (!tex) throw SDLError("SDL_CreateTextureFromSurface");
	unique_ptr<SDL_Texture, SDL_Texture_Deleter> tex_P(tex);
	imgsurf_P.release();
	// Texture-map coordinates
	SDL_Rect Rect_Black;
	Rect_Black.x = 0;
	Rect_Black.y = 0;
	Rect_Black.w = 80;
	Rect_Black.h = 80;
	SDL_Rect Rect_White;
	Rect_White.x = 0;
	Rect_White.y = 80;
	Rect_White.w = 80;
	Rect_White.h = 80;
	SDL_Rect Rect_Board;
	Rect_Board.x = 0;
	Rect_Board.y = 160;
	Rect_Board.w = 480;
	Rect_Board.h = 480;
	SDL_Rect Rect_Highlight_1H;
	Rect_Highlight_1H.x = 80;
	Rect_Highlight_1H.y = 0;
	Rect_Highlight_1H.w = 160;
	Rect_Highlight_1H.h = 80;
	SDL_Rect Rect_Highlight_0H;
	Rect_Highlight_0H.x = 80;
	Rect_Highlight_0H.y = 80;
	Rect_Highlight_0H.w = 160;
	Rect_Highlight_0H.h = 80;
	SDL_Rect Rect_Highlight_1V;
	Rect_Highlight_1V.x = 240;
	Rect_Highlight_1V.y = 0;
	Rect_Highlight_1V.w = 80;
	Rect_Highlight_1V.h = 160;
	SDL_Rect Rect_Highlight_0V;
	Rect_Highlight_0V.x = 320;
	Rect_Highlight_0V.y = 0;
	Rect_Highlight_0V.w = 80;
	Rect_Highlight_0V.h = 160;

	bool running = true;
	SDL_Event ev;

	GAME_STATE displayState = 0000000777777LL;
	const float endSwapAnimation = (float)(SQUARE_SIZE * 2 - 1) / (SQUARE_SIZE * 2);
	float swapAnimation = 0;
	float swapAnim2 = 0;
	bool swapping = false;
	bool vertical = false;
	int swapPos = 0;
	int mouseX = 0, mouseY = 0;
	bool mouseClicked;
	GAME_STATE finalState = displayState;
	unordered_set<GAME_STATE> seenStates;
	seenStates.insert(displayState);

	while (running) {
		mouseClicked = false;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT: running = false; break;

			case SDL_MOUSEMOTION:
				mouseX = ev.motion.x;
				mouseY = ev.motion.y;
				break;

			case SDL_MOUSEBUTTONDOWN:
				if (ev.button.button == 1) {
					mouseClicked = true;
					mouseX = ev.button.x;
					mouseY = ev.button.y;
				}
				break;
			}
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_Rect dest = Rect_Board;
		dest.x = START_X;
		dest.y = START_Y;
		SDL_RenderCopy(renderer, tex, &Rect_Board, &dest);

		dest = Rect_Black;
		int x1, y1, x2, y2;
		int swapPos2 = swapPos + (vertical ? BOARD_WIDTH : 1);
		GetScreenPos(swapPos, x1, y1);
		GetScreenPos(swapPos2, x2, y2);
		for (int i = 0; i < BOARD_CELLS; i++) {
			if (i == swapPos) {
				dest.x = (int)(0.5f + lerp(x1, x2, swapAnimation));
				dest.y = (int)(0.5f + lerp(y1, y2, swapAnimation));
			} else if (i == swapPos2) {
				dest.x = (int)(0.5f + lerp(x2, x1, swapAnimation));
				dest.y = (int)(0.5f + lerp(y2, y1, swapAnimation));
			} else {
				GetScreenPos(i, dest.x, dest.y);
			}
			SDL_RenderCopy(renderer, tex,
				(displayState & STATE_BIT(i)) ? &Rect_White : &Rect_Black,
				&dest);
		}
		if (swapping) {
			swapAnim2 = lerp(swapAnim2, 1, ANIM_SPEED);
			swapAnimation = lerp(swapAnimation, swapAnim2, ANIM_SPEED);
			if (swapAnimation > endSwapAnimation) {
				swapping = false;
				swapAnimation = 0.0f;
				swapAnim2 = 0.0f;
				displayState = finalState;
			}
		} else {
			if (GetMoveFromPos(mouseX, mouseY, swapPos, vertical)) {
				dest = vertical ? Rect_Highlight_1V : Rect_Highlight_1H;
				GetScreenPos(swapPos, dest.x, dest.y);
				SDL_RenderCopy(renderer, tex,
					vertical ? &Rect_Highlight_1V : &Rect_Highlight_1H,
					&dest);
				if (mouseClicked) {
					swapping = true;
					finalState = PerformSwap(displayState, swapPos, vertical);
				}
			}
		}
		SDL_RenderPresent(renderer);
	}
}

int main(int argc, char** argv)
{
	try
	{
		SDLi sdli; // Initialize SDL
		IMGi imgi(IMG_INIT_PNG); // And SDL_image
		TTFi ttfi; // And SDL_ttf
		SDLmain(argc, argv);
		return 0;
	}
	catch (SDLError& ex)
	{
		cout << ex.what() << endl;
		getchar();
		return 1;
	}
}