package game

import (
	"log"
	"math/rand"
	"sync"
	"time"
)

// Enum-like declaration to hold the game mode options
const (
	paused  uint8 = 0
	scatter uint8 = 1
	chase   uint8 = 2
)

/*
	NOTE: at 24 ticks/sec, currTicks will experience an integer overflow after
	about 45 minutes, so don't run it continuously for too long (indefinite
	pausing is fine though, as it doesn't increment the current tick amount)
*/

/*
A game state object, to hold the internal game state and provide
helper methods that can be accessed by the game engine
*/
type gameState struct {

	/* Message header - 4 bytes */

	currTicks uint16       // Current ticks (see note above)
	muTicks   sync.RWMutex // Associated mutex

	updatePeriod uint8        // Ticks / update
	muPeriod     sync.RWMutex // Associated mutex

	lastUnpausedMode uint8        // Last unpaused mode (for pausing purposes)
	mode             uint8        // Game mode
	muMode           sync.RWMutex // Associated mutex

	/* Game information - 4 bytes */

	currScore uint16       // Current score
	muScore   sync.RWMutex // Associated mutex

	currLevel uint8        // Current level (by default, starts at 1)
	muLevel   sync.RWMutex // Associated mutex

	currLives uint8        // Current lives (by default, starts at 3)
	muLives   sync.RWMutex // Associated mutex

	/* Pacman location - 2 bytes */

	pacmanLoc *locationState

	/* Fruit location - 2 bytes */

	fruitExists   bool
	fruitLoc      *locationState
	fruitSpawned1 bool
	fruitSpawned2 bool
	muFruit       sync.RWMutex // Associated mutex (for fruitExists)

	/* Ghosts - 4 * 3 = 12 bytes */

	ghosts []*ghostState

	/* Pellet State - 31 * 4 = 124 bytes */

	// Pellets encoded within an array, with each uint32 acting as a bit array
	pellets    [mazeRows]uint32
	numPellets uint16       // Number of pellets
	muPellets  sync.RWMutex // Associated mutex

	/* Auxiliary (non-serialized) state information */

	// Wall state
	walls [mazeRows]uint32

	// A random number generator for making frightened ghost decisions
	rng *rand.Rand
}

// Create a new game state with default values
func newGameState() *gameState {

	// New game state object
	gs := gameState{

		// Message header
		currTicks:        0,
		updatePeriod:     initUpdatePeriod,
		mode:             paused,
		lastUnpausedMode: initMode,

		// Game info
		currScore: 0,
		currLevel: initLevel,
		currLives: initLives,

		// Fruit
		fruitExists:   false,
		fruitSpawned1: false,
		fruitSpawned2: false,

		// Ghosts
		ghosts: make([]*ghostState, numColors),

		// RNG (random number generation) source
		rng: rand.New(rand.NewSource(time.Now().UnixNano())),

		// Pellet count at the start
		numPellets: initPelletCount,
	}

	// Declare the initial locations of Pacman and the fruit
	gs.pacmanLoc = newLocationStateCopy(pacmanSpawnLoc)
	gs.fruitLoc = newLocationStateCopy(fruitSpawnLoc)

	// Initialize the ghosts
	for color := uint8(0); color < numColors; color++ {
		gs.ghosts[color] = newGhostState(&gs, color)
	}

	// Copy over maze bit arrays
	copy(gs.pellets[:], initPellets[:])
	copy(gs.walls[:], initWalls[:])

	// Collect the first pellet (no lock necessary, as no other routine sees gs)
	gs.collectPellet(gs.pacmanLoc.getCoords())

	// Return the new game state
	return &gs
}

/**************************** Curr Ticks Functions ****************************/

// Helper function to get the update period
func (gs *gameState) getCurrTicks() uint16 {

	// (Read) lock the current ticks
	gs.muTicks.RLock()
	defer gs.muTicks.RUnlock()

	// Return the current ticks
	return gs.currTicks
}

// Helper function to increment the current ticks
func (gs *gameState) nextTick() {

	// (Write) lock the current ticks
	gs.muTicks.Lock()
	{
		gs.currTicks++ // Update the current ticks
	}
	gs.muTicks.Unlock()
}

/**************************** Upd Period Functions ****************************/

