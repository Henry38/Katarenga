#include "server.hpp"
#include <common/board/Board.hpp>
#include <common/board/board_utils.hpp>

// Standard Library
#include <string>
#include <unistd.h>


/******* Functions related to player messages *******/

void Server::process_player_check_connectivity(zmqpp::message& message, const GameActor &src_actor)
{
    if(src_actor == GameActor::WhiteClient)
    {
        // check wether the white player is already connected or not
        if(m_connectiviy.first == false)
        {
            // we accept the white player!
            m_connectiviy.first = true;
            sendGameStatus(src_actor);
        }
    }
    else if (src_actor == GameActor::BlackClient)
    {
        // check wether the black player is already connected or not
        if(m_connectiviy.second == false)
        {
            // we accept the black player!
            m_connectiviy.second = true;
            sendGameStatus(src_actor);
        }
    }
    else
    {
        server_msg("Non-supported CheckConnectivity message received from " + to_string(src_actor) + " actor");
    }
}

void Server::process_player_move_message(zmqpp::message& message, const GameActor &src_actor)
{
    MoveMessage move_msg = ConstructObject<MoveMessage>(message);
    MoveType move_type = move_msg.getType();
    if (move_type == MoveType::PlayThisMove)
    {
        BoardPlayer move_player = gameActorToPlayer(src_actor);
        if (move_player == m_board->getCurrentPlayer())
        {
            // That's a move from the current player, handle it
            int src = move_msg.getSource();
            int dest = move_msg.getDestination();

            bool valid_move = m_board->playMove(src, dest); // Returns false if invalid move

            if (valid_move)
            {
                server_msg("player " + to_string(move_player) + " played move " + std::to_string(src) + ":" + std::to_string(dest));

                // The move was accepted, notify players
                move_msg.setType(MoveType::MovePlayed);
                zmqpp::message message = ConstructMessage<MoveMessage>(move_msg);
                sendToBothPlayers(message);

                // Check if the player has won
                if (m_board->isGameFinished())
                {
                    sendWonMessage();
                }
            }
            else
            {
                server_msg("player " + to_string(move_player) + " cannot play move " + std::to_string(src) + ":" + std::to_string(dest));

                // This is not a valid move, reject it
                rejectMove(move_msg, src_actor);
            }
        }
        else
        {
            // That's a move from the other player, reject it
            rejectMove(move_msg, src_actor);
        }
    }
    // Else, the player sent a message of type MovePlayed or InvalidMove, ignore it
}

void Server::process_player_ask_game_status(zmqpp::message& input_message, const GameActor &src_actor)
{
    sendGameStatus(src_actor);
}

void Server::process_player_stop_game(zmqpp::message& message, const GameActor &src_actor)
{
    StopGame m = ConstructObject<StopGame>(message);

    if (src_actor == GameActor::WhiteClient)
    {
        std::string reason = "White player: " + m.getReason();
        zmqpp::message message = ConstructMessage<GameStopped>(reason);
        m_black_player_socket.send(message, true);

        server_msg(reason);
        m_connectiviy.first = false;
    }
    else if (src_actor == GameActor::BlackClient)
    {
        std::string reason = "Black player: " + m.getReason();
        zmqpp::message message = ConstructMessage<GameStopped>(reason);
        m_white_player_socket.send(message, true);

        server_msg(reason);
        m_connectiviy.second = false;
    }else
    {
        server_msg("Non-supported StopGame message received from " + to_string(src_actor) + " actor.");
    }

    // We do not treat any more message and quit
    m_game_stopped = true;
}




void Server::rejectMove(MoveMessage& move_msg, const GameActor &actor)
{
    move_msg.setType(MoveType::InvalidMove);

    zmqpp::message m = ConstructMessage<MoveMessage>(move_msg);
    sendToActor(m, actor);
}

void Server::sendWonMessage()
{
    zmqpp::message message = ConstructMessage<PlayerWon>(m_board->whoWon());
    sendToBothPlayers(message);
}

void Server::sendGameStatus(const GameActor& player)
{
    zmqpp::message zmq_message = ConstructMessage<AnswerGameStatus>(m_board->getBoardConfiguration());
    sendToActor(zmq_message, player);
}

