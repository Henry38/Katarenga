#include "graphics.hpp"
#include "player.hpp"
#include "utils.hpp"

#include <GLTK/GLFWApplication.h>

#include <iostream>

using MessageType = MessageWrapper::MessageType;

void print_help()
{
    render_msg("h,help for help");
    render_msg("c,click for click in a case");
    render_msg("p,print to print the board");
    render_msg("s,stop for quit");
}

void Graphics::process_main_answer_board_configuration(zmqpp::message& message)
{
    AnswerBoardConfiguration m = ConstructObject<AnswerBoardConfiguration>(message);
    std::string board_configuration = m.getConfiguration();

    render_msg("AnswerBoardConfiguration received");
    render_msg(board_configuration);
}

Graphics::Graphics(zmqpp::context& zmq_context, const std::string& main_thread_binding_point) :
    m_poller(),
    m_main_thread_socket(zmq_context, zmqpp::socket_type::pair),
    m_main_thread_reactor(),
    m_end_game(false)
{
    m_main_thread_socket.connect(main_thread_binding_point);

    // We want to listen to the socket
    m_poller.add(m_main_thread_socket, zmqpp::poller::poll_in);

    using MessageType = MessageWrapper::MessageType;
    using Callback = MessageReactor::Callback;

    // Add callback functions to react to messages received from the main thread
    Callback process_main_answer_board_configuration = std::bind(&Graphics::process_main_answer_board_configuration, this, std::placeholders::_1);

    m_main_thread_reactor.add(MessageType::AnswerBoardConfiguration, process_main_answer_board_configuration);
}

Graphics::~Graphics()
{
    m_main_thread_socket.close();
}

void Graphics::loop()
{
    render_msg("render thread ready to play!");

    print_help();

    while(!m_end_game)
    {
        render_msg("Enter a command: ");

        std::string command = "";
        std::cin >> command;

        if(command == "h" || command == "help")
        {
            print_help();
        }
        else if(command == "c" || command == "click")
        {
            render_msg("Enter your string as the index of the cell '<src_cell_index>' ");

            std::string move_str;
            std::cin >> move_str;

            zmqpp::message message = ConstructMessage<CaseClicked>(move_str);

            // envoie le message (non bloquant)
            bool ret = m_main_thread_socket.send(message, true);

            if(!ret)
                render_msg("(click) error, message not sent");
        }
        else if(command == "p" || command == "print")
        {
            zmqpp::message message = ConstructMessage<AskBoardConfiguration>();

            // envoie le coup (non bloquant)
            bool ret = m_main_thread_socket.send(message, true);

            if(!ret)
                std::cout << "(print) error, message not sent" << std::endl;
            else
            {
                zmqpp::message input_message;

                // wait no more than 5 seconds the coming answer
                if(m_poller.poll(5000))
                {
                    if(m_poller.has_input(m_main_thread_socket))
                    {
                        // receive the message
                        m_main_thread_socket.receive(input_message);

                        // Will call the callback corresponding to the message type
                        bool processed = m_main_thread_reactor.process_message(input_message);

                        if(!processed)
                            render_msg("message received from the main thread but no callback were defined for its type");
                    }
                    else
                    {
                        render_msg("This should not happen, terminating.");
                        std::terminate(); // Forcefully terminates the execution
                    }
                }
                else
                {
                    render_msg("no board configuration received from the main thread");
                }
            }
        }
        else if(command == "s" || command == "stop")
        {
            zmqpp::message message = ConstructMessage<StopGame>("human decide to stop", 0);

            // envoie le message (non bloquant)
            bool ret = m_main_thread_socket.send(message, true);

            if(!ret)
                render_msg("(stop) error, message not sent");
            else
                m_end_game = true;
        }
        else
        {
            render_msg("unknow command '" + command + "'");
        }

//        if ((has_won = board.gameFinished()) != 0)
//        {
//            end_game = true;
//            render_msg("Woah! " << (has_won == 1?"White":"Black") << " player has won the game!"
//                                                                  "");
//        }
    }

    render_msg("Terminating");
}

void graphics_function(zmqpp::context& zmq_context, const std::string& main_thread_binding_point)
{
    Graphics graphic(zmq_context, main_thread_binding_point);

    graphic.loop();
}
