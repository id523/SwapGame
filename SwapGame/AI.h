#pragma once
#include <unordered_set>

#include "GameStates.h"

using namespace std;

namespace AI {
	struct Move {
		int swapPos = 0;
		bool vertical = false;
		GameState result = (GameState)0;
		const Move* previous = nullptr;
		const unordered_set<GameState>* illegalStates = nullptr;
		bool IsIllegalState(GameState s) const;
		void GetNextMoves(vector<Move*>& dest) const;
	};
	int Heuristic(Player p, GameState s);
	int Negamax(const Move& root, int depth, int alpha, int beta, Player player, int& swapPos, bool& vertical);
	void ComputeMove(
		GameState currentState,
		const std::unordered_set<GameState>& seenStates,
		Player player,
		int& swapPos,
		bool& vertical);
}