void Server::sendToBothPlayers(zmqpp::message& message)
{
    zmqpp::message white_message = message.copy();
    m_white_player_socket.send(white_message, true);

    zmqpp::message black_message = message.copy();
    m_black_player_socket.send(black_message, true);
}

void Server::sendToActor(zmqpp::message& message, const GameActor& actor)
{
    if (actor == GameActor::WhiteClient)
    {
        m_white_player_socket.send(message, true);
    }
    else if (actor == GameActor::BlackClient)
    {
        m_black_player_socket.send(message, true);
    }
    else
    {
        server_msg("Sending messages to " + to_string(actor) + " is not supported");
    }
}



/******* Functions related to command line *******/

void Server::stop_the_game()
{
    zmqpp::message message = ConstructMessage<StopGame>("Game is closed by the server");
    sendToBothPlayers(message);

    m_game_stopped = true;

    server_msg("Game closed");
}

void Server::process_command_line(const std::string& command)
{
    if(command.size() == 0)
        return;

    // all available options here
    static std::list<std::string> options = {"help", "h", "print", "p", "move", "m", "close", "c", "reach", "r"};

    std::string option_found = "";

    // try to find the option matching the command
    for (const std::string& option : options)
    {
        size_t option_size = option.size();

        if(command.size() >= option_size && command.substr(0, option_size) == option) {
            option_found = option;
            break;
        }
    }

    if(option_found.size() == 0) {
        server_msg("Unknow command '" + command + "'");
        return;
    }

    if(option_found == "h" || option_found == "help")
    {
        server_msg("h,help for help");
        server_msg("p,print to print the board");
        server_msg("move, play a move (format like 'move <player>:src:dst')");
        server_msg("c,close to close the server");
    }
    else if(option_found == "p" || option_found == "print")
    {
        std::string board_configuration = format_board(m_board->getBoardConfiguration());
        server_msg(board_configuration);
    }
    else if(option_found == "m" || option_found == "move")
    {
        int player = 0;
        int src_index = -1;
        int dst_index = -1;

        try {
            std::string token;

            std::string str = command;
            str.erase(0, option_found.size() + 1);

            token = str.substr(0, str.find(":"));
            str.erase(0, token.size() + 1);

            player = std::stoi(token);

            token = str.substr(0, str.find(":"));
            str.erase(0, token.size() + 1);

            src_index = std::stoi(token);
            dst_index = std::stoi(str);
        }
        catch(...)
        {
            server_msg("cannot parse move command '" + command + "'");
            return;
        }

        // emulate a move received from the player
        BoardPlayer bplayer = (player == 1) ? BoardPlayer::White : BoardPlayer::Black;
        zmqpp::message message = ConstructMessage<MoveMessage>(MoveType::PlayThisMove, src_index, dst_index, bplayer);
        GameActor actor = (player == 1) ? GameActor::WhiteClient : GameActor::BlackClient;
        process_player_move_message(message, actor);
    }
    else if(option_found == "c" || option_found == "close")
    {
        stop_the_game();
    }
    else if(option_found == "r" || option_found == "reach")
    {
        int src_index = -1;
        std::string str = command;
        str.erase(0, option_found.size() + 1);
        src_index = std::stoi(str);
        std::string s = std::to_string(src_index) + " -> ";

        for (int dest : m_board->getReachableCells(src_index))
        {
             server_msg(s + std::to_string(dest));
        }
    }
    else
    {
        server_msg("Unknow command '" + command + "'");
    }
}



