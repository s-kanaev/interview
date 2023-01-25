#include "common.hpp"
#include "Server.hpp"
#include "Database.hpp"

#include <boost/network/protocol/http/server.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
// #include <json_spirit.h>
#include <json_spirit_writer_template.h>
#include <json_spirit_reader_template.h>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstring>
#include <cstdio>

namespace http = boost::network::http;
namespace utils = boost::network::utils;

// asynchronous server request handler
struct AsyncRequestHandler {
protected:
    // for use with condition variable of post request
    std::mutex m_mutex;
    typedef boost::iterator_range<char const *> rValue;
    void ConnectionReadCallback(rValue input_range,
                                boost::system::error_code error,
                                std::size_t size,
                                async_server::connection_ptr connection,
                                std::string *result,
                                std::condition_variable *cv)
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        result->append(input_range.begin());
        cv->notify_one();
    }

    void HandlePostRequest(async_server::request const& request,
                           DBRequest *db_request,
                           async_server::connection_ptr connection)
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
            if ((r > 0) && (id >= 0)) {
                _request->id = id;
            } else {
                db_request->request_type = REQUEST_INVALID;
                return;
            }
        }
        std::condition_variable cv;
        std::unique_lock<std::mutex> locker(m_mutex);
        connection->read(boost::bind(&AsyncRequestHandler::ConnectionReadCallback, this, _1, _2, _3, _4, &request_body, &cv));
        cv.wait(locker);
        m_mutex.unlock();
        // decode request_body from json
        //request_body = request.body;
        if (request_body.empty()) {
            db_request->request_type = REQUEST_INVALID;
            return;
        }
        json_spirit::Value mval;
        json_spirit::Object obj;
        bool success;
        success = json_spirit::read_string<std::string, json_spirit::Value>(request_body, mval);
        if (success) {
            if ((mval.type() == json_spirit::obj_type) &&
                (mval.get_obj().size() == 3)) {
                obj = mval.get_obj();
                for (int i = 0; i < obj.size(); ++i) {
                    if (obj[i].name_ == "firstName") {
                        _request->first_name = strdup(obj[i].value_.get_str().data());
                    } else if (obj[i].name_ == "lastName") {
                        _request->last_name = strdup(obj[i].value_.get_str().data());
                    } else if (obj[i].name_ == "birthDate") {
                        _request->birth_date = strdup(obj[i].value_.get_str().data());
                    } else {
                        db_request->request_type = REQUEST_INVALID;
                        if (_request->first_name) free(_request->first_name);
                        if (_request->last_name) free(_request->last_name);
                        if (_request->birth_date) free(_request->birth_date);
                    }
                }
            } else {
                _request->first_name =
                _request->last_name =
                _request->birth_date = NULL;
                _request->id = 0;
            }
        }
    }

    void HandleDeleteRequest(async_server::request const& request,
                             DBRequest *db_request,
                             async_server::connection_ptr connection)
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
    }

    void HandleGetRequest(async_server::request const& request,
                          DBRequest *db_request,
                          async_server::connection_ptr connection)
    {
        db_request->request_type = REQUEST_GET;
        GetRequest *_request = &db_request->any_request.get_request;
        // get path
        std::string request_path = request.destination;
        // retrieve id
        if (request_path == "/users") {
            _request->id = 0;
        } else {
            bigserial_t id;
            int r = sscanf(request_path.data(), "/users/%llu", &id);
            if (r > 0) {
                _request->id = id;
            } else {
                db_request->request_type = REQUEST_INVALID;
            }
        }
    }

public:
    void operator()(async_server::request const& request,
                    async_server::connection_ptr connection)
    {
        DBRequest db_request;
        if (request.method == "POST") {
            HandlePostRequest(request, &db_request, connection);
        } else if (request.method == "DELETE") {
            HandleDeleteRequest(request, &db_request, connection);
        } else if (request.method == "GET") {
            HandleGetRequest(request, &db_request, connection);
        } else {
            db_request.request_type = REQUEST_INVALID;
        }
        // enqueue request to database
        Database::getInstance().QueueRequest(db_request, connection);
        // return
    }
};

