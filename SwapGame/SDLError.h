#pragma once
#include <exception>

typedef const char* (*ErrorFunction)();

class SDLError :
	public std::exception
{
private:
	const char* msg;
public:
	SDLError();
	SDLError(ErrorFunction ef);
	SDLError(const char* func);
	SDLError(const char* func, ErrorFunction ef);
	~SDLError();
	const char* what() const;
};

