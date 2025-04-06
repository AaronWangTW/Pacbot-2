#include "DecisionModule.hpp"

#include <algorithm>
#include <future>
#include <vector>

#include "walls.hpp"

std::vector<std::pair<int, int>> FOOD_POSITIONS;  // row,col

int ACTION_TICK = 12;

bool pelletAt(std::array<uint32_t, 31>& pelletArr, int row, int col) {
  return (pelletArr[row] >> col) & 1;
}

int numberOfSetBits(uint32_t i) {
  // https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer
  i = i - ((i >> 1) & 0x55555555);                 // add pairs of bits
  i = (i & 0x33333333) + ((i >> 2) & 0x33333333);  // quads
  i = (i + (i >> 4)) & 0x0F0F0F0F;                 // groups of 8
  i *= 0x01010101;                                 // horizontal sum of bytes
  return i >> 24;  // return just that top byte (after truncating to 32-bit even
                   // when int is wider than uint32_t)
}

int numPellets(std::array<uint32_t, 31> pelletArr) {
  int sum = 0;
  for (unsigned int i = 0; i < 31; i++) {
    sum += numberOfSetBits(pelletArr[i]);
  }
  return sum;
}

DecisionModule::DecisionModule(GameAgent agent, int depthLimit) {
  this->_agent = agent;
  this->_depthLimit = depthLimit;
}
DecisionModule::DecisionModule() {
  this->_agent = GameAgent();
  this->_depthLimit = 6;
}

void DecisionModule::overrideAgent(GameAgent agent) { this->_agent = agent; }

int DecisionModule::evaluateState(GameAgent& currAgent) {
  // Calculate distance to the nearest food
  int min_food_distance =
      10000;  // larger than 31^2 + 28^2, largest squared distance
  for (std::pair<int, int>& pos : FOOD_POSITIONS) {
    if (pelletAt(_agent.gameState.pelletArr, pos.first, pos.second)) {
      int rowDist = pos.first - _agent.gameState.pacmanLoc.getRow();
      int colDist = pos.second - _agent.gameState.pacmanLoc.getCol();
      int dist = rowDist * rowDist + colDist * colDist;
      if (dist < min_food_distance) {
        min_food_distance = dist;
      }
    }
  }
  // Calculate the distance to nearest ghost
  int minGhostDist = 10000;
  int minDistGhostSteps = 0;
  Location& pacLoc = _agent.gameState.pacmanLoc;
  for (int i = 0; i < 4; i++) {
    Location& loc = _agent.gameState.ghosts[i].location;
    int gDist = abs(pacLoc.getCol() - loc.getCol()) +
                abs(pacLoc.getRow() - loc.getRow());
    if (gDist < minGhostDist) {
      minGhostDist = gDist;
      minDistGhostSteps = _agent.gameState.ghosts[i].getFreightenedSteps();
    }
  }
  // avoid certain death
  if (minGhostDist <= 1 && minDistGhostSteps == 0) {
    return -999999;
  }
  // eat a scared ghost
  if (minGhostDist <= 1 && minDistGhostSteps > 0) {
    return 999999;
  }
  return _agent.gameState.currScore * 5 - min_food_distance;
}

std::queue<GameAgent> DecisionModule::bfsSearch(int bfsDepth, GameAgent& currAgent) {
  std::queue<std::pair<GameAgent, int>> bfsQueue;
  std::queue<GameAgent> resultQueue;
  bfsQueue.push(std::make_pair(currAgent, 0));

  while (!bfsQueue.empty()) {
    GameAgent agent;
    int curr_depth;
    std::tie(agent, curr_depth) = bfsQueue.front();
    bfsQueue.pop();

    if (agent.gameState.currLives == 0 || curr_depth == _depthLimit ||
        numPellets(agent.gameState.pelletArr) == 0) {
      resultQueue.push(agent);
      continue;
    }

    if (curr_depth >= bfsDepth) {
      resultQueue.push(agent);
      continue;
    }

    Location p_loc = agent.gameState.pacmanLoc;
    std::vector<std::pair<int, int>> targets = {
        {p_loc.getRow(), p_loc.getCol()},
        {p_loc.getRow() + 1, p_loc.getCol()},
        {p_loc.getRow() - 1, p_loc.getCol()},
        {p_loc.getRow(), p_loc.getCol() + 1},
        {p_loc.getRow(), p_loc.getCol() - 1}};
    std::vector<Directions> dir = {NONE, DOWN, UP, RIGHT, LEFT};

    std::vector<std::future<void>> futures;

    for (unsigned int i = 0; i < targets.size(); ++i) {
      if (wallAt(targets[i].first, targets[i].second)) {
        continue;
      }

      futures.push_back(std::async(std::launch::async, [&, i]() {
        GameAgent new_agent = agent;
        int prev_lives = new_agent.gameState.currLives;
        new_agent.step(ACTION_TICK, dir[i]);

        if (new_agent.gameState.currLives < prev_lives) {
          resultQueue.push(new_agent);
          return;
        }

        bfsQueue.push({new_agent, curr_depth + 1});
      }));
    }

    for (auto& fut : futures) {
      fut.get();
    }
  }

  return resultQueue;
}

