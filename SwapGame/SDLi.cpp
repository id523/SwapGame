#include "SDLi.h"
#include <SDL.h>
#include "SDLError.h"


SDLi::SDLi()
{
	SDLErrored = false;
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		SDLErrored = true;
		throw SDLError("SDL_Init");
	}
}


SDLi::~SDLi()
{
	if (!SDLErrored)
		SDL_Quit();
}