// asynchronous server reply part
std::string WriteDBRecordAsJSON(DBRecord *db_record)
{
    json_spirit::Object db_record_obj;
    std::string str;
    db_record_obj.push_back(json_spirit::Pair("id", (boost::int64_t)db_record->id));
    db_record_obj.push_back(json_spirit::Pair("firstName", db_record->first_name));
    db_record_obj.push_back(json_spirit::Pair("lastName", db_record->last_name));
    db_record_obj.push_back(json_spirit::Pair("birthDate", db_record->birth_date));
    str = json_spirit::write_string<json_spirit::Value>(db_record_obj, json_spirit::pretty_print);
    return str;
}

static async_server::response_header common_headers[] = {
    {"Connection", "close"},
    {"Content-Type", "text/plain"},
    {"Content-Length", "0"}
};

void ServerSendReply(DBReply db_reply,
                     async_server::connection_ptr connection)
{
    std::string reply_string("");
    std::string one_record_string("");
    switch (db_reply.Kind()) {
        case REPLY_OK:
            // set reply state
            connection->set_status(async_server::connection::ok);
            reply_string = "200 OK";
            // if there are any db records available - post them
            std::vector<std::shared_ptr<DBRecord>> *db_records;
            db_records = db_reply.Records().get();
            switch (db_records->size()) {
                case 0:
                    break;
                case 1:
                    reply_string.append("\n\n");
                    one_record_string = WriteDBRecordAsJSON(db_records->at(0).get());
                    reply_string.append(one_record_string);
                    break;
                default:
                    reply_string.append("\n\n");
                     // post '['
                    reply_string.append("[\n");
                    // post each of the record
                    for (int i = 0; i < db_records->size(); ++i) {
                        one_record_string = WriteDBRecordAsJSON(db_records->at(i).get());
                        reply_string.append(one_record_string);
                        reply_string.append(", ");
                    }
                    // post ']'
                    reply_string.append("\n]");
                    break;
            }
            break;
        case REPLY_NOT_FOUND:
            // set reply state
            connection->set_status(async_server::connection::not_found);
            reply_string = "404 Not Found";
            break;
        case REPLY_BAD_REQUEST:
            // set reply state
            connection->set_status(async_server::connection::bad_request);
            reply_string = "400 Bad Request";
            break;
    }
    common_headers[2].value = boost::lexical_cast<std::string>(reply_string.length());
    connection->set_headers(boost::make_iterator_range(common_headers, common_headers+2));
    // send the reply
    connection->write(reply_string);
};

// server shutdown
void Signal_INT_TERM_handler(const boost::system::error_code& error,
                             int signal,
                             boost::shared_ptr<async_server> server_instance)
{
    if (!error)
        server_instance->stop();
}

/*
   run server.
   - allocate handler object
   - allocate server object (put its options)
   - set sigint/sigterm signals handlers to stop server
   - call to run() method of server instance
   - interrupt all threads in thread pool
   - join all threads in pool
 */
void RunServer(std::string address_str, std::string port_str)
{
    AsyncRequestHandler request_handler;
    boost::shared_ptr<async_server> _server;

    async_server::options options(request_handler);
    options.address(address_str)
           .port(port_str)
           .thread_pool(threadPool)
           .io_service(iOService);
 //            .reuse_address(true) // FIXME: ???
   _server = boost::shared_ptr<async_server>(
        new async_server(async_server::options(options)));

    // TODO: set SIGINT and SIGTERM handlers
    boost::asio::signal_set signals(*iOService, SIGINT, SIGTERM);
    signals.async_wait(boost::bind(Signal_INT_TERM_handler, _1, _2, _server));

    _server->run(); // it will block

    threadGroup->interrupt_all();
    iOServiceWork.reset();
    iOService->stop();
    threadGroup->join_all();
}
