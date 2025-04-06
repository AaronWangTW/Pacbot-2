#pragma once
#include "GameState.hpp"
// #include "delta/IDelta.hpp"
#include "delta/GhostMoveDelta.hpp"
#include "delta/GhostPlanDelta.hpp"
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
  /**
   * @brief Stores the series of deltas that led to this gameState
   */
  std::stack<IDelta*> deltas;
  /**
   * @brief Stores the indexes of the previous versions in the delta stack
   */
  std::stack<int> versions;
  std::array<IGhostAgent*, 4> ghostAgents;
  void perform(IDelta* action);

  int _depthInterval;
  std::mutex _mutex;
  std::vector<std::thread> _threads;

public:
  GameState gameState;
  GameAgent(const GameAgent &other)
      : gameState(other.gameState),
        deltas(other.deltas),
        versions(other.versions),
        ghostAgents{new RedGhostAgent(),
                    new PinkGhostAgent(),
                    new CyanGhostAgent(),
                    new OrangeGhostAgent()} {};
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

  int simulateAction(int numTicks, Directions pacmanDirection);
  void collectFruit(int row, int col);
  void collectPellet(int row, int col);
  int safetyCheck();
};
