#include "SDLError.h"
#include <SDL.h>
#include <string>

char* my_strdup(const char* s) {
	size_t len = strlen(s);
	char* buf = (char*)malloc(len + 1);
	if (!buf) throw std::bad_alloc();
	return (char*)memcpy(buf, s, len + 1);
}

SDLError::SDLError() : SDLError((ErrorFunction)nullptr) {
}

SDLError::SDLError(ErrorFunction ef)
{
	if (!ef) ef = SDL_GetError;
	std::string s;
	s += "SDL Error: ";
	s += ef();
	msg = my_strdup(s.c_str());
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
	msg = my_strdup(s.c_str());
}

SDLError::~SDLError()
{
	free((void*)msg);
}

const char* SDLError::what() const
{
	return msg;
}
