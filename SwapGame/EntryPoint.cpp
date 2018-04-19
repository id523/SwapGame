#include <iostream>
#include <memory>
#include <cmath>
#include <SDL.h>
#include <SDL_image.h>

#include "SDLError.h"
#include "SDLi.h"
#include "IMGi.h"
#include "TTFi.h"

using namespace std;

#define DELETER_CLASS(c, d) \
struct c##_Deleter { void operator()(c* r) { if (r) d(r); } }

DELETER_CLASS(SDL_Renderer, SDL_DestroyRenderer);
DELETER_CLASS(SDL_Window, SDL_DestroyWindow);
DELETER_CLASS(SDL_Surface, SDL_FreeSurface);
DELETER_CLASS(SDL_Texture, SDL_DestroyTexture);

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

	bool running = true;
	SDL_Event ev;
	while (running) {
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT: running = false; break;
			case SDL_MOUSEMOTION:
				break;
			}
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_Rect dest = Rect_Board;
		dest.x = 160;
		dest.y = 0;
		SDL_RenderCopy(renderer, tex, &Rect_Board, &dest);

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