int DecisionModule::deepSearch(int depth, GameAgent& currAgent) {
  if (_agent.gameState.currLives == 0 || depth == _depthLimit ||
      numPellets(_agent.gameState.pelletArr) == 0) {
    return evaluateState(currAgent) - depth * 100;
  }
  Location p_loc = _agent.gameState.pacmanLoc;

  std::vector<std::pair<int, int>> targets = {
      {p_loc.getRow(), p_loc.getCol()},
      {p_loc.getRow() + 1, p_loc.getCol()},
      {p_loc.getRow() - 1, p_loc.getCol()},
      {p_loc.getRow(), p_loc.getCol() + 1},
      {p_loc.getRow(), p_loc.getCol() - 1}};
  std::vector<Directions> dir = {NONE, DOWN, UP, RIGHT, LEFT};
  std::vector<int> heuristics;
  std::vector<std::future<int>> futures;

  auto search = [&](int i) -> int {
    if (wallAt(targets[i].first, targets[i].second)) {
      return evaluateState(currAgent) -
             depth * 100;  // Skip this direction if there's a wall.
    }

    // Simulate the state
    GameAgent& agent = currAgent;
    int prev_lives = agent.gameState.currLives;
    agent.step(ACTION_TICK, dir[i]);

    if (!agent.gameState.currLives < prev_lives) {
      return evaluateState(currAgent) -
             depth * 100;  // Return penalty if simulation ends the game.
    }

    // Perform a recursive deep search and return the result.
    return this->deepSearch(depth + 1, currAgent);
  };

  for (unsigned int i = 0; i < targets.size(); ++i) {
    futures.push_back(std::async(std::launch::async, search, i));
  }

  for (auto& fut : futures) {
    heuristics.push_back(fut.get());
  }

  if (heuristics.empty()) {
    return evaluateState(currAgent) - depth * 100;
  }
  int max_val = *(std::max_element(heuristics.begin(), heuristics.end()));
  return max_val + evaluateState(currAgent) - depth * 100;
}

Directions DecisionModule::decide() {
  Location p_loc = _agent.gameState.pacmanLoc;

  if (numPellets(_agent.gameState.pelletArr) <= _depthLimit) {
    _depthLimit = numPellets(_agent.gameState.pelletArr) - 1;
  }
  std::vector<std::pair<int, int>> targets = {
      {p_loc.getRow(), p_loc.getCol()},
      {p_loc.getRow() + 1, p_loc.getCol()},
      {p_loc.getRow() - 1, p_loc.getCol()},
      {p_loc.getRow(), p_loc.getCol() + 1},
      {p_loc.getRow(), p_loc.getCol() - 1}};
  std::vector<Directions> dir = {NONE, DOWN, UP, RIGHT, LEFT};
  std::vector<int> action_scores(5, -INT_MAX);

  for(int i = 0; i < 5; i++) {
    if (wallAt(targets[i].first, targets[i].second)) {
      continue;
    }
    _agent.step(ACTION_TICK,dir[i]);
    action_scores[i] = deepSearch(0,_agent);
  }

  auto max_action =
      std::max_element(action_scores.begin(), action_scores.end());
  return dir[max_action - action_scores.begin()];
}