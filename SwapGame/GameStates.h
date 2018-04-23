#pragma once

typedef long long GameState;

#define STATE_BIT(n) ((GameState)1 << n)

#define BOARD_WIDTH 6
#define BOARD_HEIGHT 6
#define BOARD_CELLS (BOARD_WIDTH * BOARD_HEIGHT)
#define SQUARE_SIZE 80
#define START_X 160
#define START_Y 0

#define PLAYER_BLACK 1
#define PLAYER_WHITE 2

GameState PerformSwap(GameState s, int sp1, bool vertical) {
	int sp2 = sp1 + (vertical ? BOARD_WIDTH : 1);
	bool bit1 = !!(s & STATE_BIT(sp1));
	bool bit2 = !!(s & STATE_BIT(sp2));
	s &= ~STATE_BIT(sp1);
	s &= ~STATE_BIT(sp2);
	if (bit1) s |= STATE_BIT(sp2);
	if (bit2) s |= STATE_BIT(sp1);
	return s;
}

void GetScreenPos(int pos, int& x, int& y) {
	x = START_X + SQUARE_SIZE * (pos % BOARD_WIDTH);
	y = START_Y + SQUARE_SIZE * (pos / BOARD_WIDTH);
}

bool GetMoveFromPos(int mx, int my, int& swapPos, bool& vertical) {
	mx -= START_X;
	my -= START_Y;
	int gx = mx / SQUARE_SIZE;
	int gy = my / SQUARE_SIZE;
	if (gx < 0 || gy < 0 || gx >= BOARD_WIDTH || gy >= BOARD_HEIGHT) {
		return false;
	}
	int rx = mx - gx * SQUARE_SIZE;
	int ry = my - gy * SQUARE_SIZE;
	if (rx >= ry) {
		if (rx >= SQUARE_SIZE - ry) {
			// Quadrant Right
			if (gx < BOARD_WIDTH - 1) {
				swapPos = gy * BOARD_WIDTH + gx;
				vertical = false;
				return true;
			} else {
				return false;
			}
		} else {
			// Quadrant Up
			if (gy > 0) {
				swapPos = (gy - 1) * BOARD_WIDTH + gx;
				vertical = true;
				return true;
			} else {
				return false;
			}
		}
	} else {
		if (rx >= SQUARE_SIZE - ry) {
			// Quadrant Down
			if (gy < BOARD_HEIGHT - 1) {
				swapPos = gy * BOARD_WIDTH + gx;
				vertical = true;
				return true;
			} else {
				return false;
			}
		} else {
			// Quadrant Left
			if (gx > 0) {
				swapPos = gy * BOARD_WIDTH + gx - 1;
				vertical = false;
				return true;
			} else {
				return false;
			}
		}
	}
}
const GameState TopRowMask = STATE_BIT(BOARD_WIDTH) - 1;
const GameState BottomRowMask = TopRowMask << (BOARD_WIDTH * (BOARD_HEIGHT - 1));

int GetWinner(GameState s) {
	if ((s & TopRowMask) == 0) return PLAYER_BLACK;
	if ((s & BottomRowMask) == BottomRowMask) return PLAYER_WHITE;
	return 0;
}