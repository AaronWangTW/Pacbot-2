#pragma once
#include <array>
#include <cstdint>

enum Directions { UP, LEFT, DOWN, RIGHT, NONE };

const static std::array<Directions, 4> DIRECTIONS_LIST{
    Directions::UP, Directions::LEFT, Directions::DOWN, Directions::RIGHT};

const static std::array<Directions, 5> ALL_DIRECTIONS{
    Directions::UP, Directions::LEFT, Directions::DOWN, Directions::RIGHT, Directions::NONE};

const static std::array<int, 5> reversedDirections{
    Directions::DOWN, Directions::RIGHT, Directions::UP, Directions::LEFT,
    Directions::NONE};

class Location {
private:
  uint8_t row, col;

public:
  uint8_t getRow() const;
  void setRow(uint8_t row);

  uint8_t getRowDir() const;
  void setRowDir(uint8_t rowDir);

  uint8_t getCol() const;
  void setCol(uint8_t col);

  uint8_t getColDir() const;
  void setColDir(uint8_t colDir);
};
