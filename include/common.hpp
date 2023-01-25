#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <boost/network/protocol/http/server.hpp>
#include <boost/network/utils/thread_pool.hpp>
#include <boost/fusion/adapted/std_pair.hpp>

typedef unsigned long long int bigserial_t;
typedef long long int fake_bigserial_t;

namespace http = boost::network::http;
namespace utils = boost::network::utils;

typedef enum _RequestType {
    REQUEST_POST,
    REQUEST_DELETE,
    REQUEST_GET,
    REQUEST_INVALID
} RequestType;

typedef struct _PostRequest {
    bigserial_t id; // -1 if not set
    char *first_name, *last_name, *birth_date; // NULL if not set
} PostRequest;

typedef struct _DeleteRequest {
    bigserial_t id; // should be no less than nil
} DeleteRequest;

typedef struct _GetRequest {
    bigserial_t id; // -1 to retrieve all records
} GetRequest;

// request to db structure
typedef struct _DBRequest {
    RequestType request_type;
    union {
        PostRequest post_request;
        DeleteRequest delete_request;
        GetRequest get_request;
    } any_request;
} DBRequest;

typedef enum _DBReplyKind {
    REPLY_OK,           // 200
    REPLY_BAD_REQUEST,  // 400
    REPLY_NOT_FOUND     // 404
} DBReplyKind;

class DBRecord {
public:
    bigserial_t id;
    std::string first_name, last_name, birth_date;
};

// typedef struct _DBRecord {
    //     int id;
//     char *first_name, *last_name, *birth_date;
// } DBRecord;

struct AsyncRequestHandler;
typedef http::async_server<AsyncRequestHandler> async_server;
// typedef async_server::connection_ptr connection_object;

extern boost::network::utils::thread_pool threadPool;

#endif