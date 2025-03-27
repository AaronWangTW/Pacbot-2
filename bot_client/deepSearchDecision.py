import threading
import asyncio
import random
import copy
import time
from queue import Queue
import numpy as np
from concurrent.futures import ThreadPoolExecutor

from gameState import *
from variables import *
from websockets.sync.client import ClientConnection
from serverMessage import ServerMessage
from grid import grid

D_MESSAGES = [b"w", b"a", b"s", b"d", b"."]
TICK_ESTIMATE_BY_LEVEL = [12, int(12 * 1.5), 12 * 2, int(12 * 2.5), 12 * 3]
FOOD_POSITIONS = []

PELLET_WEIGHT = 0.65
GHOST_WEIGHT = 0.35
FRIGHTENED_GHOST_WEIGHT = 2 * GHOST_WEIGHT
GHOST_CUTOFF = 10
GHOST_FRIGHTENED_CUTOFF = 13
CHERRY_WEIGHT = 2 * PELLET_WEIGHT
REVERSE_WEIGHT = 15

gs = GameState()

for col in range(28):
    for row in range(31):
        if gs.pelletAt(row, col):
            FOOD_POSITIONS.append((col, row))

def bfs(grid, start, target, max_dist=float("inf")):
    visited = []
    queue = [(start, [])]

    while len(queue) > 0:
        nxt = queue.pop(0)
        visited.append(nxt[0])
        new_path = copy.deepcopy(nxt[1])
        new_path.append(nxt[0])
        loc = nxt[0]
        if type(target) is tuple:
            if target == loc:
                return new_path
        elif grid[loc[0]][loc[1]] in target:
            return new_path

        if grid[loc[0] + 1][loc[1]] in [o, e, O] and (loc[0] + 1, loc[1]) not in visited and len(new_path) <= max_dist:
            queue.append(((loc[0] + 1, loc[1]),new_path))
        if grid[loc[0] - 1][loc[1]] in [o, e, O] and (loc[0] - 1, loc[1]) not in visited and len(new_path) <= max_dist:
            queue.append(((loc[0] - 1, loc[1]),new_path))
        if grid[loc[0]][loc[1] + 1] in [o, e, O] and (loc[0], loc[1] + 1) not in visited and len(new_path) <= max_dist:
            queue.append(((loc[0], loc[1] + 1),new_path))
        if grid[loc[0]][loc[1] - 1] in [o, e, O] and (loc[0], loc[1] - 1) not in visited and len(new_path) <= max_dist:
            queue.append(((loc[0], loc[1] - 1),new_path))

    return None

