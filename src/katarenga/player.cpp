#include "player.hpp"
#include "graphics.hpp"
#include <common/board/Board.hpp>

#include <functional>
#include <algorithm>


void Player::process_server_answer_game_status(zmqpp::message& message)
{
    AnswerGameStatus m = ConstructObject<AnswerGameStatus>(message);

    // Update internal state
    m_game_inited = true;
    m_board->setBoardConfiguration(m.getConfiguration());
    m_game_finished = m_board->isGameFinished();

    retrieve_piece_locations(m.getConfiguration()); // TODO maybe remove it and use the list of pieces in the Board

    // Then forward to the render thread
    m_render_thread_socket.send(message);
}

void Player::process_server_move_message(zmqpp::message& message)
{
    MoveMessage m = ConstructObject<MoveMessage>(message);
    BoardPlayer move_player = m.getPlayer();
    MoveType type = m.getType();

    if (type == MoveType::MovePlayed)
    {
        if (move_player != m_board->getCurrentPlayer())
        {
            throw std::runtime_error("Move received from a player not in his turn.");
            // TODO need to ask the correct board configuration to the server instead of throwing an error
        }
        else
        {
            int src = m.getSource();
            int dest = m.getDestination();

            // Update the new location of the piece if it was my move
            if (move_player == m_self_player)
            {
                auto it = std::find(m_piece_locations.begin(), m_piece_locations.end(), src);
                if (it != m_piece_locations.end())
                {
                    *it = dest;
                    player_msg("Updating piece location from " + std::to_string(src) + " to " + std::to_string(dest));
                }
                else
                {
                    throw std::runtime_error("Bad internal state: Did not find piece location for move received");
                    // TODO need to ask the correct board configuration to the server instead of throwing an error
                }
            }
            m_board->playMove(src, dest);
        }
    }
    else if (type == MoveType::InvalidMove)
    {
        // TODO maybe do something here before forwarding it to the render thread
    }
    else
    {
        throw std::runtime_error("PlayThisMove received from the server, this should not happen");
    }

    // Forward it to the render thread
    m_render_thread_socket.send(message);
}

void Player::process_server_player_won(zmqpp::message& message)
{
    m_game_finished = true;

    // Forward it to the render thread
    m_render_thread_socket.send(message);
}

void Player::process_server_game_stopped(zmqpp::message& message)
{
    m_game_stopped = true;

    // Forward it to the render thread
    m_render_thread_socket.send(message);
}

void Player::retrieve_piece_locations(const std::string& board_configuration)
{
    m_piece_locations.clear();
    std::string debug_str = "";

    // Retrieve pieces from the regular board but not Camp cells
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            int cell_index = (row * 8) + col;
            if (board_configuration.at(2 * cell_index + 1) == m_self_player_sign)
            {
                // This cell is occupied by one of my pieces
                m_piece_locations.push_back(cell_index);
                debug_str += " " + std::to_string(cell_index);
            }
        }
    }
    player_msg("Found pieces at locations " + debug_str);
}


void Player::process_graphics_case_clicked(zmqpp::message& message)
{
    CaseClicked c = ConstructObject<CaseClicked>(message);

    int id = stoi(c.getCase());
    player_msg("case clicked " + std::to_string(id));

    bool state0 = (m_memo.first == -1 && m_memo.second == -1);
    bool state1 = (m_memo.first != -1 && m_memo.second == -1);

    bool is_case_owned_by_player = std::find(m_piece_locations.begin(), m_piece_locations.end(), id) != m_piece_locations.end();

    if(state0)
    {
        if(is_case_owned_by_player)
            m_memo.first = id;
    }
    else if(state1)
    {
        if(is_case_owned_by_player)
            m_memo.first = id;
        else
            m_memo.second = id;
    }

    bool state2 = (m_memo.first != -1 && m_memo.second != -1);

    if(state2)
    {
        // Check if the move is valid
        if (m_board->isValidMove(m_memo.first, m_memo.second, m_self_player))
        {
            // Send the move to the server
            zmqpp::message play_message = ConstructMessage<MoveMessage>(MoveType::PlayThisMove, m_memo.first, m_memo.second, m_self_player);

            m_server_thread_socket.send(play_message, true);
        }
        else
        {
            // Inform the render that the move is not valid
            zmqpp::message reject_message = ConstructMessage<MoveMessage>(MoveType::InvalidMove, m_memo.first, m_memo.second, m_self_player);
            m_render_thread_socket.send(reject_message, true);
        }

        // re-init the state
        m_memo.first = -1;
        m_memo.second = -1;
    }
}

void Player::process_graphics_ask_game_status(zmqpp::message& /*message*/)
{
    player_msg("AskGameStatus received from render thread");
    zmqpp::message zmq_message = ConstructMessage<AskGameStatus>();
    m_server_thread_socket.send(zmq_message, true);
}

void Player::process_graphics_stop_game(zmqpp::message& /*message*/)
{
    m_game_stopped = true;

    zmqpp::message message = ConstructMessage<StopGame>("Game stopped by the graphical interface");
    m_server_thread_socket.send(message, true);
}



