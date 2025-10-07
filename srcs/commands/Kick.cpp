#include "Kick.hpp"

#include "Channel.hpp"
#include "Client.hpp"
#include "Config.hpp"
#include "LogManager.hpp"
#include "ReplyHandler.hpp"
#include "Server.hpp"
#include "consts.hpp"
#include "reply_codes.hpp"
#include "utils.hpp"

#include <sstream>
#include <string>

/*************************************************************
 *		🛠️ UTIL FUNCTIONS											*
 *************************************************************/

static ReplyCode parse_args(std::vector<std::string>& args,
                            std::vector<std::string>* channelNames,
                            std::vector<std::string>* userNames,
                            std::string*              comment)
{
    std::vector<std::string>::iterator it = args.begin();
    std::istringstream                 iss(*it);
    std::string                        token;
    while (std::getline(iss, token, ',')) {
        channelNames->push_back(token);
    }

    ++it;
    if (it == args.end())
        return ERR_NEEDMOREPARAMS;
    iss.clear();
    iss.str(*it);
    while (std::getline(iss, token, ',')) {
        userNames->push_back(token);
    }
    ++it;
    if (it != args.end())
        *comment = *it;

    return RPL_SUCCESS;
}

/************************************************************
 *		📁 CLASS METHODS									*
 ************************************************************/

/**
 * @brief
 * first check for missing arg -> ERR_NEEDMOREPARAMS
 * @param server
 * @param client
 * @param args
 * @return ReplyCode to be used in CommandFactory and potentially generate an error
 */
ReplyCode Kick::check_args(Server& server, Client& client, std::vector<std::string>& args)
{
    (void)server;
    (void)client;
    t_params    channelNames;
    t_params    nicknames;
    std::string comment;
    // ReplyHandler& 	rh = ReplyHandler::get_instance(&server);

    ReplyCode code = parse_args(args, &channelNames, &nicknames, &comment);
    if (code != RPL_SUCCESS) {
        LOG_CMD.warning(TO_STRING(code) + " = parsing error");
        return code;
    }
    (void)comment;
    return RPL_SUCCESS;
}

/************************************************************
 *		🥚 CONSTRUCTORS & DESTRUCTOR						*
 ************************************************************/

Kick::Kick() : _args() {}
Kick::Kick(const std::vector<std::string>& args) : _args(args) {}
Kick::Kick(const Kick& other) : ICommand(), _args(other._args) {}
Kick::~Kick() {}

/************************************************************
 *		➕ OPERATORS											*
 ************************************************************/

Kick& Kick::operator=(const Kick& other)
{
    if (this != &other) {
        _args = other._args;
    }
    return *this;
}

/*************************************************************
 *		🛠️ FUNCTIONS											*
 *************************************************************/

/**
 * @brief Force part a channel
 * loop over channels :
 * check for invalid channel name -> ERR_BADCHANMASK
 * then check for existing channel -> ERR_NOSUCHCHANNEL
 * then check for op privilege -> ERR_CHANOPRIVSNEEDED
 * inner loop over channel members :
 * - then check for user presence -> ERR_USERNOTINCHANNEL
 * - then execute : remove - broadcast + direct notice
 * @param server
 * @param client
 */
void Kick::execute(Server& server, Client& client)
{
    ReplyHandler& rh = ReplyHandler::get_instance(&server);
    t_params      channelNames;
    t_params      nicknames;
    std::string   comment;

    parse_args(_args, &channelNames, &nicknames, &comment);

    for (t_params::iterator chanNamesIt = channelNames.begin(); chanNamesIt != channelNames.end(); ++chanNamesIt) {
        if (!Channel::is_valid_channel_name(*chanNamesIt)) {
            rh.process_response(client, ERR_BADCHANMASK);
            continue;
        }
        std::map<std::string, Channel*>::iterator chanIt = server.channels.find(*chanNamesIt);
        if (chanIt == server.channels.end()) {
            rh.process_response(client, ERR_NOSUCHCHANNEL);
            continue;
        }
        Channel* channel = chanIt->second;
        if (!channel->is_operator(client)) {
            rh.process_response(client, ERR_CHANOPRIVSNEEDED, channel->get_name());
            continue;
        }
        for (t_params::iterator nickIt = nicknames.begin(); nickIt != nicknames.end(); ++nickIt) {
            Client* targetUser = server.find_client_by_nickname(static_cast<const std::string&>(*nickIt));
            if (!channel->is_member(*targetUser)) {
                rh.process_response(client, ERR_USERNOTINCHANNEL);
            } else {
                channel->remove_member(*targetUser);
                std::string messageParam = channel->get_name() + " " + targetUser->get_nickname();
                if (!comment.empty())
                    messageParam.append(" :").append(comment);
                channel->broadcast(server, RPL_KICK, messageParam, &client);
                rh.process_response(*targetUser, RPL_KICK, messageParam, &client);
            }
        }
    }
}
