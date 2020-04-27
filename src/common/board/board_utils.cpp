#include "Board.hpp"

const char board_tiles[8][4][4] = {
                          {{'R', 'B', 'K', 'N'}, // 0
                           {'N', 'R', 'N', 'K'},
                           {'B', 'B', 'K', 'R'},
                           {'K', 'N', 'R', 'B'}},
                          {{'R', 'B', 'N', 'K'}, // 1
                           {'N', 'B', 'R', 'B'},
                           {'K', 'K', 'N', 'R'},
                           {'B', 'R', 'K', 'N'}},
                          {{'R', 'N', 'K', 'B'}, // 2
                           {'B', 'B', 'N', 'R'},
                           {'K', 'K', 'R', 'N'},
                           {'N', 'R', 'B', 'K'}},
                          {{'R', 'N', 'K', 'B'}, // 3
                           {'B', 'B', 'K', 'R'},
                           {'K', 'N', 'R', 'N'},
                           {'N', 'R', 'B', 'K'}},
                          {{'R', 'N', 'B', 'K'}, // 4
                           {'B', 'R', 'B', 'N'},
                           {'K', 'N', 'K', 'R'},
                           {'N', 'K', 'R', 'B'}},
                          {{'R', 'B', 'N', 'K'}, // 5
                           {'N', 'B', 'R', 'B'},
                           {'K', 'K', 'N', 'R'},
                           {'B', 'R', 'K', 'N'}},
                          {{'R', 'B', 'K', 'N'}, // 6
                           {'N', 'B', 'K', 'R'},
                           {'K', 'N', 'R', 'B'},
                           {'B', 'R', 'N', 'K'}},
                          {{'R', 'B', 'K', 'N'}, // 7
                           {'N', 'B', 'N', 'R'},
                           {'K', 'K', 'R', 'B'},
                           {'B', 'R', 'N', 'K'}}};

// Generates 4 different random values between 0 and 7
void randomPickTiles(int *a, int *b, int *c, int *d)
{
    srand (time(NULL));
    *a = rand() % 8;
    do {
        *b = rand() % 8;
    } while(*b == *a);
    do {
        *c = rand() % 8;
    } while(*c == *a || *c == *b);
    do {
        *d = rand() % 8;
    } while(*d == *a || *d == *b || *d == *c);
}

// Generates 4 random values between 0 and 3
void randomPickRotations(int *a, int *b, int *c, int *d)
{
    srand (time(NULL));
    *a = rand() % 4;
    *b = rand() % 4;
    *c = rand() % 4;
    *d = rand() % 4;
}

void generateRotation(char (&tab)[4][4], const char (&board_tile)[4][4], int rota)
{
    switch(rota)
    {
    case 0: // Normal rotation
        for(int i = 0; i < 4; ++i) {
            for(int j = 0; j < 4; ++j) {
                tab[i][j] = board_tile[i][j];
            }
        }
        break;
    case 1: // 90° clockwise
        for(int i = 0; i < 4; ++i) {
            for(int j = 0; j < 4; ++j) {
                tab[i][j] = board_tile[j][3 - i];
            }
        }
        break;
    case 2: // 180° clockwise
        for(int i = 0; i < 4; ++i) {
            for(int j = 0; j < 4; ++j) {
                tab[i][j] = board_tile[3-i][3 - j];
            }
        }
        break;
    case 3: // 270° clockwise
        for(int i = 0; i < 4; ++i) {
            for(int j = 0; j < 4; ++j) {
                tab[i][j] = board_tile[3 - j][i];
            }
        }
        break;
    }
}


std::string generateBoardCellTypes()
{
    int tile_top_left, tile_top_right, tile_bot_left, tile_bot_right;
    randomPickTiles(&tile_top_left, &tile_top_right, &tile_bot_left, &tile_bot_right);

    int rota_top_left, rota_top_right, rota_bot_left, rota_bot_right;
    randomPickRotations(&rota_top_left, &rota_top_right, &rota_bot_left, &rota_bot_right);

    char top_left[4][4];
    char top_right[4][4];
    char bot_left[4][4];
    char bot_right[4][4];

    generateRotation(top_left, board_tiles[tile_top_left], rota_top_left);
    generateRotation(top_right, board_tiles[tile_top_right], rota_top_right);
    generateRotation(bot_left, board_tiles[tile_bot_left], rota_bot_left);
    generateRotation(bot_right, board_tiles[tile_bot_right], rota_bot_right);

    std::string s1 = "";
    std::string s2 = "";
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            s1 += top_left[i][j];
            s2 += bot_left[i][j];
        }
        for(int j = 0; j < 4; ++j) {
            s1 += top_right[i][j];
            s2 += bot_right[i][j];
        }
    }

    std::string boardString = s1 + s2;
    return boardString;
}

std::string format_board(const std::string& board_configuration)
{
    std::string s = "";

    s += "\n";
    s += "    0  1  2  3  4  5  6  7   ";
    s += "\n";
    s += "             White           ";
    s += "\n";
    //s += "  X                         X";
    s += "  ";
    s += (board_configuration.at(128) == '-' ? "X" : " ");
    s += "                         ";
    s += (board_configuration.at(129) == '-' ? "X" : " ");
    s += "\n";

    std::string board = board_configuration.substr(0,128);

    for (int j = 0; j < 8; ++j) {
        //s += std::to_string(j+1) + "  ";
        s += std::to_string(j*8) + "  ";
        for (int i = 0; i < 8; ++i) {
            size_t index = (j * 8) + i;
            std::string c = board.substr(2*index,2);
            s += " " + c;
        }
        s += "\n";
    }

    //s += "  X                         X";
    s += "  ";
    s += (board_configuration.at(130) == '+' ? "X" : " ");
    s += "                         ";
    s += (board_configuration.at(131) == '+' ? "X" : " ");
    s += "\n";
    s += "             Black           ";

    return s;
}

BoardPlayer otherPlayer(const BoardPlayer &player)
{
    if (player == BoardPlayer::White)
        return BoardPlayer::Black;
    else if (player == BoardPlayer::Black)
        return BoardPlayer::White;
   else
        return BoardPlayer::None;
}

std::string to_string(const BoardPlayer &player)
{
    std::string s;
    if (player == BoardPlayer::White)
        s = "White";
    else if (player == BoardPlayer::Black)
        s = "Black";
    else
        s = "None";
    return s;
}
