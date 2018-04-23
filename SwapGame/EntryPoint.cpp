#include <iostream>
#include <memory>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "SDLError.h"
#include "SDLi.h"
#include "IMGi.h"
#include "TTFi.h"

#include "AI.h"
#include "GameStates.h"
#include "MinMax.h"
#include "NineSlice.h"

// "Conversion, possible loss of data"
#pragma warning(disable: 4244)

using namespace std;

#define ANIM_SPEED 0.35f
#define CPU_DELAY 10
#define HIGHLIGHT_MARGIN 10

#define MAKE_RECT(VAR, X, Y, W, H) SDL_Rect VAR; VAR.x = X; VAR.y = Y; VAR.w = W; VAR.h = H
#define MAKE_COLOR(VAR, R, G, B, A) SDL_Color VAR; VAR.r = R; VAR.g = G; VAR.b = B; VAR.a = A

#define DECLARE_DELETER(c, d) struct c##_Deleter { void operator()(c* r) { if (r) d(r); } }

DECLARE_DELETER(SDL_Renderer, SDL_DestroyRenderer);
DECLARE_DELETER(SDL_Window, SDL_DestroyWindow);
DECLARE_DELETER(SDL_Surface, SDL_FreeSurface);
DECLARE_DELETER(SDL_Texture, SDL_DestroyTexture);
DECLARE_DELETER(TTF_Font, TTF_CloseFont);

#define SAFEPTR(x) unique_ptr<x, x##_Deleter>
#define MAKESAFE(t, v) unique_ptr<t, t##_Deleter> v##_P(v)

float lerp(float a, float b, float x) {
	return a * (1 - x) + b * x;
}

SAFEPTR(SDL_Texture) StringTexture(
	SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color c, SDL_Rect* bounds = nullptr) {
	SDL_Surface* text_surf = TTF_RenderText_Blended(font, text, c);
	if (!text_surf) throw SDLError("TTF_RenderText_Blended", TTF_GetError);
	if (bounds) {
		bounds->x = 0;
		bounds->y = 0;
		bounds->w = text_surf->w;
		bounds->h = text_surf->h;
	}
	MAKESAFE(SDL_Surface, text_surf);
	SDL_Texture* text_tex = SDL_CreateTextureFromSurface(renderer, text_surf);
	if (!text_tex) throw SDLError("SDL_CreateTextureFromSurface");
	return move(SAFEPTR(SDL_Texture)(text_tex));
}

SDL_Renderer* textRenderer;
TTF_Font* textFont;
SDL_Color textColor;
unordered_map<string, SDL_Rect> stringBounds;
unordered_map<string, SAFEPTR(SDL_Texture)> stringTextures;
void RenderText(const string& s, SDL_Rect* bounds = nullptr, SDL_Texture** tex = nullptr) {
	if (!stringBounds.count(s)) {
#ifdef _DEBUG
		cout << "Rendering text: '" << s.c_str() << "'" << endl;
#endif
		SDL_Rect newBounds;
		stringTextures.emplace(s, StringTexture(textRenderer, textFont, s.c_str(), textColor, &newBounds));
		stringBounds.emplace(s, newBounds);
	}
	if (bounds) *bounds = stringBounds[s];
	if (tex) *tex = stringTextures[s].get();
}

void CenterText(SDL_Renderer* renderer, SDL_Rect r, const string& text) {
	SDL_Rect strBounds;
	SDL_Texture* strTexture;
	RenderText(text, &strBounds, &strTexture);
	int ox = (strBounds.w - r.w) / 2;
	int oy = (strBounds.h - r.h) / 2;
	if (ox <= 0) {
		r.w = strBounds.w;
		r.x -= ox;
	} else {
		strBounds.w = r.w;
		strBounds.x = ox;
	}
	if (oy <= 0) {
		r.h = strBounds.h;
		r.y -= oy;
	} else {
		strBounds.h = r.h;
		strBounds.y = oy;
	}
	SDL_RenderCopy(renderer, strTexture, &strBounds, &r);
}

