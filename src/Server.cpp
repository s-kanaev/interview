#include "common.hpp"
#include "Server.hpp"
#include "Database.hpp"

#include <boost/network/protocol/http/server.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <json_spirit_writer_template.h>
#include <json_spirit_reader_template.h>
#include <boost/thread/mutex.hpp>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>

namespace http = boost::network::http;
namespace utils = boost::network::utils;

// flag showing the servir is running ant its mutex
boost::mutex _server_running_mutex;
bool _server_running = false;

// asynchronous server request handler
struct AsyncRequestHandler {
protected:
    // let's make a thread safe json read
    boost::mutex m_json_mutex;
    // typedef to decrease line length
    typedef boost::iterator_range<char const *> rValue;

    // read from connection for post request
    void ConnectionReadCallback(rValue input_range,
                                boost::system::error_code error,
                                std::size_t size,
                                async_server::connection_ptr connection,
                                std::string *result,
                                boost::condition_variable *cv,
                                boost::mutex *mutex,
                                int *waiting_length)
    {
        boost::unique_lock<boost::mutex> locker(*mutex);
        result->append(input_range.begin());
        //printf("%p waiting_length = %d, received = %d\n",
        //       waiting_length, *waiting_length, strlen(input_range.begin()));
        (*waiting_length) -= strlen(input_range.begin());
        if (*waiting_length <= 0) {
            cv->notify_one();
        }
    }

    // thread safe json reader
    bool json_read_thread_safe(const std::string& s, json_spirit::Value& value )
    {
        boost::unique_lock<boost::mutex> scoped_lock(m_json_mutex);
        return json_spirit::read_string<std::string, json_spirit::Value>(s, value);
    };

