#pragma once
#include <unordered_set>

#include "GameStates.h"

namespace AI {
	void ComputeMove(
		GameState currentState,
		const std::unordered_set<GameState>& seenStates,
		Player player,
		int& swapPos,
		bool& vertical);
}