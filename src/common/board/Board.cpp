#include "Board.hpp"
#include "Piece.hpp"
#include "Cell.hpp"

#include <iostream>
#include <algorithm>

Board::Board() :
    _boardCells(),
    _campCells(),
    _piecesW(),
    _piecesB(),
    _gameFinished(false),
    _currentPlayer(BoardPlayer::None),
    _winningPlayer(BoardPlayer::None)
{
    // Initialization of the BoardCells
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            _boardCells[row][col] = BoardCell(CellType::NONE,row,col);
        }
    }

    // Initialization of the CampCells
    _campCells[CampCell::Type::WhiteLeft] = CampCell(-1,-1,64);
    _campCells[CampCell::Type::WhiteRight] = CampCell(-1,9,65);
    _campCells[CampCell::Type::BlackLeft] = CampCell(9,-1,66);
    _campCells[CampCell::Type::BlackRight] = CampCell(9,9,67);

    _piecesW.reserve(8);
    _piecesB.reserve(8);
}

Board::~Board()
{
    for(size_t i = 0; i < _piecesW.size(); ++i)
	{
        delete _piecesW[i];
		_piecesW[i] = nullptr;
	}
    _piecesW.clear();
    for(size_t i = 0; i < _piecesB.size(); ++i)
	{
        delete _piecesB[i];
		_piecesB[i] = nullptr;
	}
    _piecesB.clear();
}

/* Returns the current configuration of the board with for each cell
 * its type and whether there is a piece on that cell,
 * for each camp cell whether there is a piece on that cell,
 * the current player and whether the game is finished or not
 */
const std::string Board::getBoardConfiguration() const
{
    std::string s = "";

    // Populate the cells of the board
    for(int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            const BoardCell& cell = _boardCells[row][col];
            switch(cell.getType())
            {
            case CellType::KING:
                s += "K";
                break;
            case CellType::BISHOP:
                s += "B";
                break;
            case CellType::KNIGHT:
                s += "N";
                break;
            case CellType::ROOK:
                s += "R";
                break;
            default:
                s += " ";
            }

            Piece* piece = cell.getPiece();
            if(piece)
                s += (piece->getPlayer() == BoardPlayer::White ? "+" : "-");
            else
                s += " ";
        }
    }

    // Populate the camp cells
    s += (_campCells.at(CampCell::Type::WhiteLeft).isEmpty() ? " " : "-");
    s += (_campCells.at(CampCell::Type::WhiteRight).isEmpty() ? " " : "-");
    s += (_campCells.at(CampCell::Type::BlackLeft).isEmpty() ? " " : "+");
    s += (_campCells.at(CampCell::Type::BlackRight).isEmpty() ? " " : "+");

    // Then put the current player and whether the game is finished
    s += (_currentPlayer == BoardPlayer::White ? "+" : "-");

    if(isGameFinished())
        s += (whoWon() == BoardPlayer::White ? "+" : "-");
    else
        s += " ";

    return s;
}

