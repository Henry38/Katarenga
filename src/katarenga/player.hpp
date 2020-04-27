#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "utils.hpp"
#include <common/messages/message_utils.hpp>
#include <common/board/board_utils.hpp>

#include <zmqpp/zmqpp.hpp>

// Standard Library
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace zmqpp {
    class context;
    class poller;
}

class Board;

/**
 * @brief The Player class lives in the main thread
 */
class Player
{
    using Memo = std::pair<int,int>;

public:
    Player(GameSettings& game_settings);
    virtual ~Player();

public:
    void connect();
    void loop();    // function executed in the main thread

private:
    void process_server_answer_game_status(zmqpp::message& message);
    void process_server_move_message(zmqpp::message& message);
    void process_server_player_won(zmqpp::message& message);
    void process_server_game_stopped(zmqpp::message& message);

    void process_graphics_case_clicked(zmqpp::message& message);
    void process_graphics_ask_game_status(zmqpp::message& message);
    void process_graphics_stop_game(zmqpp::message& message);

private:
    void retrieve_piece_locations(const std::string& board_configuration);

    // Socket-related content
    zmqpp::context  m_zmq_context;
    zmqpp::poller   m_poller;
    zmqpp::socket   m_server_thread_socket;
    zmqpp::socket   m_render_thread_socket;

    std::thread     m_render_thread;

    MessageReactor  m_server_thread_reactor;
    MessageReactor  m_render_thread_reactor;

    // Game-related content
    std::unique_ptr<Board> m_board;
    bool m_game_inited;
    bool m_game_stopped;
    bool m_game_finished;
    BoardPlayer m_self_player;
    char m_self_player_sign;
    bool m_connected;
    std::vector<int> m_piece_locations; // The list of cell ids where my pieces are
    Memo m_memo;        // memo for case clicked by the player <src,dst>
};

#endif // PLAYER_HPP