Player::Player(GameSettings& game_settings) :
    m_zmq_context(),
    m_poller(),
    m_server_thread_socket(m_zmq_context, zmqpp::socket_type::pair),
    m_render_thread_socket(m_zmq_context, zmqpp::socket_type::pair),
    m_server_thread_reactor(),
    m_render_thread_reactor(),
    m_board(new Board()),
    m_game_inited(false),
    m_game_stopped(false),
    m_game_finished(false),
    m_self_player(game_settings.self_player),
    m_connected(false),
    m_piece_locations(),
    m_memo({-1,-1})
{
    m_server_thread_socket.connect(game_settings.server_binding_point);
    m_render_thread_socket.bind(game_settings.render_binding_point);

    // Create the render thread and
    m_render_thread = std::thread(graphics_function,
                                  std::ref(m_zmq_context),
                                  game_settings.render_binding_point);


    // We want to listen to the two sockets at the same time
    m_poller.add(m_server_thread_socket, zmqpp::poller::poll_in);
    m_poller.add(m_render_thread_socket, zmqpp::poller::poll_in);

    using MessageType = MessageWrapper::MessageType;
    using Callback = MessageReactor::Callback;

    // Add callback functions to react to messages received from the server
    Callback process_server_answer_game_status          = std::bind(&Player::process_server_answer_game_status, this, std::placeholders::_1);
    Callback process_server_move_message                = std::bind(&Player::process_server_move_message, this, std::placeholders::_1);
    Callback process_server_player_won                  = std::bind(&Player::process_server_player_won, this, std::placeholders::_1);
    Callback process_server_game_stopped                = std::bind(&Player::process_server_game_stopped, this, std::placeholders::_1);

    m_server_thread_reactor.add(MessageType::AnswerGameStatus, process_server_answer_game_status);
    m_server_thread_reactor.add(MessageType::MoveMessage, process_server_move_message);
    m_server_thread_reactor.add(MessageType::PlayerWon, process_server_player_won);
    m_server_thread_reactor.add(MessageType::GameStopped, process_server_game_stopped);

    // Add callback functions to react to messages received from the render thread
    Callback process_graphics_case_clicked              = std::bind(&Player::process_graphics_case_clicked, this, std::placeholders::_1);
    Callback process_graphics_ask_game_status           = std::bind(&Player::process_graphics_ask_game_status, this, std::placeholders::_1);
    Callback process_graphics_stop_game                 = std::bind(&Player::process_graphics_stop_game, this, std::placeholders::_1);

    m_render_thread_reactor.add(MessageType::CaseClicked, process_graphics_case_clicked);
    m_render_thread_reactor.add(MessageType::AskGameStatus, process_graphics_ask_game_status);
    m_render_thread_reactor.add(MessageType::StopGame, process_graphics_stop_game);


    m_piece_locations.reserve(8); // Reserve space for 8 pieces
    m_self_player_sign = (m_self_player == BoardPlayer::White) ? '+' : '-';
}

Player::~Player()
{
    m_server_thread_socket.close();
    m_render_thread_socket.close();
}

void Player::connect()
{
    if(m_connected)
        return;

    zmqpp::poller poller;   // safer to use a local poller/reactor I think
    MessageReactor reactor;

    poller.add(m_server_thread_socket, zmqpp::poller::poll_in);

    using MessageType = MessageWrapper::MessageType;
    using Callback = MessageReactor::Callback;

    Callback process_server_answer_game_status = std::bind(&Player::process_server_answer_game_status, this, std::placeholders::_1);
    reactor.add(MessageType::AnswerGameStatus, process_server_answer_game_status);

    zmqpp::message message = ConstructMessage<CheckConnectivity>();
    m_server_thread_socket.send(message, true);

    // wait 5 seconds the response of the server
    if(poller.poll(5000))
    {
        if(poller.has_input(m_server_thread_socket))
        {
            zmqpp::message input_message;

            // receive the message
            m_server_thread_socket.receive(input_message);
            m_connected = true;

            // process it
            bool processed = reactor.process_message(input_message);

            if(!processed)
                player_msg("message received from the server thread but no callback were defined for its type");
        }
    }

    if(!m_connected)
        player_msg("Server not responding :(");
}

void Player::loop()
{
    while(!m_connected)
    {
        connect();
    }

    player_msg("Player entering the main loop!");

    while(!m_game_finished && !m_game_stopped)
    {
        zmqpp::message input_message;

        if(m_poller.poll(zmqpp::poller::wait_forever))
        {
            if(m_poller.has_input(m_server_thread_socket))
            {
                // receive the message
                m_server_thread_socket.receive(input_message);

                // Will call the callback corresponding to the message type
                bool processed = m_server_thread_reactor.process_message(input_message);

                if(!processed)
                    player_msg("message received from the server thread but no callback were defined for its type");
            }
            else if(m_poller.has_input(m_render_thread_socket))
            {
                // receive the message
                m_render_thread_socket.receive(input_message);

                // Will call the callback corresponding to the message type
                bool processed = m_render_thread_reactor.process_message(input_message);

                if(!processed)
                    player_msg("message received from the render thread but no callback were defined for its type");
            }
            else
            {
                player_msg("This should not happen, terminating.");
                std::terminate(); // Forcefully terminates the execution
            }
        }
    }

    // Clean up and terminate
    m_render_thread.join();

    player_msg("Terminating.");
}
