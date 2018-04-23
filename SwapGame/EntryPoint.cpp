#include <iostream>
#include <memory>
#include <cmath>
#include <cstdint>
#include <unordered_set>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "SDLError.h"
#include "SDLi.h"
#include "IMGi.h"
#include "TTFi.h"

#include "NineSlice.h"

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
#define ANIM_SPEED 0.35f
#define HIGHLIGHT_MARGIN 10

#define PLAYER_BLACK 1
#define PLAYER_WHITE 2

#define DELETER_CLASS(c, d) \
struct c##_Deleter { void operator()(c* r) { if (r) d(r); } }

DELETER_CLASS(SDL_Renderer, SDL_DestroyRenderer);
DELETER_CLASS(SDL_Window, SDL_DestroyWindow);
DELETER_CLASS(SDL_Surface, SDL_FreeSurface);
DELETER_CLASS(SDL_Texture, SDL_DestroyTexture);
DELETER_CLASS(TTF_Font, TTF_CloseFont);

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
const GAME_STATE TopRowMask = (1LL << BOARD_WIDTH) - 1;
const GAME_STATE BottomRowMask = TopRowMask << (BOARD_WIDTH * (BOARD_HEIGHT - 1));
int GetWinner(GAME_STATE s) {
	if ((s & TopRowMask) == 0) return PLAYER_BLACK;
	if ((s & BottomRowMask) == BottomRowMask) return PLAYER_WHITE;
	return 0;
}

float lerp(float a, float b, float x) {
	return a * (1 - x) + b * x;
}

