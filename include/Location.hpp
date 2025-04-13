#pragma once
#include <array>
#include <cstdint>
#include <unordered_map>

enum Directions { UP, LEFT, DOWN, RIGHT, NONE };

const static std::array<Directions, 4> DIRECTIONS_LIST{
    Directions::UP, Directions::LEFT, Directions::DOWN, Directions::RIGHT};

const static std::array<Directions, 5> ALL_DIRECTIONS{
    Directions::UP, Directions::LEFT, Directions::DOWN, Directions::RIGHT, Directions::NONE};

const static std::unordered_map<Directions, Directions> REVERSE_DIRECTIONS{
    {Directions::UP, Directions::DOWN},
    {Directions::LEFT, Directions::RIGHT},
    {Directions::DOWN, Directions::UP},
    {Directions::RIGHT, Directions::LEFT},
    {Directions::NONE, Directions::NONE}
};

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
