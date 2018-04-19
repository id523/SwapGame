#include "IMGi.h"
#include <SDL_image.h>
#include "SDLError.h"


IMGi::IMGi(int flags)
{
	IMGErrored = false;
	if (IMG_Init(flags) == 0)
	{
		IMGErrored = true;
		throw SDLError("IMG_Init");
	}
}

IMGi::IMGi()
{
	IMGi(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP);
}

IMGi::~IMGi()
{
	if (!IMGErrored)
		IMG_Quit();
}