// Helper function to get the update period
func (gs *gameState) getUpdatePeriod() uint8 {

	// (Read) lock the update period
	gs.muPeriod.RLock()
	defer gs.muPeriod.RUnlock()

	// Return the update period
	return gs.updatePeriod
}

// Helper function to set the update period
func (gs *gameState) setUpdatePeriod(period uint8) {

	// Send a message to the terminal
	log.Printf("\033[36mGAME: Update period changed (%d -> %d)\033[0m\n",
		gs.getUpdatePeriod(), period)

	// (Write) lock the update period
	gs.muPeriod.Lock()
	{
		gs.updatePeriod = period // Update the update period
	}
	gs.muPeriod.Unlock()
}

/******************************* Mode Functions *******************************/

// Helper function to get the game mode
func (gs *gameState) getMode() uint8 {

	// (Read) lock the game mode
	gs.muMode.RLock()
	defer gs.muMode.RUnlock()

	// Return the current game mode
	return gs.mode
}

// Helper function to get the last unpaused mode
func (gs *gameState) getLastUnpausedMode() uint8 {

	// (Read) lock the game mode
	gs.muMode.RLock()
	defer gs.muMode.RUnlock()

	// If the current mode is not paused, return it
	if gs.mode != paused {
		return gs.mode
	}

	// Return the last unpaused game mode
	return gs.lastUnpausedMode
}

// Helper function to determine if the game is paused
func (gs *gameState) isPaused() bool {
	return gs.getMode() == paused
}

// Helper function to set the game mode
func (gs *gameState) setMode(mode uint8) {

	// (Write) lock the game mode
	gs.muMode.Lock()
	{
		gs.mode = mode // Update the game mode
	}
	gs.muMode.Unlock()
}

// Helper function to set the game mode
func (gs *gameState) setLastUnpausedMode(mode uint8) {

	// (Write) lock the game mode
	gs.muMode.Lock()
	{
		gs.lastUnpausedMode = mode // Update the game mode
	}
	gs.muMode.Unlock()
}

// Helper function to pause the game
func (gs *gameState) pause() {

	// If the game engine is already paused, there's no more to do
	if gs.isPaused() {
		return
	}

	// Otherwise, save the current mode
	gs.setLastUnpausedMode(gs.getMode())

	// Set the mode to paused
	gs.setMode(paused)
}

// Helper function to play the game
func (gs *gameState) play() {

	// If the game engine is already playing, there's no more to do
	if !gs.isPaused() {
		return
	}

	// Otherwise, set the current mode to the last unpaused mode
	gs.setMode(gs.getLastUnpausedMode())
}

/**************************** Game Score Functions ****************************/

// Helper function to get the current score of the game
func (gs *gameState) getScore() uint16 {

	// (Read) lock the current score
	gs.muScore.RLock()
	defer gs.muScore.RUnlock()

	// Return the current score
	return gs.currScore
}

// (For performance) helper function to increment the current score of the game
func (gs *gameState) incrementScore(change uint16) {

	// (Write) lock the current score
	gs.muScore.Lock()
	{
		gs.currScore += change // Add the delta to the score
	}
	gs.muScore.Unlock()
}

/**************************** Game Level Functions ****************************/

// Helper function to get the current level of the game
func (gs *gameState) getLevel() uint8 {

	// (Read) lock the current level
	gs.muLevel.RLock()
	defer gs.muLevel.RUnlock()

	// Return the current level
	return gs.currLevel
}

// Helper function to set the current level of the game
func (gs *gameState) setLevel(level uint8) {

	// (Write) lock the current level
	gs.muLevel.Lock()
	{
		gs.currLevel = level // Update the level
	}
	gs.muLevel.Unlock()
}

/**************************** Game Level Functions ****************************/

// Helper function to get the current level of the game
func (gs *gameState) getLives() uint8 {

	// (Read) lock the current lives
	gs.muLives.RLock()
	defer gs.muLives.RUnlock()

	// Return the current lives
	return gs.currLives
}

// Helper function to increment the current score of the game
func (gs *gameState) setLives(lives uint8) {

	// (Write) lock the current lives
	gs.muLives.Lock()
	{
		gs.currLives = lives // Update the lives
	}
	gs.muLives.Unlock()
}
