#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include "Database.hpp"
#include "common.hpp"
#include <boost/network/protocol/http/server.hpp>
#include <json_spirit.h>
#include <cstring>
#include <cstdio>

namespace http = boost::network::http;
namespace utils = boost::network::utils;

// TODO: define connection_object as async_server::connection_ptr
struct connection_object;

struct AsyncRequestHandler;
http::async_server<AsyncRequestHandler> async_server;

// asynchronous server request handler
struct AsyncRequestHandler {
    void HandlePostRequest(async_server::request const& request,
                           DBRequest *db_request)
    {
        db_request.request_type = REQUEST_POST;
        PostRequest *_request = &db_request->any_request.post_request;
        _request->first_name = _request->last_name = _request->birth_date = NULL;
        _request->id = 0;
        std::string request_body;
        // TODO: get path
        request_path = request.destination;
        if (request_type == "/users") {
            _request->id = 0;
        } else {
            bigserial_t id;
            int r = std::sscanf(request_type.data(), "/users/%llu", &id);
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
        std::string::iterator body_it = request_body.begin();
        std::string::iterator body_end = request_body.end();
        json_spirit::Value mval;
        json_spirit::Object obj;
        bool success;
        success = json_spirit::read(body_it, body_end, mval);
        while (success) {
            obj = mval.get_obj();
            for (int i = 0; i < obj.size(); ++i) {
                if (obj[i].name_ == "firstName") {
                    _request->first_name = std::strdup(obj[i].value_.get_str());
                } else if (obj[i].name_ == "lastName") {
                    _request->last_name = std::strdup(obj[i].value_.get_str());
                } else if (obj[i].name_ == "birthDate") {
                    _request->birth_date = std::strdup(obj[i].value_.get_str());
                }
            }
            success = json_spirit::read(body_it, body_end, mval);
        }
    }
    void HandleDeleteRequest(async_server::request const& request,
                             DBRequest *db_request)
    {
        db_request->request_type = REQUEST_DELETE;
        DeleteRequest *_request = &db_request.any_request.delete_request;
        // get path
        request_path = request.destination;
        bigserial_t id;
        // retrieve id
        int r = std::sscanf(request_path.data(), "/users/%llu", &id);
        if (r > 0 && id > 0) {
            _request->id = id;
        } else {
            db_request->request_type = REQUEST_INVALID;
        }
    }
    void HandleGetRequest(async_server::request const& request,
                          DBRequest *db_request)
    {
        db_request.request_type = REQUEST_GET;
        GetRequest *_request = &db_request.any_request.get_request;
        // get path
        request_path = request.destination;
        // retrieve id
        if (request_path == "/users") {
            _request->id = 0;
        } {
            int r = std::sscanf(request_path.data(), "/users/%llu", &id);
            if (r > 0) {
                _request->id = id;
            } else {
                db_request->request_type = REQUEST_INVALID;
            }
        }
    }

    void opertator()(async_server::request const& request,
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
        Database::get_instance().QueueRequest(db_request, connection);
        // return
    };
};

struct AsyncReplyHandler {
    void WriteDBRecordAsJSON(std::string &str, DBRecord *db_record)
    {
        json_spirit::Object db_record_obj;
        db_record_obj.push_back(json_spirit::Pair("id", db_record->id));
        db_record_obj.push_back(json_spirit::Pair("firstName", db_record->first_name));
        db_record_obj.push_back(json_spirit::Pair("lastName", db_record->last_name));
        db_record_obj.push_back(json_spirit::Pair("birthDate", db_record->birth_date));
        json_spirit::write(db_record_obj, str, json_spirit::pretty_print);
    }

    void operator()(DBReply *db_reply,
                    async_server::connection_ptr connection)
    {
        std::string reply_string("");
        std::string one_record_string("");
        switch (db_reply->Kind()) {
            case REPLY_OK:
                // set reply state
                connection->set_status(async_server::connection::ok);
                // if there are any db records available - post them
                std::vector<std::shared_ptr<DBRecord>> *db_records = db_reply->Records();
                switch (db_records->size()) {
                    case 0:
                        break;
                    case 1:
                        WriteDBRecordAsJSON(reply_string, db_records()->at(0).get());
                        break;
                    default:
                        // post '['
                        reply_string.append("[");
                        // post each of the record
                        for (std::vector<std::shared_ptr<DBReply>>::iterator it = db_records->begin();
                             it != db_records->end();
                             ++it) {
                            WriteDBRecordAsJSON(one_record_string, (*it).get());
                            reply_string.append(one_record_string);
                        }
                        // post ']'
                        reply_string.append("]");
                        break;
                }
                break;
            case REPLY_NOT_FOUND:
                // set reply state
                connection->set_status(async_server::connection::not_found);
                break;
            case REPLY_BAD_REQUEST:
                // set reply state
                connection->set_status(async_server::connection::bad_request);
                break;
        }
        // send the reply
        connection->write(reply_string);
    };
};

//  should be singleton ?
class Server {
public:
    Server();
    ~Server();

protected:
private:
};

#endif