void Board::setBoardConfiguration(const std::string &boardString)
{
    clearAllPieces();

    // Set Board cells and retrieve pieces
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            size_t index = 2 * ((row * 8) + col);
            _boardCells[row][col].setType(charToCellType(boardString.at(index)));

            char c = boardString.at(index + 1);
            if (c == '+')
            {
                // The piece is linked to the corresponding cell in the constructor
                Piece* piece = new Piece(BoardPlayer::White, &_boardCells[row][col]);
                _piecesW.push_back(piece);
            }
            else if (c == '-')
            {
                // The piece is linked to the corresponding cell in the constructor
                Piece* piece = new Piece(BoardPlayer::Black, &_boardCells[row][col]);
                _piecesB.push_back(piece);
            }
            else
            {
                _boardCells[row][col].setPiece(nullptr);
            }
        }
    }

    // Set Camp cells
    if (boardString.at(128) == '-')
    {
        Piece* piece = new Piece(BoardPlayer::Black, &_campCells.at(CampCell::Type::WhiteLeft));
        _piecesB.push_back(piece);
    }
    if (boardString.at(129) == '-')
    {
        Piece* piece = new Piece(BoardPlayer::Black, &_campCells.at(CampCell::Type::WhiteRight));
        _piecesB.push_back(piece);
    }
    if (boardString.at(130) == '+')
    {
        Piece* piece = new Piece(BoardPlayer::White, &_campCells.at(CampCell::Type::BlackLeft));
        _piecesW.push_back(piece);
    }
    if (boardString.at(131) == '+')
    {
        Piece* piece = new Piece(BoardPlayer::White, &_campCells.at(CampCell::Type::BlackRight));
        _piecesW.push_back(piece);
    }

    // Set current player
    _currentPlayer = (boardString.at(132) == '+' ? BoardPlayer::White : BoardPlayer:: Black);

    BoardPlayer winPlayer = (boardString.at(133) == '+' ? BoardPlayer::White : (boardString.at(133) == '-' ? BoardPlayer::Black : BoardPlayer::None));

    updateGameFinished();

    // Last check
    if (_winningPlayer != winPlayer)
    {
        std::cout << "Something went wrong, the winning player is different from the boardConfig" << std::endl;
        std::cout << "In Board:" << to_string(_winningPlayer) << " and bordConfig: " << to_string(winPlayer) << std::endl;
    }
}


void Board::initBoard(const std::string& boardString)
{
    int index = 0;

    // Iterates over rows
    for(int row = 0; row < 8; ++row)
    {
        // Iterates over columns
        for(int col = 0; col < 8; ++col)
        {
            BoardCell& cell = _boardCells[row][col];
            const char charType = boardString.at(index);

            switch (charType)
            {
            case 'K':
                cell.setType(CellType::KING);
                break;
            case 'R':
                cell.setType(CellType::ROOK);
                break;
            case 'B':
                cell.setType(CellType::BISHOP);
                break;
            case 'N':
                cell.setType(CellType::KNIGHT);
                break;
            default:
                cell.setType(CellType::NONE);
            }
            index++;
        }
    }

    // Initialization of Black and White Pieces
    clearAllPieces();
    for(int col = 0; col < 8; ++col)
    {
        // For White
        Piece* pieceW = new Piece(BoardPlayer::White, &_boardCells[0][col]);
        _piecesW.push_back(pieceW);

        // For Black
        Piece* pieceB = new Piece(BoardPlayer::Black, &_boardCells[7][col]);
        _piecesB.push_back(pieceB);
    }

    _currentPlayer = BoardPlayer::White;
    _winningPlayer = BoardPlayer::None;

    _piecesW.push_back(new Piece(BoardPlayer::White, &_boardCells[6][4]));
    _piecesW.push_back(new Piece(BoardPlayer::White, &_campCells[CampCell::Type::BlackRight]));
}


std::vector<int> Board::getReachableCells(int index)
{
    std::vector<int> reachable;
    BoardCell c = indexToBoardCell(index);

    if (c.getPiece() != nullptr)
    {
        std::vector<Move> moves;
        fillAllMoves(c.getRow(), c.getColumn(), &moves);
        for (Move m : moves)
        {
            reachable.push_back(m.second.getIndex());
        }
    }
    return reachable;
}

void Board::clearAllPieces()
{
    for (Piece* p : _piecesW)
    {
        delete p;
    }
    _piecesW.clear();
    for (Piece* p : _piecesB)
    {
        delete p;
    }
    _piecesB.clear();
}

bool Board::isValidMove(const Move& m, BoardPlayer player) const
{
    if (m.first.isEmpty() || m.first.getPiece()->getPlayer() != player)
        return false;

    if (!m.second.isEmpty() && m.second.getPiece()->getPlayer() == player)
        return false;

    int src_row = m.first.getRow();
    int src_col = m.first.getColumn();

    std::vector<Move> moves;
    fillAllMoves(src_row, src_col, &moves);

    for (const Move& move: moves) { // TODO we could use a find here
        if (move.second == m.second)
            return true;
    }

    return false;
}

bool Board::isValidMove(const std::string& move_str, BoardPlayer player) const
{
    Move m = stringToMove(move_str);
    return isValidMove(m, player);
}

