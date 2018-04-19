#pragma once
#include <exception>
class SDLError :
	public std::exception
{
private:
	const char* msg;
public:
	SDLError();
	SDLError(const char* func);
	~SDLError();
	const char* what() const;
};

