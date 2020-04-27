#include "GameStatus.hpp"


AnswerGameStatus::AnswerGameStatus() : MessageWrapper()
{

}

AnswerGameStatus::AnswerGameStatus(const std::string& board_config) : MessageWrapper(),
    m_configuration(board_config)
{

}

void AnswerGameStatus::toMessage(zmqpp::message& message)
{
    message << m_configuration;
}

void AnswerGameStatus::fromMessage(zmqpp::message& message)
{
    message >> m_configuration;
}

std::string AnswerGameStatus::getConfiguration() const
{
    return m_configuration;
}

void AnswerGameStatus::setConfiguration(const std::string& configuration)
{
    m_configuration = configuration;
}