bool Board::isValidMove(int src, int dest, BoardPlayer player) const
{
    Move m = indicesToMove(src, dest);
    return isValidMove(m, player);
}

bool Board::playMove(const Move& move)
{
    if (!isValidMove(move, getCurrentPlayer()))
        return false;

    Cell& src_cell = indexToBoardCell(move.first.getIndex());
    Cell& dst_cell = indexToCell(move.second.getIndex());
    Piece * piece = src_cell.getPiece();

    if(!dst_cell.isEmpty())
        removePiece(dst_cell.getPiece());

    dst_cell.setPiece(piece);
    src_cell.setPiece(nullptr);
    piece->setCell(&dst_cell);
    nextPlayer();

    updateGameFinished();

    return true;
}

bool Board::playMove(const std::string& move_str)
{
    Move m = stringToMove(move_str);
    return playMove(m);
}

bool Board::playMove(int src, int dest)
{
    Move m = indicesToMove(src, dest);
    return playMove(m);
}

void Board::fillAllMoves(int row, int col, std::vector<Move>* list) const
{
    // TODO Moves should be simplified into a pair of indices instead of Cells
    // TODO and this function should return only the indices of reachable Cells
    const BoardCell& cell = _boardCells[row][col];

    if (cell.isEmpty() || cell.isCampCell())
        return;

    BoardPlayer player = cell.getPiece()->getPlayer();

    // Check if the piece can go to an adversary camp
    if (row == 0 && player == BoardPlayer::Black)
    {
        if(_campCells.at(CampCell::WhiteLeft).isEmpty())
            list->push_back(Move(cell,_campCells.at(CampCell::WhiteLeft)));

        if(_campCells.at(CampCell::WhiteRight).isEmpty())
            list->push_back(Move(cell,_campCells.at(CampCell::WhiteRight)));
    }
    if (row == 7 && player == BoardPlayer::White)
    {
        if(_campCells.at(CampCell::BlackLeft).isEmpty())
            list->push_back(Move(cell,_campCells.at(CampCell::BlackLeft)));

        if(_campCells.at(CampCell::BlackRight).isEmpty())
            list->push_back(Move(cell,_campCells.at(CampCell::BlackRight)));
    }

    switch(cell.getType())
    {
    case CellType::KING:
    {
        // all deltas move from a King cell (check cells directly around)
        static const std::vector< std::pair<int,int> > deltas = {
            {-1,-1}, {-1, 0}, {-1, 1}, { 0,-1}, { 0, 1}, { 1,-1}, { 1, 0}, { 1, 1}
        };

        for (const std::pair<int,int>& delta : deltas)
        {
            int new_row = row + delta.first;
            int new_col = col + delta.second;

            if(new_row < 0 || new_row > 7 || new_col < 0 || new_col > 7)
                continue;

            const BoardCell& new_cell = _boardCells[new_row][new_col];
            if(new_cell.isEmpty() || new_cell.getPiece()->getPlayer() == otherPlayer(player))
                list->push_back(Move(cell,new_cell));
        }
        break;
    }

    case CellType::BISHOP:
    {
        // all direction move from a Bishop cell (check cells in diagonal until next edge or Bishop cell or oponnent)
        static const std::vector< std::pair<int,int> > directions = {
            {-1,-1}, {-1, 1}, { 1,-1}, { 1, 1}
        };

        for (const std::pair<int,int> direction : directions)
        {
            int delta_row = direction.first;
            int delta_col = direction.second;

            int new_row = row + delta_row;
            int new_col = col + delta_col;

            while(new_row >= 0 && new_row <= 7 && new_col >= 0 && new_col <= 7) {
                const BoardCell& new_cell = _boardCells[new_row][new_col];

                if(new_cell.isEmpty())
                {
                    list->push_back(Move(cell,new_cell));
                    if(new_cell.getType() == CellType::BISHOP)
                        break;
                }
                else
                {
                    if(new_cell.getPiece()->getPlayer() == otherPlayer(player))
                        list->push_back(Move(cell,new_cell));

                    break;
                }

                new_row += delta_row;
                new_col += delta_col;
            }
        }
        break;
    }

    case CellType::ROOK: // Check cells in straight line until Rook cell or edge or oponnent
    {
        // all direction move from a Rook cell (check cells in diagonal until next edge or Bishop cell or oponnent)
        static const std::vector< std::pair<int,int> > directions = {
            {-1, 0}, { 1, 0}, { 0,-1}, { 0, 1}
        };

        for (const std::pair<int,int> direction : directions)
        {
            int delta_row = direction.first;
            int delta_col = direction.second;

            int new_row = row + delta_row;
            int new_col = col + delta_col;

            while(new_row >= 0 && new_row <= 7 && new_col >= 0 && new_col <= 7) {
                const BoardCell& new_cell = _boardCells[new_row][new_col];

                if(new_cell.isEmpty())
                {
                    list->push_back(Move(cell,new_cell));
                    if(new_cell.getType() == CellType::ROOK)
                        break;
                }
                else
                {
                    if(new_cell.getPiece()->getPlayer() == otherPlayer(player))
                        list->push_back(Move(cell,new_cell));

                    break;
                }

                new_row += delta_row;
                new_col += delta_col;
            }
        }
        break;
    }

    case CellType::KNIGHT:
    {
        // all deltas move from a Knight cell
        static const std::vector< std::pair<int,int> > deltas = {
            {-1,-2}, { 1,-2}, {-1, 2}, { 1, 2}, {-2,-1}, { 2,-1}, {-2, 1}, { 2, 1}
        };

        for (const std::pair<int,int>& delta : deltas)
        {
            int new_row = row + delta.first;
            int new_col = col + delta.second;

            if(new_row < 0 || new_row > 7 || new_col < 0 || new_col > 7)
                continue;

            const BoardCell& new_cell = _boardCells[new_row][new_col];
            if(new_cell.isEmpty() || new_cell.getPiece()->getPlayer() == otherPlayer(player))
                list->push_back(Move(cell,new_cell));
        }
        break;
    }

    default:
        break;
    }
}

