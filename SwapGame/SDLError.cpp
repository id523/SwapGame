#include "SDLError.h"
#include <SDL.h>
#include <string>

SDLError::SDLError() : SDLError((ErrorFunction)nullptr) {
}

SDLError::SDLError(ErrorFunction ef)
{
	if (!ef) ef = SDL_GetError;
	std::string s;
	s += "SDL Error: ";
	s += ef();
	msg = _strdup(s.c_str());
}

SDLError::SDLError(const char * func) : SDLError(func, nullptr) {
}

SDLError::SDLError(const char* func, ErrorFunction ef)
{
	if (!ef) ef = SDL_GetError;
	std::string s;
	s += "SDL Error in ";
	s += func;
	s += ": ";
	s += ef();
	msg = _strdup(s.c_str());
}

SDLError::~SDLError()
{
	free((void*)msg);
}

const char* SDLError::what() const
{
	return msg;
}
