#include "AI.h"
#include "GameStates.h"

using namespace std;

namespace AI {
	int Heuristic(Player p, GameState s) {
		Player w = GetWinner(s);
		if (w != PLAYER_NONE) {
			return p == w ? 2 * BOARD_WIDTH : -2 * BOARD_WIDTH;
		}
		int result = 0;
		for (int i = 0; i < BOARD_WIDTH; i++) {
			if (!(s & STATE_BIT(i))) {
				result += 1;
			}
		}
		for (int i = (BOARD_HEIGHT - 1) * BOARD_WIDTH; i < BOARD_CELLS; i++) {
			if (s & STATE_BIT(i)) {
				result -= 1;
			}
		}
		if (p == PLAYER_WHITE) {
			return -result;
		} else {
			return result;
		}
	}
	int Negamax(const Move & root, int depth, int alpha, int beta, Player player, int & swapPos, bool & vertical) {
		if (depth <= 0 || GetWinner(root.result) != PLAYER_NONE) {
			swapPos = root.swapPos;
			vertical = root.vertical;
			return Heuristic(player, root.result);
		}
		vector<Move*> nextMoves;
		root.GetNextMoves(nextMoves);
		int bestScore = -INT_MAX;
		swapPos = 0;
		vertical = false;
		for (Move* mv : nextMoves) {
			int newSwapPos;
			bool newVertical;
			int newScore = -Negamax(*mv, depth - 1, -beta, -alpha, OtherPlayer(player), newSwapPos, newVertical);
			if (newScore >= bestScore) {
				bestScore = newScore;
				swapPos = mv->swapPos;
				vertical = mv->vertical;
			}
			if (alpha <= newScore) alpha = newScore;
			if (alpha >= beta) break;
		}
		for (Move* mv : nextMoves) {
			delete mv;
		}
		nextMoves.clear();
		return bestScore;
	}
	inline bool Move::IsIllegalState(GameState s) const {
		if (result == s) return true;
		if (previous) return previous->IsIllegalState(s);
		return illegalStates && illegalStates->count(s);
	}
	inline void Move::GetNextMoves(vector<Move*>& dest) const {
		// Horizontal moves
		for (int y = 0; y < BOARD_HEIGHT; y++) {
			for (int x = 0; x < BOARD_WIDTH - 1; x++) {
				Move* newMove = new Move();
				newMove->swapPos = y * BOARD_WIDTH + x;
				newMove->vertical = false;
				newMove->result = PerformSwap(result, newMove->swapPos, newMove->vertical);
				newMove->previous = this;
				if (IsIllegalState(newMove->result)) {
					delete newMove;
				} else {
					dest.push_back(newMove);
				}
			}
		}
		// Vertical moves
		for (int y = 0; y < BOARD_HEIGHT - 1; y++) {
			for (int x = 0; x < BOARD_WIDTH; x++) {
				Move* newMove = new Move();
				newMove->swapPos = y * BOARD_WIDTH + x;
				newMove->vertical = true;
				newMove->result = PerformSwap(result, newMove->swapPos, newMove->vertical);
				newMove->previous = this;
				if (IsIllegalState(newMove->result)) {
					delete newMove;
				} else {
					dest.push_back(newMove);
				}
			}
		}
	}
	void ComputeMove(
		GameState currentState, const unordered_set<GameState>& seenStates, Player player,
		int& swapPos, bool& vertical) {
		Move* rootMove = new Move();
		rootMove->illegalStates = &seenStates;
		rootMove->previous = nullptr;
		rootMove->result = currentState;
		Negamax(*rootMove, 3, -INT_MAX, INT_MAX, player, swapPos, vertical);
		delete rootMove;
	}
}