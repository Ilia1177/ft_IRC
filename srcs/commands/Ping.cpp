#include "Ping.hpp"
#include "ReplyHandler.hpp"
#include "reply_codes.hpp"
#include "LogManager.hpp"

#include <sstream>

/************************************************************
*		🥚 CONSTRUCTORS & DESTRUCTOR						*
************************************************************/

Ping::Ping(void) {}

Ping::Ping(const std::string& token) : _token(token) {}

Ping::Ping(const Ping& other)
    : _token(other._token)
{
}

Ping::~Ping(void) {}

/************************************************************
*		➕ OPERATORS											*
************************************************************/

Ping& Ping::operator=(const Ping& other) 
{
    if (this != &other)
    {
    }
    return (*this);
}

/*************************************************************
*		🛠️ FUNCTIONS											*
*************************************************************/

/**
 * @brief checks that there is an origin token
 * 
 * @param s 
 * @param c 
 * @param params 
 * @return ReplyCode 
 */
ReplyCode Ping::check_args(Server& s, Client& c, std::string& params)
{
    (void) s;
    (void) c;
    std::string token;

    std::istringstream iss(params);
    iss >> token;

    LOG_DV_CMD(token);
    if (token.empty())
        return ERR_NOORIGIN;
    return CORRECT_FORMAT;
}

/**
 * @brief sends a PONG message with token provided by PING sender
 * 
 * @param s server
 * @param c client
 */
void			 Ping::execute(Server& s, Client& c)
{
    ReplyHandler& rh = ReplyHandler::get_instance(&s);
    rh.process_response(c, MSG_PING, _token);
}