void Board::updateGameFinished()
{
    // A player won if the two Camp cells of its oponnent are occupied or if the oponnend has only one piece remaining
    // Check if the White player won
    if ((!_campCells.at(CampCell::Type::BlackLeft).isEmpty() && !_campCells.at(CampCell::Type::BlackRight).isEmpty()) ||
        _piecesB.size() < 2)
    {
        _winningPlayer = BoardPlayer::White;
        _gameFinished = true;
    }
    // Check if the Black player won
    else if ((!_campCells.at(CampCell::Type::WhiteLeft).isEmpty() && !_campCells.at(CampCell::Type::WhiteRight).isEmpty()) ||
             _piecesW.size() < 2)
    {
        _winningPlayer = BoardPlayer::Black;
        _gameFinished = true;
    }
}

bool Board::isGameFinished() const
{
    return _gameFinished;
}

BoardPlayer Board::whoWon() const
{
    return _winningPlayer;
}

BoardPlayer Board::getCurrentPlayer() const
{
    return _currentPlayer;
}

void Board::setCurrentPlayer(BoardPlayer &player)
{
    _currentPlayer = player;
}

void Board::nextPlayer()
{
    _currentPlayer = (_currentPlayer == BoardPlayer::White) ? BoardPlayer::Black : BoardPlayer::White;
}


void Board::removePiece(Piece* p)
{
    std::cout << "Outch! " << ((p->getPlayer() == BoardPlayer::White) ? "White" : "Black") << " just lost a Piece!" << std::endl;
    std::vector<Piece*> * list = p->getPlayer() == BoardPlayer::White ? &_piecesW : &_piecesB;

    auto it = std::find(list->begin(), list->end(), p);
    if (it != list->end())
        list->erase(it);

    delete p;
}

BoardCell& Board::indexToBoardCell(int cell_index)
{
    if(cell_index < 0 || cell_index > 63)
        throw std::runtime_error("no BoardCell defined for index " + std::to_string(cell_index));

    int row = cell_index / 8;
    int col = cell_index % 8;

    return _boardCells[row][col];
}

