#pragma once
#include "GameState.hpp"
#include "ghost/agent/CyanGhostAgent.hpp"
#include "ghost/agent/IGhostAgent.hpp"
#include "ghost/agent/OrangeGhostAgent.hpp"
#include "ghost/agent/PinkGhostAgent.hpp"
#include "ghost/agent/RedGhostAgent.hpp"
#include <memory>
#include <stack>
#include <queue>

#include <thread>
#include <mutex>

/**
 * @class GameStateAgent
 * @brief An agent to play the game
 *
 */
class GameAgent {
private:
  std::array<IGhostAgent*, 4> ghostAgents;

  int _depthInterval;
  std::mutex _mutex;
  std::vector<std::thread> _threads;

public:
  GameState gameState;
  GameAgent(const GameAgent &other);
  GameAgent()
      : ghostAgents{new RedGhostAgent(),
                    new PinkGhostAgent(),
                    new CyanGhostAgent(),
                    new OrangeGhostAgent()} {};
  /**
   * @brief Updates the game based on the direction the pacman moves
   *
   * @param numTicks The number of tics to simulate
   * @param pacmanDirection The action taken by the pacman in that time
   */
  void step(int numTicks, Directions pacmanDirection);

  /**
   * @brief Copies the game state into the internal game state
   *
   * @param gameState The game state to copy
   */
  void update(const GameState &gameState);
  GameAgent& operator=(const GameAgent& other);
};
