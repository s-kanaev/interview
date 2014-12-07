#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <boost/network/protocol/http/server.hpp>
#include <boost/network/utils/thread_pool.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/fusion/adapted/std_pair.hpp>

// bigserial database type definition (should be only greater than nil)
typedef unsigned long long int bigserial_t;

namespace http = boost::network::http;
namespace utils = boost::network::utils;

// request type enumeration
typedef enum _RequestType {
    REQUEST_POST,
    REQUEST_DELETE,
    REQUEST_GET,
    REQUEST_INVALID
} RequestType;

// POST request descriptor
typedef struct _PostRequest {
    bigserial_t id; // 0 if not set
    char *first_name, *last_name, *birth_date; // NULL if not set
} PostRequest;

// DELETE request descriptor
typedef struct _DeleteRequest {
    bigserial_t id; // should be no less than nil
} DeleteRequest;

// GET request descriptor
typedef struct _GetRequest {
    bigserial_t id; // 0 to retrieve all records
} GetRequest;

// unified request descriptor
typedef struct _DBRequest {
    RequestType request_type;
    union {
        PostRequest post_request;
        DeleteRequest delete_request;
        GetRequest get_request;
    } any_request;
} DBRequest;

// kind of reply from db
typedef enum _DBReplyKind {
    REPLY_OK,           // 200
    REPLY_BAD_REQUEST,  // 400
    REPLY_NOT_FOUND     // 404
} DBReplyKind;

// database record descriptor for use with db reply
class DBRecord {
public:
    bigserial_t id;
    std::string first_name, last_name, birth_date;
};

// io service for thread pool
extern boost::shared_ptr<boost::asio::io_service> iOService;
extern boost::shared_ptr<boost::asio::io_service::work> iOServiceWork;
// thread group for thread pool
extern boost::shared_ptr<boost::thread_group> threadGroup;

// thread pool
extern boost::shared_ptr<boost::network::utils::thread_pool> threadPool;

#endif