class DeepDecisionModule:
    def __init__(self, state: GameState) -> None:
        self.state = state
        self.depth = 6
        self.task_queue = Queue()
        self.results = {}
        self.lock = threading.Lock()

    def set_connection(self, connection: ClientConnection):
        self.connection = connection
        print("connected")

    def _get_direction(self, p_loc: Location, next_loc: tuple):
        if p_loc.col == next_loc[0]:
            if p_loc.row < next_loc[1]:
                return Directions.DOWN
            else:
                return Directions.UP
        else:
            if p_loc.col < next_loc[0]:
                return Directions.RIGHT
            else:
                return Directions.LEFT

    def _update_game_state(self):
        message = self.connection.recv()

        # Convert the message to bytes, if necessary
        messageBytes: bytes
        if isinstance(message, bytes):
            messageBytes = message  # type: ignore
        else:
            messageBytes = message.encode("ascii")  # type: ignore

        # Update the state, given this message from the server
        self.state.update(messageBytes, True)

    def _send_command_message_to_target(self, direction):
        self.state.queueAction(2, direction)
        # self.connection.send(ServerMessage(D_MESSAGES[direction], 4).getBytes())

    def _send_stop_command(self):
        self.state.queueAction(2, Directions.NONE)
        # self.connection.send(ServerMessage(D_MESSAGES[4], 4).getBytes())

    def _send_socket_command_to_target(self, p_loc, target):
        direction = self._get_direction(p_loc, target)
        match direction:
            case Directions.UP:
                direction = b"n"
            case Directions.DOWN:
                direction = b"s"
            case Directions.LEFT:
                direction = b"w"
            case Directions.RIGHT:
                direction = b"e"
        self.sock.send(direction)
        print(direction)

    def _send_socket_stop_command(self):
        self.sock.send(b"x")
        print("stay in place")

    def evaluationFunction(self, game_state: GameState):
        pacman_pos = np.array([game_state.pacmanLoc.row, game_state.pacmanLoc.col])
        score = game_state.currScore
        pellet_arr = game_state.pelletArr

        # Convert pellet positions to numpy array for fast distance calculations
        pellet_positions = np.argwhere(pellet_arr)
        if pellet_positions.size > 0:
            pellet_distances = np.abs(pellet_positions - pacman_pos).sum(axis=1)
            min_pellet_dist = np.min(pellet_distances)
            pellet_score = 10 / (min_pellet_dist + 1)
        else:
            pellet_score = 0

        # Ghost avoidance & hunting
        ghost_positions = np.array(
            [[g.location.row, g.location.col] for g in game_state.ghosts]
        )
        ghost_states = np.array([g.isFrightened() for g in game_state.ghosts])

        if ghost_positions.size > 0:
            ghost_distances = np.abs(ghost_positions - pacman_pos).sum(axis=1)
            frightened_ghosts = -500 * ghost_states * (ghost_distances - 5)
            active_ghosts = (1 - ghost_states) * (
                -500 / (ghost_distances + 1) * (ghost_distances < 5)
            )

            ghost_reward = frightened_ghosts.sum()
            ghost_penalty = active_ghosts.sum()
        else:
            ghost_reward = 0
            ghost_penalty = 0

        # Fruit bonus
        fruit_bonus = 500 if game_state.fruitAt(*pacman_pos) else 0

        return (
            score
            + pellet_score
            + ghost_reward
            + fruit_bonus
            + ghost_penalty
        )

    def _find_distance_of_closest_pellet(self, target_loc):
        return len(bfs(self.grid, target_loc, [o])) - 1

    def _find_distance_of_closest_powerup(self, target_loc):
        return len(bfs(self.grid, target_loc, [O])) - 1

    def  _find_paths_to_closest_ghosts_state(self, state:GameState):
        ghosts = state.ghosts
        state_paths = [(ghost.frightSteps > 2, bfs(self.grid, (state.pacmanLoc.col,30-state.pacmanLoc.row), (ghost.location.col, 30 - ghost.location.row), GHOST_CUTOFF)) for ghost in ghosts]
        return [sp for sp in state_paths if sp[1] is not None]

    def _find_distance_to_cherry(self, target_loc):
        return len(bfs(self.grid, target_loc, (self.state.fruitLoc.col, 30- self.state.fruitLoc.row))) - 1

    def oldEvaluationFunction(self, game_state: GameState):
        pacman_pos = np.array([game_state.pacmanLoc.row, game_state.pacmanLoc.col])
        pacman_direction = game_state.pacmanLoc.getDirection()
        score = game_state.currScore
        pellet_arr = game_state.pelletArr
            
        if game_state.wallAt(pacman_pos[0],pacman_pos[1]):
            return float('-inf')
        
        # Calculate pellet distance heuristic
        if game_state.numPellets() - game_state.numPowerups == 0:  # No more power-ups
            dist_to_pellet = self._find_distance_of_closest_powerup(pacman_pos)
        else:
            dist_to_pellet = self._find_distance_of_closest_pellet(pacman_pos)
        
        # Calculate ghost proximity heuristic (ghosts are bad, but frightened ghosts are good)
        paths_to_ghosts = self._find_paths_to_closest_ghosts_state(game_state)
        ghosts = []
        ghost_heuristic = 0
        for state, path in paths_to_ghosts:
            dist = len(path) - 1
            ghosts.append((state, dist))
            if dist < GHOST_CUTOFF:
                if state == False:  # Normal ghost (bad)
                    ghost_heuristic += pow((GHOST_CUTOFF - dist), 2) * GHOST_WEIGHT
                else:  # Frightened ghost (good)
                    ghost_heuristic += pow((GHOST_CUTOFF - dist), 2) * -1 * FRIGHTENED_GHOST_WEIGHT
        
        # Pellet heuristic (closer to pellet = better)
        pellet_heuristic = dist_to_pellet * PELLET_WEIGHT
        
        # Fruit heuristic (if there's a fruit, go towards it)
        fruit_bonus = self._find_distance_to_cherry(pacman_pos) * CHERRY_WEIGHT if game_state.fruitLoc.col != 32 else 0
        
        # Combine all heuristics for this target
        total_heuristic = pellet_heuristic + ghost_heuristic + fruit_bonus
        
        # Return the evaluated score for the best target (this is where we return the heuristic score)
        return score + total_heuristic

    def worker(self):
        while True:
            task = self.task_queue.get()
            if task is None:
                break
            branch, depth, state = task
            score = self.deepSearch(branch, depth, state)
            with self.lock:
                if branch not in self.results or self.results[branch] < score:
                    self.results[branch] = score
            self.task_queue.task_done()

    def deepSearch(self, branch, depth, state: GameState):
        if state.currLives == 0 or depth >= self.depth or state.numPellets() == 0:
            return self.evaluationFunction(state)
            # return self.oldEvaluationFunction(state)

        best_score = float("-inf")

        p_loc = state.pacmanLoc
        directions = [Directions.LEFT, Directions.RIGHT, Directions.DOWN, Directions.UP]

        moves = []
        for i, (dx, dy) in enumerate([(-1, 0), (1, 0), (0, 1), (0, -1)]):
            target = (p_loc.col + dx, p_loc.row + dy)
            if not state.wallAt(target[1], target[0]):
                sim_state = fast_copy_game_state(state)
                safe = sim_state.simulateAction(
                    TICK_ESTIMATE_BY_LEVEL[state.currLevel - 1], directions[i]
                )
                if safe:
                    eval_score = self.evaluationFunction(sim_state)
                else:
                    eval_score = float("-inf")
                #eval_score = self.oldEvaluationFunction(sim_state)
                moves.append((eval_score, sim_state))

        moves.sort(reverse=True, key=lambda x: x[0])  # Prioritize best moves

        for eval_score, sim_state in moves:
            best_score = max(best_score, self.deepSearch(branch, depth + 1, sim_state))
        #print(best_score)
        return best_score

    def tick(self):
        if self.state.gameMode == GameModes.PAUSED:
            print("Paused")
            return

        self.results.clear()  # Reset results every tick

        p_loc = self.state.pacmanLoc
        directions = [Directions.LEFT, Directions.RIGHT, Directions.DOWN, Directions.UP]

        num_tasks = 0
        for i, (dx, dy) in enumerate([(-1, 0), (1, 0), (0, 1), (0, -1)]):
            target = (p_loc.col + dx, p_loc.row + dy)
            if not self.state.wallAt(target[1], target[0]):
                sim_state = fast_copy_game_state(self.state)
                sim_state.simulateAction(
                    TICK_ESTIMATE_BY_LEVEL[self.state.currLevel - 1], directions[i]
                )
                self.task_queue.put((i, 0, sim_state))
                num_tasks += 1

        if num_tasks == 0:
            print("No valid moves, skipping tick")
            return

        # Ensure worker threads persist across multiple ticks
        if not hasattr(self, "workers"):
            self.workers = [
                threading.Thread(target=self.worker, daemon=True) for _ in range(16)
            ]
            for w in self.workers:
                w.start()

        self.task_queue.join()

        if self.results:
            best_branch = max(self.results, key=self.results.get)
            self._send_command_message_to_target(directions[best_branch])
            print(directions[best_branch])

    async def decisionLoop(self) -> None:
        while self.state.isConnected():
            await asyncio.sleep(0)
            self.state.lock()
            await asyncio.to_thread(self.tick)  # Offload tick to a thread
            self.state.unlock()
            await asyncio.sleep(0.01)
