#include "NineSlice.h"
#include "MinMax.h"

NineSlice::NineSlice() : Texture(nullptr) {}

NineSlice::NineSlice(SDL_Texture* tex, int bX, int bY,
	int x1, int x2, int x3,
	int y1, int y2, int y3)
	: Texture(tex), baseX(bX), baseY(bY),
	X1(x1), X2(x2), X3(x3),
	Y1(y1), Y2(y2), Y3(y3) { }

NineSlice::~NineSlice() { }

void NineSlice::RenderRect(SDL_Renderer* r, int x, int y, int w, int h) const {
	SDL_Rect src, dest;
	if (w <= 0) return;
	if (h <= 0) return;
	// Compute sizes of corners
	int lw = Min(X1, w / 2);
	int th = Min(Y1, h / 2);
	int rw = Min(X3 - X2, w - lw);
	int bh = Min(Y3 - Y2, h - th);
	// Compute lengths of edges
	int remW = w - lw - rw;
	int remH = h - th - bh;
	// Compute positions of corners and edges
	int lx = x + lw;
	int ty = y + th;
	int rx = x + w - rw;
	int by = y + h - bh;
	// Draw top-left corner
	src.x = baseX;
	src.y = baseY;
	src.w = lw;
	src.h = th;
	dest.x = x;
	dest.y = y;
	dest.w = lw;
	dest.h = th;
	SDL_RenderCopy(r, Texture, &src, &dest);
	
	// Draw top edge
	src.x = baseX + X1;
	//src.y = baseY;
	src.w = X2 - X1;
	//src.h = th;
	dest.x = lx;
	//dest.y = y;
	dest.w = remW;
	//dest.h = th;
	SDL_RenderCopy(r, Texture, &src, &dest);
	
	// Draw top-right corner
	src.x = baseX + X3 - rw;
	//src.y = baseY;
	src.w = rw;
	//src.h = th;
	dest.x = rx;
	//dest.y = y;
	dest.w = rw;
	//dest.h = th;
	SDL_RenderCopy(r, Texture, &src, &dest);
	
	// Draw left edge
	src.x = baseX;
	src.y = baseY + Y1;
	src.w = lw;
	src.h = Y2 - Y1;
	dest.x = x;
	dest.y = ty;
	dest.w = lw;
	dest.h = remH;
	SDL_RenderCopy(r, Texture, &src, &dest);
	
	// Draw center
	src.x = baseX + X1;
	//src.y = baseY + Y1;
	src.w = X2 - X1;
	//src.h = Y2 - Y1;
	dest.x = lx;
	//dest.y = ty;
	dest.w = remW;
	//dest.h = remH;
	SDL_RenderCopy(r, Texture, &src, &dest);
	
	// Draw right edge
	src.x = baseX + X3 - rw;
	//src.y = baseY + Y1;
	src.w = rw;
	//src.h = Y2 - Y1;
	dest.x = rx;
	//dest.y = ty;
	dest.w = rw;
	//dest.h = remH;
	SDL_RenderCopy(r, Texture, &src, &dest);

	// Draw bottom-left corner
	src.x = baseX;
	src.y = baseY + Y3 - bh;
	src.w = lw;
	src.h = bh;
	dest.x = x;
	dest.y = by;
	dest.w = lw;
	dest.h = bh;
	SDL_RenderCopy(r, Texture, &src, &dest);

	// Draw bottom edge
	src.x = baseX + X1;
	//src.y = baseY + Y3 - bh;
	src.w = X2 - X1;
	//src.h = bh;
	dest.x = lx;
	//dest.y = by;
	dest.w = remW;
	//dest.h = bh;
	SDL_RenderCopy(r, Texture, &src, &dest);

	// Draw bottom-right corner
	src.x = baseX + X3 - rw;
	//src.y = baseY + Y3 - bh;
	src.w = rw;
	//src.h = bh;
	dest.x = rx;
	//dest.y = by;
	dest.w = rw;
	//dest.h = bh;
	SDL_RenderCopy(r, Texture, &src, &dest);
}

void NineSlice::RenderRect(SDL_Renderer* r, const SDL_Rect* rect) const {
	RenderRect(r, rect->x, rect->y, rect->w, rect->h);
}
