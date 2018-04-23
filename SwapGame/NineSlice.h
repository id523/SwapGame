#pragma once
#include <SDL.h>

class NineSlice {
public:
	SDL_Texture* Texture;
	int baseX = 0, baseY = 0;
	int X1 = 0, X2 = 0, X3 = 0;
	int Y1 = 0, Y2 = 0, Y3 = 0;
	NineSlice();
	NineSlice(SDL_Texture* tex, int bX, int bY, int x1, int x2, int x3, int y1, int y2, int y3);
	~NineSlice();
	void RenderRect(SDL_Renderer* r, int x, int y, int w, int h) const;
	void RenderRect(SDL_Renderer* r, const SDL_Rect* rect) const;
};