    // post request handler
    void HandlePostRequest(async_server::request const& request,
                           DBRequest *db_request,
                           async_server::connection_ptr connection)
    {
        db_request->request_type = REQUEST_POST;
        PostRequest *_request = &db_request->any_request.post_request;
        _request->first_name = _request->last_name = _request->birth_date = NULL;
        _request->id = 0;
        std::string request_body;

        // get request path and an id
        std::string request_path = request.destination;
        if (request_path == "/users") {
            _request->id = 0;
        } else {
            bigserial_t id;
            int r = sscanf(request_path.data(), "/users/%llu", &id);
            if ((r > 0) && (id >= 0)) {
                _request->id = id;
            } else if (strlen(request_path.data()) > strlen("/users")) {
                // it is not a valid request - it is neither /users nor /users/id
                db_request->request_type = REQUEST_INVALID;
                return;
            } else {
                // so, we couldn't retrieve and id -> it's an invalid request either
                db_request->request_type = REQUEST_INVALID;
                return;
            }
        }

        // conditional to wait for request data receiver
        boost::condition_variable cv;
        // for use with condition variable of post request
        boost::mutex _mutex;
        boost::unique_lock<boost::mutex> locker(_mutex);
        int waiting_length = 0;

        // get request data length
        async_server::request::vector_type::iterator it;
        for (it = request.headers.begin(); it != request.headers.end(); ++it) {
            if (0 == it->name.compare("Content-Length")) {
                sscanf(it->value.c_str(), "%d", &waiting_length);
                break;
            }
        }
        //printf("Waiting for: %d bytes\n", waiting_length);
        if (waiting_length == 0) {
            db_request->request_type = REQUEST_INVALID;
            return;
        }

        // let's read supplementary data
        connection->read(
                    boost::bind(
                        &AsyncRequestHandler::ConnectionReadCallback,
                        this, _1, _2, _3, _4,
                        &request_body, &cv, &_mutex, &waiting_length));

        cv.wait(locker, [&]() {
                    return (waiting_length <= 0) ||
                           boost::this_thread::interruption_requested();
                });
        locker.unlock();

        // decode request_body from json
        if (request_body.empty()) {
            db_request->request_type = REQUEST_INVALID;
            return;
        }
        json_spirit::Value mval;
        json_spirit::Object obj;
        bool success;
        success = json_read_thread_safe(request_body, mval);
        //success = json_spirit::read_string<std::string, json_spirit::Value>(request_body, mval);
        if (success) {
            // valid request should have exactly 3 values
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
                        // neither value of the above? it is invalide request, then
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

    // delete request handler
    void HandleDeleteRequest(async_server::request const& request,
                             DBRequest *db_request,
                             async_server::connection_ptr connection)
    {
        db_request->request_type = REQUEST_DELETE;
        DeleteRequest *_request = &db_request->any_request.delete_request;
        // get path and id
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
            if ((r > 0) && (id >= 0)) {
                _request->id = id;
            } else if (strlen(request_path.data()) > strlen("/users")) {
                // it is not a valid request - it is neither /users nor /users/id
                db_request->request_type = REQUEST_INVALID;
                return;
            } else {
                // so, we couldn't retrieve and id -> it's an invalid request either
                db_request->request_type = REQUEST_INVALID;
                return;
            }
        }
    }

public:
    void operator()(async_server::request const& request,
                    async_server::connection_ptr connection)
    {
        DBRequest db_request;
        // call to appropriate method handler
        if (request.method == "POST") {
            HandlePostRequest(request, &db_request, connection);
        } else if (request.method == "DELETE") {
            HandleDeleteRequest(request, &db_request, connection);
        } else if (request.method == "GET") {
            HandleGetRequest(request, &db_request, connection);
        } else {
            // or think of this request as invalid
            db_request.request_type = REQUEST_INVALID;
        }
        // enqueue request to database
        Database::getInstance().QueueRequest(db_request, connection);
        // return
    }
};

// asynchronous server reply part
// put json'ed value to string
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

// reply connection headers
static async_server::response_header common_headers[] = {
    {"Connection", "close"},        // close connection after the transaction
    {"Content-Type", "text/plain"}, // it's a plain text within the message
    {"Content-Length", "0"}         // lengs of the message - we will fill it in later
};

// send reply to client
void ServerSendReply(DBReply db_reply,
                     async_server::connection_ptr connection)
{
    // lock _server_running flag to prevent stop in mid of request sending
    boost::unique_lock<boost::mutex> server_running_lock(_server_running_mutex);
    if (!_server_running) {
        return;
    }
    // full reply string
    std::string reply_string("");
    // single db record for the reply
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
    // fill in content length to header
    common_headers[2].value = boost::lexical_cast<std::string>(reply_string.length());
    // set this connection headers
    connection->set_headers(boost::make_iterator_range(common_headers, common_headers+2));
    // send the reply
    connection->write(reply_string);
}

// server shutdown
void Signal_INT_TERM_handler(const boost::system::error_code& error,
                             int signal,
                             boost::shared_ptr<async_server> server_instance)
{
    // just stop it if no error dispatched
    if (!error) {
        printf("Stopping server\n");
        boost::unique_lock<boost::mutex> server_running_lock(_server_running_mutex);
        _server_running = false;
        server_running_lock.unlock();
        server_instance->stop();
    }
}

/*
   run server:
   - allocate handler object
   - allocate server object (put its options)
   - set sigint/sigterm signals handlers to stop server
   - call to run() method of server instance
   - interrupt all threads in thread pool
   - stop io_service
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
           .io_service(iOService)
           .reuse_address(true);
   _server = boost::shared_ptr<async_server>(
        new async_server(async_server::options(options)));

    boost::asio::signal_set _signals(*iOService, SIGINT, SIGTERM);
    _signals.async_wait(boost::bind(Signal_INT_TERM_handler, _1, _2, _server));

    boost::unique_lock<boost::mutex> server_running_lock(_server_running_mutex);
    _server_running = true;
    server_running_lock.unlock();
    _server->run(); // it will block

    threadGroup->interrupt_all();

    iOService->stop();

    threadGroup->join_all();
}
