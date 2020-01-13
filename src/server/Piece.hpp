﻿#ifndef PIECE_HPP
#define PIECE_HPP

#include "utils.hpp"
#include <vector>

class Cell;

class Piece
{

public:
    Piece();
    Piece(int player, Cell* cell, std::vector<Move>* moveList);
    ~Piece();

public:
    int getPlayer() const;
    std::vector<Move>* getMoveList();

    Cell* getCell() const;
    void setCell(Cell* cell);

private:
    int _player;
    Cell* _cell;
    std::vector<Move>* _moveList;

};

#endif // PIECE_HPP