Server::Server(ServerInfo &server_info) :
    m_zmq_context(),
    m_poller(),
    m_white_player_socket(m_zmq_context, zmqpp::socket_type::pair),
    m_black_player_socket(m_zmq_context, zmqpp::socket_type::pair),
    m_white_player_reactor(),
    m_black_player_reactor(),
    m_board(new Board()),
    m_game_stopped(false),
    m_connectiviy({false,false})
{
    m_white_player_socket.bind(server_info.white_binding_point);
    m_black_player_socket.bind(server_info.black_binding_point);

    // Listen to both players and std::cin
    m_poller.add(m_white_player_socket, zmqpp::poller::poll_in);
    m_poller.add(m_black_player_socket, zmqpp::poller::poll_in);
    m_poller.add(STDIN_FILENO, zmqpp::poller::poll_in);

    using Callback = MessageReactor::Callback;

    /* Create the function callback here and add them to the white_player_reactor */
    Callback process_white_check_connectivity      = std::bind(&Server::process_player_check_connectivity, this, std::placeholders::_1, GameActor::WhiteClient);
    Callback process_white_ask_game_status         = std::bind(&Server::process_player_ask_game_status, this, std::placeholders::_1, GameActor::WhiteClient);
    Callback process_white_move_message            = std::bind(&Server::process_player_move_message, this, std::placeholders::_1, GameActor::WhiteClient);
    Callback process_white_stop_game               = std::bind(&Server::process_player_stop_game, this, std::placeholders::_1, GameActor::WhiteClient);

    m_white_player_reactor.add(MessageWrapper::MessageType::CheckConnectivity, process_white_check_connectivity);
    m_white_player_reactor.add(MessageWrapper::MessageType::AskGameStatus, process_white_ask_game_status);
    m_white_player_reactor.add(MessageWrapper::MessageType::MoveMessage, process_white_move_message);
    m_white_player_reactor.add(MessageWrapper::MessageType::StopGame, process_white_stop_game);

    /* Create the function callback here and add them to the black_player_reactor */
    Callback process_black_check_connectivity      = std::bind(&Server::process_player_check_connectivity, this, std::placeholders::_1, GameActor::BlackClient);
    Callback process_black_ask_game_status         = std::bind(&Server::process_player_ask_game_status, this, std::placeholders::_1, GameActor::BlackClient);
    Callback process_black_move_message            = std::bind(&Server::process_player_move_message, this, std::placeholders::_1, GameActor::BlackClient);
    Callback process_black_stop_game               = std::bind(&Server::process_player_stop_game, this, std::placeholders::_1, GameActor::BlackClient);

    m_black_player_reactor.add(MessageWrapper::MessageType::CheckConnectivity, process_black_check_connectivity);
    m_black_player_reactor.add(MessageWrapper::MessageType::AskGameStatus, process_black_ask_game_status);
    m_black_player_reactor.add(MessageWrapper::MessageType::MoveMessage, process_black_move_message);
    m_black_player_reactor.add(MessageWrapper::MessageType::StopGame, process_black_stop_game);
}

Server::~Server()
{
    m_white_player_socket.close();
    m_black_player_socket.close();
}

void Server::new_game()
{
    m_board->initBoard(generateBoardCellTypes());
    m_game_stopped = false;

    server_msg("New game created");
}

void Server::loop()
{
    server_msg("Starting main loop of the game");

    while(!m_board->isGameFinished() && !m_game_stopped)
    {
        // TODO at some point we'll need to have a heartbeat of the two players
        // (or a mean to check whether they are still connected)
        // and stop the game when one player does not respond anymore
        if(m_poller.poll(zmqpp::poller::wait_forever))
        {
            if(m_poller.has_input(m_white_player_socket))
            {
                zmqpp::message input_message;

                // receive the message
                m_white_player_socket.receive(input_message);

                // Will call the callback corresponding to the message type
                bool processed = m_white_player_reactor.process_message(input_message);

                if(!processed)
                    server_msg("Message received from one of the player socket but no callback were defined to handle it");
            }
            else if(m_poller.has_input(m_black_player_socket))
            {
                zmqpp::message input_message;

                // receive the message
                m_black_player_socket.receive(input_message);

                // Will call the callback corresponding to the message type
                bool processed = m_black_player_reactor.process_message(input_message);

                if(!processed)
                    server_msg("Message received from one of the player socket but no callback were defined to handle it");
            }
            else if(m_poller.has_input(STDIN_FILENO))
            {
                std::string command;
                std::getline(std::cin, command);

                // process the command read from std::cin
                process_command_line(command);
            }
        }
    }
}
