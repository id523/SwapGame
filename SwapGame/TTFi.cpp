#include "TTFi.h"
#include <SDL_ttf.h>
#include "SDLError.h"


TTFi::TTFi()
{
	TTFErrored = false;
	if (TTF_Init() != 0)
	{
		TTFErrored = true;
		throw SDLError("TTF_Init");
	}
}

TTFi::~TTFi()
{
	if (!TTFErrored)
		TTF_Quit();
}
