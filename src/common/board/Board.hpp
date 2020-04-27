#ifndef BOARD_HPP
#define BOARD_HPP

#include "board_utils.hpp"

// Standard Library
#include <string>
#include <vector>
#include <map>
//#include <utility>

class Piece;

/**
 * @brief The Board class
 */
class Board
{

public:
    Board();
    virtual ~Board();

public:
    const std::string getBoardConfiguration() const;
    void setBoardConfiguration(const std::string& boardString);
    void initBoard(const std::string& boardString);

    bool isValidMove(const Move& m, BoardPlayer player) const;
    bool isValidMove(const std::string& move_str, BoardPlayer player) const;
    bool isValidMove(int src, int dest, BoardPlayer player) const;

    bool playMove(const Move& move);
    bool playMove(const std::string& move_str);
    bool playMove(int src, int dest);

    BoardPlayer getCurrentPlayer() const;
    void setCurrentPlayer(BoardPlayer& player);
    bool isGameFinished() const;
    BoardPlayer whoWon() const;

private:
    void nextPlayer();
    void removePiece(Piece* p);
    void clearAllPieces();
    const BoardCell& indexToBoardCell(int cell_index) const;
    BoardCell& indexToBoardCell(int cell_index);
    const CampCell& indexToCampCell(int campcell_index) const;
    CampCell& indexToCampCell(int campcell_index);
    const Cell& indexToCell(int cell_index) const;
    Cell& indexToCell(int cell_index);

    // Fills the list with possible Moves of the Piece in the cell (row,col)
    void fillAllMoves(int row, int col, std::vector<Move>* moveList) const;
    void updateGameFinished();

    // Converts a Move to a string representation of that Move
    std::string moveToString(const Move& move) const;
    // Converts a string representation of a Move to a Move
    Move stringToMove(const std::string& move_str) const;

    Move indicesToMove(int src, int dest) const;

private:
    BoardCell                           _boardCells[8][8];
    std::map<CampCell::Type, CampCell>  _campCells;

    std::vector<Piece*>                 _piecesW;       // The list of White Pieces
    std::vector<Piece*>                 _piecesB;		// The list of Black Pieces

    bool _gameFinished;

    BoardPlayer _currentPlayer;
    BoardPlayer _winningPlayer;
};

#endif // BOARD_HPP
