#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include "common.hpp"
#include "DBReply.hpp"
#include <boost/network/protocol/http/server.hpp>
#include <json_spirit.h>
#include <cstring>
#include <cstdio>

namespace http = boost::network::http;
namespace utils = boost::network::utils;

struct AsyncRequestHandler;
typedef http::async_server<AsyncRequestHandler> async_server;
// typedef async_server::connection_ptr connection_object;

void ServerSendReply(DBReply db_reply,
                     async_server::connection_ptr connection);

#endif