void SDLmain(int argc, char** argv)
{
	// Boilerplate: Create window and renderer
	SDL_Window* window = SDL_CreateWindow("Swap Game",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 480, 0);
	if (!window) throw SDLError("SDL_CreateWindow");
	MAKESAFE(SDL_Window, window);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RendererFlags::SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) throw SDLError("SDL_CreateRenderer");
	MAKESAFE(SDL_Renderer, renderer);

	// Load texture from texture file
	SDL_Surface* imgsurf = IMG_Load("SwapGameTex.png");
	if (!imgsurf) throw SDLError("IMG_Load", IMG_GetError);
	MAKESAFE(SDL_Surface, imgsurf);
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, imgsurf);
	if (!tex) throw SDLError("SDL_CreateTextureFromSurface");
	MAKESAFE(SDL_Texture, tex);
	imgsurf_P.release();

	// Load font
	TTF_Font* font = TTF_OpenFont("OpenSans_Bold.ttf", 20);
	if (!font) throw SDLError("TTF_OpenFont", TTF_GetError);
	MAKESAFE(TTF_Font, font);

	// Declare colors
	MAKE_COLOR(Color_White, 255, 255, 255, SDL_ALPHA_OPAQUE);

	// Initialize text rendering
	textRenderer = renderer;
	textFont = font;
	textColor = Color_White;

	// Texture-map coordinates
	MAKE_RECT(Rect_Black, 0, 0, 80, 80);
	MAKE_RECT(Rect_White, 80, 0, 80, 80);
	MAKE_RECT(Rect_Board, 0, 80, 480, 480);

	// Nine-slice textures
	unique_ptr<const NineSlice> HighlightIllegal = make_unique<const NineSlice>(tex, 160, 0, 15, 25, 40, 15, 25, 40);
	unique_ptr<const NineSlice> HighlightLegal = make_unique<const NineSlice>(tex, 200, 0, 15, 25, 40, 15, 25, 40);
	unique_ptr<const NineSlice> RoundedBG = make_unique<const NineSlice>(tex, 160, 40, 8, 12, 20, 8, 12, 20);
	unique_ptr<const NineSlice> RoundedFGRidge = make_unique<const NineSlice>(tex, 180, 40, 8, 12, 20, 8, 12, 20);
	unique_ptr<const NineSlice> RoundedFGOut = make_unique<const NineSlice>(tex, 200, 40, 8, 12, 20, 8, 12, 20);
	unique_ptr<const NineSlice> RoundedFGIn = make_unique<const NineSlice>(tex, 220, 40, 8, 12, 20, 8, 12, 20);
	// SDL event-loop variables
	bool running = true;
	SDL_Event ev;
	// Game variables
	const GameState startState = (1LL << (BOARD_HEIGHT / 2 * BOARD_WIDTH)) - 1;
	GameState displayState = startState;
	const float endSwapAnimation = (SQUARE_SIZE * 1.5f - 1) / (SQUARE_SIZE * 1.5f);
	float swapAnimation = 0;
	float swapAnim2 = 0;
	bool swapping = false;
	bool vertical = false;
	int AITimer = CPU_DELAY;
	int swapPos = 0;
	int mouseX = 0, mouseY = 0;
	bool mouseClicked;
	Player currentPlayer = PLAYER_WHITE;
	Player winner = PLAYER_NONE;
	GameState finalState = displayState;
	unordered_set<GameState> seenStates;
	seenStates.insert(displayState);

	bool whiteIsAI = false;
	bool blackIsAI = true;

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
		SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
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
				AITimer = CPU_DELAY;
				// Compute the new state of the board
				displayState = finalState;
				seenStates.insert(displayState);
				// Check if either side has won
				winner = GetWinner(displayState);
				// Set next player
				currentPlayer = OtherPlayer(currentPlayer);
			}
		} else if (winner == 0) {
			// If a game is in progress, and the current player is human,
			if ((currentPlayer == PLAYER_BLACK && !blackIsAI) || (currentPlayer == PLAYER_WHITE && !whiteIsAI)) {
				// compute the swap corresponding to the current position of the mouse
				if (GetMoveFromPos(mouseX, mouseY, swapPos, vertical)) {
					// Work out what state the swap will result in, and whether it is legal
					finalState = PerformSwap(displayState, swapPos, vertical);
					bool legalMove = !seenStates.count(finalState);

					// Draw the highlight in the correct colour, corresponding to the legality of the move
					const NineSlice* Highlight = legalMove ? HighlightLegal.get() : HighlightIllegal.get();
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
				// A game is in progress and the current player is an AI
				if (AITimer > 0) {
					// The AI is waiting to move
					AITimer -= 1;
				} else {
					// The AI is ready to move
					// TODO: Set swapPos and vertical to the AI's move
					AI::ComputeMove(displayState, seenStates, currentPlayer, swapPos, vertical);
					finalState = PerformSwap(displayState, swapPos, vertical);
					swapping = true;
				}
			}
		} else {
			// If there has been a winner
		}

		// Write status text
		const char* statusText = nullptr;
		if (winner == PLAYER_BLACK) {
			statusText = "Black wins!";
		} else if (winner == PLAYER_WHITE) {
			statusText = "White wins!";
		} else if (currentPlayer == PLAYER_BLACK) {
			statusText = "Black's turn to move.";
		} else {
			statusText = "White's turn to move.";
		}
		dest.x = 480;
		dest.y = 10;
		dest.w = 320;
		dest.h = 40;
		CenterText(renderer, dest, statusText);

		// Show controls for player/CPU
		SDL_Point mouse;
		mouse.x = mouseX;
		mouse.y = mouseY;
		bool mouseHover = false;

		// Draw selector for white
		dest.x = 490;
		dest.y = 70;
		dest.w = 150;
		dest.h = 35;
		CenterText(renderer, dest, "White player:");
		dest.x = 640;
		dest.w = 120;
		mouseHover = !!SDL_PointInRect(&mouse, &dest);
		if (mouseHover) {
			SDL_SetTextureColorMod(tex, 0, 48, 128);
		} else {
			SDL_SetTextureColorMod(tex, 0, 16, 64);
		}
		RoundedBG->RenderRect(renderer, &dest);
		SDL_SetTextureColorMod(tex, 255, 255, 255);
		RoundedFGRidge->RenderRect(renderer, &dest);
		CenterText(renderer, dest, whiteIsAI ? "CPU" : "Manual");
		if (mouseClicked && mouseHover) {
			whiteIsAI = !whiteIsAI;
		}

		// Draw selector for black
		dest.x = 490;
		dest.y = 120;
		dest.w = 150;
		dest.h = 35;
		CenterText(renderer, dest, "Black player:");
		dest.x = 640;
		dest.w = 120;
		mouseHover = !!SDL_PointInRect(&mouse, &dest);
		if (mouseHover) {
			SDL_SetTextureColorMod(tex, 0, 48, 128);
		} else {
			SDL_SetTextureColorMod(tex, 0, 16, 64);
		}
		RoundedBG->RenderRect(renderer, &dest);
		SDL_SetTextureColorMod(tex, 255, 255, 255);
		RoundedFGRidge->RenderRect(renderer, &dest);
		CenterText(renderer, dest, blackIsAI ? "CPU" : "Manual");
		if (mouseClicked && mouseHover) {
			blackIsAI = !blackIsAI;
		}

		// Draw restart-game button
		if (winner) {
			dest.x = 510;
			dest.y = 200;
			dest.w = 260;
			dest.h = 40;
			mouseHover = !!SDL_PointInRect(&mouse, &dest);
			if (mouseHover) {
				SDL_SetTextureColorMod(tex, 0, 48, 128);
			} else {
				SDL_SetTextureColorMod(tex, 0, 16, 64);
			}
			RoundedBG->RenderRect(renderer, &dest);
			SDL_SetTextureColorMod(tex, 255, 255, 255);
			RoundedFGRidge->RenderRect(renderer, &dest);
			CenterText(renderer, dest, "Restart Game");
			if (mouseClicked && mouseHover) {
				winner = PLAYER_NONE;
				displayState = startState;
				currentPlayer = PLAYER_WHITE;
				seenStates.clear();
				seenStates.insert(startState);
			}
		}

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