#pragma once
#include "GameState.hpp"
#include "delta/IDelta.hpp"
#include "ghost/agent/CyanGhostAgent.hpp"
#include "ghost/agent/IGhostAgent.hpp"
#include "ghost/agent/OrangeGhostAgent.hpp"
#include "ghost/agent/PinkGhostAgent.hpp"
#include "ghost/agent/RedGhostAgent.hpp"
#include <memory>
#include <stack>
#include <queue>

/**
 * @class GameStateAgent
 * @brief An agent to play the game
 *
 */
class GameAgent {
private:
  /**
   * @brief Stores the series of deltas that led to this gameState
   */
  std::stack<std::unique_ptr<IDelta>> deltas;
  /**
   * @brief Stores the indexes of the previous versions in the delta stack
   */
  std::stack<int> versions;
  std::array<std::unique_ptr<IGhostAgent>, 4> ghostAgents;
  void perform(std::unique_ptr<IDelta> &&action);

public:
  GameState gameState;
  GameAgent(const GameAgent &other);
  GameAgent()
      : ghostAgents{std::make_unique<RedGhostAgent>(),
                    std::make_unique<PinkGhostAgent>(),
                    std::make_unique<CyanGhostAgent>(),
                    std::make_unique<OrangeGhostAgent>()} {};
  /**
   * @brief Updates the game based on the direction the pacman moves
   *
   * @param numTicks The number of tics to simulate
   * @param pacmanDirection The action taken by the pacman in that time
   */
  void step(int numTicks, Directions pacmanDirection);
  /**
   * @brief Reverts to the previous game state
   */
  void undo();

  /**
   * @brief Copies the game state into the internal game state
   *
   * @param gameState The game state to copy
   */
  void update(const GameState &gameState);
  GameAgent& operator=(const GameAgent& other);
};
