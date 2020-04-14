#include "player.hpp"
#include "graphics.hpp"

#include <message/message_utils.hpp>

#include <functional>
#include <iostream>

#include <iostream> // FOR DEBUGGING PURPOSE, TODO REMOVE IT

void Player::process_server_game_init(zmqpp::message& message)
{
    GameInit m = ConstructObject<GameInit>(message);

    // Update internal state
    m_current_player = m.getCurrentPlayer();
    retrieve_piece_locations(m.getConfiguration());

    // Then forward to the render thread
    m_render_thread_socket.send(message);
}


void Player::process_server_board_configuration(zmqpp::message& message)
{
    AnswerBoardConfiguration m = ConstructObject<AnswerBoardConfiguration>(message);
    std::string configuration = m.getConfiguration();
    size_t size = configuration.size();

    retrieve_piece_locations(configuration);

    // Retrieve current player
    if (configuration[size-2] == '+')
    {
        m_current_player = 1;
    }
    else if (configuration[size-2] == '-')
    {
        m_current_player = -1;
    }
    else
    {
        std::string err = "Wrong current player sent in BoardConfiguration message: " + std::string(1, configuration.at(size-2));
        player_msg(err);
        std::terminate();
    }

    // Then forward it to the render thread
    m_render_thread_socket.send(message);
}

