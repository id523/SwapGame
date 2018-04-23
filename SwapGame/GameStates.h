#pragma once

typedef long long GameState;

#define STATE_BIT(n) ((GameState)1 << n)

#define BOARD_WIDTH 6
#define BOARD_HEIGHT 6
#define BOARD_CELLS (BOARD_WIDTH * BOARD_HEIGHT)
#define SQUARE_SIZE 80
#define START_X 0
#define START_Y 0

enum Player { PLAYER_NONE = 0, PLAYER_BLACK, PLAYER_WHITE };

const GameState TopRowMask = STATE_BIT(BOARD_WIDTH) - 1;
const GameState BottomRowMask = TopRowMask << (BOARD_WIDTH * (BOARD_HEIGHT - 1));

GameState PerformSwap(GameState s, int sp1, bool vertical);
void GetScreenPos(int pos, int& x, int& y);
bool GetMoveFromPos(int mx, int my, int& swapPos, bool& vertical);
Player GetWinner(GameState s);