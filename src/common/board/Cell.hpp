#ifndef CELL_HPP
#define CELL_HPP

#include <string>

class Piece;

enum class CellType
{
    KING, ROOK, BISHOP, KNIGHT, NONE
};

char cellTypeToChar(CellType& type);
CellType charToCellType(char s);

// Base class for all cells of the Board
class Cell
{

public:
    Cell(int row, int column);
    virtual ~Cell();

public:
    // Tells whether the cell is empty or not
    bool isEmpty() const;

    Piece* getPiece() const;
    void setPiece(Piece* piece);

    int getIndex() const;
    int getRow() const;
    int getColumn() const;

public:
    bool operator==(const Cell& other) const
    {
        return _row == other._row && _column == other._column;
    }

    virtual bool isCampCell() const = 0;

protected:
    Piece* _piece;      // Points to the Piece on this Cell, or nullptr
    int _row;           // Row of this Cell
    int _column;        // Column of this Cell
    int _index;         // Index of this Cell, equals to 8*row+column-1 if BoardCell
                        // 1 or 2 for a White CampCell, -1 or -2 for a Black CampCell

};


// Basic cell of the board.
class BoardCell : public Cell
{

public:
    BoardCell();
    BoardCell(CellType type, int row, int column);

public:
    CellType getType() const;
    void setType(CellType type);

    virtual bool isCampCell() const override;

private:
    CellType _type;     // Type of the BoardCell

};


// Special cell of the board, for the camp cells of the players
class CampCell : public Cell
{

public:
    enum Type {
        WhiteLeft = 0,
        WhiteRight = 1,
        BlackLeft = 2,
        BlackRight = 3
    };

public:
    CampCell();
    CampCell(int row, int column, int index);

public:
    virtual bool isCampCell() const override;

};

#endif // CELL_HPP