void Player::process_server_move_message(zmqpp::message& message)
{
    MoveMessage m = ConstructObject<MoveMessage>(message);
    int move_player = m.getPlayer();
    MoveType type = m.getType();

    if (type == MoveType::MovePlayed)
    {
        if (move_player != m_current_player)
        {
            throw std::runtime_error("Move received from a player not in his turn.");
            // TODO need to ask the correct board configuration to the server instead of throwing an error
        }
        else
        {
            // Update the new location of the piece if it was my move
            if (move_player == m_self_player)
            {
                bool found = false;
                int src, dest;
                convert_move_str(m.getMove(), src, dest);
                for (auto it = m_piece_locations.begin(); it != m_piece_locations.end(); ++it)
                {
                    if (*it == src)
                    {
                        *it = dest;
                        found = true;
                        std::cout << "Updating piece location from " << src << "to" << dest << std::endl;
                    }
                }

                if (!found)
                {
                    throw std::runtime_error("Bad internal state: Did not find piece location for move received");
                    // TODO need to ask the correct board configuration to the server instead of throwing an error
                }
            }
            m_current_player = -m_current_player;
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
    PlayerWon m = ConstructObject<PlayerWon>(message);

    m_game_finished = true;
    //m_current_player = m.getPlayer(); //TODO ??

    // Forward it to the render thread
    m_render_thread_socket.send(message);
}

void Player::process_server_game_stopped(zmqpp::message& message)
{
    //GameStopped m = ConstructObject<GameStopped>(message);

    m_game_finished = true;
    //m_current_player = 0; // TODO ??

    // Forward it to the render thread
    m_render_thread_socket.send(message);
}

void Player::retrieve_piece_locations(const std::string& board_configuration)
{
    m_piece_locations.clear();

    // Retrieve pieces from the regular board but not Camp cells
    for (int i = 1; i <= 8; ++i)
    {
        for (int j = 1; j <= 8; ++j)
        {
            int cell_index = 8 * i + j - 1;
            if (board_configuration.at(2 * cell_index + 1) == m_self_player_sign)
            {
                // This cell is occupied by one of my pieces
                m_piece_locations.push_back(cell_index);
                std::cout << "Found a piece at location " << cell_index << std::endl;
            }
        }
    }
}



void Player::process_graphics_case_clicked(zmqpp::message& message)
{
    CaseClicked c = ConstructObject<CaseClicked>(message);

    int id = stoi(c.getCase());

    bool state0 = (m_memo.first == -1 && m_memo.second == -1);
    bool state1 = (m_memo.first != -1 && m_memo.second == -1);

    throw std::runtime_error("todo");
    bool is_case_own_by_player = true;//std::find(m_piece_locations.begin(), m_piece_locations.end(), id) != m_piece_locations.end();

    if(state0)
    {
        if(is_case_own_by_player)
            m_memo.first = id;
    }
    else if(state1)
    {
        if(is_case_own_by_player)
            m_memo.first = id;
        else
            m_memo.second = id;
    }

    bool state2 = (m_memo.first != -1 && m_memo.second != -1);

    if(state2)
    {
        // envoie le coup au serveur
        std::string move_str = create_move_str(m_memo.first, m_memo.second);
        zmqpp::message play_message = ConstructMessage<MoveMessage>(MoveType::PlayThisMove, move_str, m_self_player);

        m_server_thread_socket.send(play_message);

        // re-init
        m_memo.first = -1;
        m_memo.second = -1;
    }

    std::cout << " '(main thread) case clicked ' " << id << std::endl;
}

void Player::process_graphics_stop_game(zmqpp::message& message)
{
    StopGame m = ConstructObject<StopGame>(message);

    m_game_finished = true;
    m.setPlayer(m_self_player);
    m.setReason("Game stopped by the graphical interface");

    zmqpp::message zmq_message = ConstructMessage<StopGame>(m);
    m_server_thread_socket.send(zmq_message);
}



Player::Player(GameSettings& game_settings) :
    m_zmq_context(),
    m_poller(),
    m_server_thread_socket(m_zmq_context, zmqpp::socket_type::pair),
    m_render_thread_socket(m_zmq_context, zmqpp::socket_type::pair),
    m_server_thread_reactor(),
    m_render_thread_reactor(),
    m_game_finished(false),
    m_game_stopped(false),
    m_self_player(game_settings.self_player),
    m_current_player(0),
    m_memo({-1,-1})
{
    m_server_thread_socket.bind(game_settings.server_binding_point);
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
    Callback process_server_game_init           = std::bind(&Player::process_server_game_init, this, std::placeholders::_1);
    Callback process_server_board_configuration = std::bind(&Player::process_server_board_configuration, this, std::placeholders::_1);
    Callback process_server_move_message        = std::bind(&Player::process_server_move_message, this, std::placeholders::_1);
    Callback process_server_player_won          = std::bind(&Player::process_server_player_won, this, std::placeholders::_1);
    Callback process_server_game_stopped        = std::bind(&Player::process_server_game_stopped, this, std::placeholders::_1);

    m_server_thread_reactor.add(MessageType::GameInit, process_server_game_init);
    m_server_thread_reactor.add(MessageType::AnswerBoardConfiguration, process_server_board_configuration);
    m_server_thread_reactor.add(MessageType::MoveMessage, process_server_move_message);
    m_server_thread_reactor.add(MessageType::PlayerWon, process_server_player_won);
    m_server_thread_reactor.add(MessageType::GameStopped, process_server_game_stopped);

    // Add callback functions to react to messages received from the render thread
    Callback process_graphics_case_clicked = std::bind(&Player::process_graphics_case_clicked, this, std::placeholders::_1);
    Callback process_graphics_stop_game = std::bind(&Player::process_graphics_stop_game, this, std::placeholders::_1);

    m_render_thread_reactor.add(MessageType::CaseClicked, process_graphics_case_clicked);
    m_render_thread_reactor.add(MessageType::StopGame, process_graphics_stop_game);


    m_piece_locations.reserve(8); // Reserve space for 8 pieces
    m_self_player_sign = (m_self_player == 1) ? '+' : '-';
}

Player::~Player()
{
    m_server_thread_socket.close();
    m_render_thread_socket.close();
}

void Player::loop()
{
    player_msg("Player entering the main loop!");

    while(!m_game_finished)
    {
        zmqpp::message input_message;

        if(m_poller.poll(zmqpp::poller::wait_forever))
        {
            if(m_poller.has_input(m_server_thread_socket))
            {
                // receive the message
                m_server_thread_socket.receive(input_message);

                // Will call the callback corresponding to the message type
                m_server_thread_reactor.process_message(input_message);
            }
            else if(m_poller.has_input(m_render_thread_socket))
            {
                // receive the message
                m_render_thread_socket.receive(input_message);

                // Will call the callback corresponding to the message type
                m_render_thread_reactor.process_message(input_message);
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
