#include "common.hpp"
#include "Server.hpp"
#include "Database.hpp"
#include <boost/network/protocol/http/server.hpp>
#include <json_spirit.h>
#include <cstring>
#include <cstdio>

namespace http = boost::network::http;
namespace utils = boost::network::utils;

// asynchronous server request handler
struct AsyncRequestHandler {
    void HandlePostRequest(async_server::request const& request,
                           DBRequest *db_request)
    {
        db_request->request_type = REQUEST_POST;
        PostRequest *_request = &db_request->any_request.post_request;
        _request->first_name = _request->last_name = _request->birth_date = NULL;
        _request->id = 0;
        std::string request_body;
        // get path
        std::string request_path = request.destination;
        if (request_path == "/users") {
            _request->id = 0;
        } else {
            bigserial_t id;
            int r = sscanf(request_path.data(), "/users/%llu", &id);
            if (r > 0) {
                if (id >= 0) _request->id = id;
                else {
                    db_request->request_type = REQUEST_INVALID;
                    return;
                }
            } else {
                db_request->request_type = REQUEST_INVALID;
                return;
            }
        }
        // decode request_body from json
        request_body = request.body;
        json_spirit::Value mval;
        json_spirit::Object obj;
        bool success;
        success = json_spirit::read(request_body, mval);
        if (success) {
            obj = mval.get_obj();
            for (int i = 0; i < obj.size(); ++i) {
                if (obj[i].name_ == "firstName") {
                    _request->first_name = strdup(obj[i].value_.get_str().data());
                } else if (obj[i].name_ == "lastName") {
                    _request->last_name = strdup(obj[i].value_.get_str().data());
                } else if (obj[i].name_ == "birthDate") {
                    _request->birth_date = strdup(obj[i].value_.get_str().data());
                }
            }
        }
    };

    void HandleDeleteRequest(async_server::request const& request,
                             DBRequest *db_request)
    {
        db_request->request_type = REQUEST_DELETE;
        DeleteRequest *_request = &db_request->any_request.delete_request;
        // get path
        std::string request_path = request.destination;
        bigserial_t id;
        // retrieve id
        int r = sscanf(request_path.data(), "/users/%llu", &id);
        if (r > 0 && id > 0) {
            _request->id = id;
        } else {
            db_request->request_type = REQUEST_INVALID;
        }
    };

    void HandleGetRequest(async_server::request const& request,
                          DBRequest *db_request)
    {
        db_request->request_type = REQUEST_GET;
        GetRequest *_request = &db_request->any_request.get_request;
        // get path
        std::string request_path = request.destination;
        // retrieve id
        if (request_path == "/users") {
            _request->id = 0;
        } {
            bigserial_t id;
            int r = sscanf(request_path.data(), "/users/%llu", &id);
            if (r > 0) {
                _request->id = id;
            } else {
                db_request->request_type = REQUEST_INVALID;
            }
        }
    };

    void operator()(async_server::request const& request,
                     async_server::connection_ptr connection)
    {
        DBRequest db_request;
        std::string request_path;
        if (request.method == "POST") {
            HandlePostRequest(request, &db_request);
        } else if (request.method == "DELETE") {
            HandleDeleteRequest(request, &db_request);
        } else if (request.method == "GET") {
            HandleGetRequest(request, &db_request);
        } else db_request.request_type = REQUEST_INVALID;
        // enqueue request to database
        Database::getInstance().QueueRequest(db_request, connection);
        // return
    };
};
