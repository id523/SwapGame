#include "SDLError.h"
#include <SDL.h>
#include <string>



SDLError::SDLError()
{
	std::string s;
	s += "SDL Error: ";
	s += SDL_GetError();
	msg = _strdup(s.c_str());
}

SDLError::SDLError(const char* func)
{
	std::string s;
	s += "SDL Error in ";
	s += func;
	s += ": ";
	s += SDL_GetError();
	msg = _strdup(s.c_str());
}


SDLError::~SDLError()
{
}

const char* SDLError::what() const
{
	return msg;
}