void SDLmain(int argc, char** argv)
{
	// Boilerplate: Create window and renderer
	SDL_Window* window = SDL_CreateWindow("Swap Game",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 480, 0);
	if (!window) throw SDLError("SDL_CreateWindow");
	unique_ptr<SDL_Window, SDL_Window_Deleter> window_P(window);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RendererFlags::SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) throw SDLError("SDL_CreateRenderer");
	unique_ptr<SDL_Renderer, SDL_Renderer_Deleter> renderer_P(renderer);

	// Load texture from texture file
	SDL_Surface* imgsurf = IMG_Load("SwapGameTex.png");
	if (!imgsurf) throw SDLError("IMG_Load", IMG_GetError);
	unique_ptr<SDL_Surface, SDL_Surface_Deleter> imgsurf_P(imgsurf);
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, imgsurf);
	if (!tex) throw SDLError("SDL_CreateTextureFromSurface");
	unique_ptr<SDL_Texture, SDL_Texture_Deleter> tex_P(tex);

	// Load font
	TTF_Font* font = TTF_OpenFont("OpenSans_Bold.ttf", 12);
	if (!font) throw SDLError("TTF_OpenFont", TTF_GetError);
	unique_ptr<TTF_Font, TTF_Font_Deleter> font_P(font);

	imgsurf_P.release();
	// Texture-map coordinates
	SDL_Rect Rect_Black;
	Rect_Black.x = 0;
	Rect_Black.y = 0;
	Rect_Black.w = 80;
	Rect_Black.h = 80;
	SDL_Rect Rect_White;
	Rect_White.x = 80;
	Rect_White.y = 0;
	Rect_White.w = 80;
	Rect_White.h = 80;
	SDL_Rect Rect_Board;
	Rect_Board.x = 0;
	Rect_Board.y = 80;
	Rect_Board.w = 480;
	Rect_Board.h = 480;

	// Nine-slice textures
	unique_ptr<NineSlice> HighlightIllegal = make_unique<NineSlice>(tex, 160, 0, 15, 25, 40, 15, 25, 40);
	unique_ptr<NineSlice> HighlightLegal = make_unique<NineSlice>(tex, 200, 0, 15, 25, 40, 15, 25, 40);

	// SDL event-loop variables
	bool running = true;
	SDL_Event ev;
	// Game variables
	const GAME_STATE startState = (1LL << (BOARD_HEIGHT / 2 * BOARD_WIDTH)) - 1;
	GAME_STATE displayState = startState;
	const float endSwapAnimation = (SQUARE_SIZE * 1.5f - 1) / (SQUARE_SIZE * 1.5f);
	float swapAnimation = 0;
	float swapAnim2 = 0;
	bool swapping = false;
	bool vertical = false;
	int swapPos = 0;
	int mouseX = 0, mouseY = 0;
	bool mouseClicked;
	int currentPlayer = PLAYER_WHITE;
	int winner = 0;
	GAME_STATE finalState = displayState;
	unordered_set<GAME_STATE> seenStates;
	seenStates.insert(displayState);
	// Main loop
	while (running) {
		// Handle events
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
		// Draw board on screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_Rect dest = Rect_Board;
		dest.x = START_X;
		dest.y = START_Y;
		SDL_RenderCopy(renderer, tex, &Rect_Board, &dest);

		// Draw pieces on screen
		dest = Rect_Black;
		int x1, y1, x2, y2;
		int swapPos2 = swapPos + (vertical ? BOARD_WIDTH : 1);
		// Compute destination positions for pieces being swapped
		GetScreenPos(swapPos, x1, y1);
		GetScreenPos(swapPos2, x2, y2);
		for (int i = 0; i < BOARD_CELLS; i++) {
			// If the piece is being swapped, compute its screen position with interpolation
			if (i == swapPos) {
				dest.x = (int)(0.5f + lerp(x1, x2, swapAnimation));
				dest.y = (int)(0.5f + lerp(y1, y2, swapAnimation));
			} else if (i == swapPos2) {
				dest.x = (int)(0.5f + lerp(x2, x1, swapAnimation));
				dest.y = (int)(0.5f + lerp(y2, y1, swapAnimation));
			} else {
				// Otherwise compute it directly
				GetScreenPos(i, dest.x, dest.y);
			}
			// Draw the piece
			SDL_RenderCopy(renderer, tex,
				(displayState & STATE_BIT(i)) ? &Rect_White : &Rect_Black,
				&dest);
		}

		// Handle game mechanics
		if (swapping) {
			// If a swap is in progress, update the animation
			swapAnim2 = lerp(swapAnim2, 1, ANIM_SPEED);
			swapAnimation = lerp(swapAnimation, swapAnim2, ANIM_SPEED);
			if (swapAnimation > endSwapAnimation) {
				// If the animation is finished, reset all of the swap-display variables
				swapping = false;
				swapAnimation = 0.0f;
				swapAnim2 = 0.0f;
				// Compute the new state of the board
				displayState = finalState;
				seenStates.insert(displayState);
				// Check if either side has won
				winner = GetWinner(displayState);
				// Set next player
				currentPlayer = (currentPlayer == PLAYER_WHITE) ? PLAYER_BLACK : PLAYER_WHITE;
			}
		} else if (winner == 0) {
			// If a game is in progress,
			// compute the swap corresponding to the current position of the mouse
			if (GetMoveFromPos(mouseX, mouseY, swapPos, vertical)) {
				// Work out what state the swap will result in, and whether it is legal
				finalState = PerformSwap(displayState, swapPos, vertical);
				bool legalMove = !seenStates.count(finalState);

				// Draw the highlight in the correct colour, corresponding to the legality of the move
				NineSlice* Highlight = legalMove ? HighlightLegal.get() : HighlightIllegal.get();
				int hX, hY;
				const int shortSize = SQUARE_SIZE - 2 * HIGHLIGHT_MARGIN;
				const int longSize = 2 * (SQUARE_SIZE - HIGHLIGHT_MARGIN);
				GetScreenPos(swapPos, hX, hY);
				Highlight->RenderRect(renderer,
					hX + HIGHLIGHT_MARGIN, hY + HIGHLIGHT_MARGIN,
					vertical ? shortSize : longSize,
					vertical ? longSize : shortSize);

				if (mouseClicked && legalMove) {
					// If the user clicked the mouse, begin carrying out the move
					swapping = true;
				}
			}
		} else {
			// If there has been a winner
		}

		// Draw UI elements on the screen
		

		// Update the screen
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
#ifdef _DEBUG
		getchar();
#endif
		return 1;
	}
}