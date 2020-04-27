#include "Cell.hpp"
#include "Piece.hpp"

char cellTypeToChar(CellType& type)
{
    char s;
    switch(type)
    {
    case CellType::KING:
        s = 'K';
        break;
    case CellType::BISHOP:
        s = 'B';
        break;
    case CellType::ROOK:
        s = 'R';
        break;
    case CellType::KNIGHT:
        s = 'N';
        break;
    default:
        s = ' ';
        break;
    }
    return s;
}

CellType charToCellType(char s)
{
    if (s == 'K')
        return CellType::KING;
    else if (s == 'N')
        return CellType::KNIGHT;
    else if (s == 'B')
        return CellType::BISHOP;
    else if (s == 'R')
        return CellType::ROOK;
    else
        return CellType::NONE;
}

Cell::Cell(int row, int column) :
    _piece(nullptr),
    _row(row),
    _column(column)
{
}

Cell::~Cell()
{
}

bool Cell::isEmpty() const
{
    return _piece == nullptr;
}

Piece *Cell::getPiece() const
{
    return _piece;
}

void Cell::setPiece(Piece* piece)
{
    _piece = piece;
}

int Cell::getIndex() const
{
    return _index;
}

int Cell::getRow() const
{
    return _row;
}

int Cell::getColumn() const
{
    return _column;
}

BoardCell::BoardCell() :
    Cell(-1,-1),
    _type(CellType::NONE)
{

}

BoardCell::BoardCell(CellType type, int row, int column) :
    Cell(row,column),
    _type(type)
{
    _index = (row * 8) + column;
}

CellType BoardCell::getType() const
{
    return _type;
}

void BoardCell::setType(CellType type)
{
    _type = type;
}

bool BoardCell::isCampCell() const
{
    return false;
}

CampCell::CampCell() :
    Cell(-1,-1)
{
    _index = -1;
}

CampCell::CampCell(int row, int column, int index) :
    Cell(row, column)
{
    _index = index;
}

bool CampCell::isCampCell() const
{
    return true;
}