const BoardCell& Board::indexToBoardCell(int cell_index) const
{
    if(cell_index < 0 || cell_index > 63)
        throw std::runtime_error("no BoardCell defined for index " + std::to_string(cell_index));

    int row = cell_index / 8;
    int col = cell_index % 8;

    return _boardCells[row][col];
}

CampCell& Board::indexToCampCell(int campcell_index)
{
    switch(campcell_index)
    {
    case 64:
        return _campCells.at(CampCell::Type::WhiteLeft);
        break;
    case 65:
        return _campCells.at(CampCell::Type::WhiteRight);
        break;
    case 66:
        return _campCells.at(CampCell::Type::BlackLeft);
        break;
    case 67:
        return _campCells.at(CampCell::Type::BlackRight);
        break;
    default:
        throw std::runtime_error("no CampCell defined for index " + std::to_string(campcell_index));
    }
}

const CampCell& Board::indexToCampCell(int campcell_index) const
{
    switch(campcell_index)
    {
    case 64:
        return _campCells.at(CampCell::Type::WhiteLeft);
        break;
    case 65:
        return _campCells.at(CampCell::Type::WhiteRight);
        break;
    case 66:
        return _campCells.at(CampCell::Type::BlackLeft);
        break;
    case 67:
        return _campCells.at(CampCell::Type::BlackRight);
        break;
    default:
        throw std::runtime_error("no CampCell defined for index " + std::to_string(campcell_index));
    }
}

Cell& Board::indexToCell(int cell_index)
{
    if (cell_index > 63)
    {
        return indexToCampCell(cell_index);
    }
    else
    {
        return indexToBoardCell(cell_index);
    }
}

const Cell& Board::indexToCell(int cell_index) const
{
    if (cell_index > 63)
    {
        return indexToCampCell(cell_index);
    }
    else
    {
        return indexToBoardCell(cell_index);
    }
}

// Converts a Move to a string representation of that Move
std::string Board::moveToString(const Move& move) const
{
    int src_index = move.first.getIndex();
    int dest_index = move.second.getIndex();
    std::string s;

    // TODO add the current player at the beginning of this move string

    if (src_index == 64 || src_index == 65)
    {
        s = "+" + std::to_string(src_index); // A CampCell
    }
    else if (src_index == 66 || src_index == 67)
    {
        s = std::to_string(src_index); // A CampCell
    }
    else if (src_index < 10)
    {
        s = "0" + std::to_string(src_index);
    }
    else
    {
        s = std::to_string(src_index);
    }
    s += ":";
    if (dest_index == 64 || dest_index == 65)
    {
        s += "+" + std::to_string(dest_index); // A CampCell
    }
    else if (dest_index == 66 || dest_index == 67)
    {
        s += std::to_string(dest_index); // A CampCell
    }
    else if (dest_index < 10)
    {
        s += "0" + std::to_string(dest_index);
    }
    else
    {
        s += std::to_string(dest_index);
    }

    if (s.size() != 5)
    {
        std::cout << s << " " << s.size() << std::endl;
        std::cout << "ERROR while transcripting a Move ("<< src_index << ":" << dest_index << ") to a string." << std::endl;
        //TODO RAISE ERROR
        return std::string();
    }
    return s;
}

// Converts a string representation of a Move to a Move
Move Board::stringToMove(const std::string& move_str) const
{
    /* A string move is of the form:
     * m<src_cell_index>:<dest_cell_index>
     * Where the index is between 0 and 63 for a Board cell
     * and 64/65/66/67 for a Camp cell. */
    if (move_str.size() != 5)
        throw std::runtime_error("error while retrieving a Move from a string (" + move_str + ").");

    std::string src_str = move_str.substr(0, 2);
    std::string dest_str = move_str.substr(3, 2);

    return Move(indexToCell(stoi(src_str)),indexToCell(stoi(dest_str)));
}

Move Board::indicesToMove(int src, int dest) const
{
    return Move(indexToCell(src), indexToCell(dest));
}
