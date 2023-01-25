#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include "common.hpp"
#include "DBReply.hpp"
#include <boost/network/protocol/http/server.hpp>
#include <cstring>
#include <cstdio>

namespace http = boost::network::http;
namespace utils = boost::network::utils;

// asynchronous server handler struct
struct AsyncRequestHandler;
// asynchronous server type
typedef http::async_server<AsyncRequestHandler> async_server;
// typedef async_server::connection_ptr connection_object;

// function to send reply to client
void ServerSendReply(DBReply db_reply,
                     async_server::connection_ptr connection);

// function to run server
void RunServer(std::string address_str, std::string port_str);

#endif
