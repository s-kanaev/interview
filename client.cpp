#include <boost/range/iterator_range.hpp>
#include <boost/network/protocol/http/client.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <string>
#include <unistd.h>
#include <mutex>
#include <condition_variable>

namespace http = boost::network::http;

using namespace std;

typedef unsigned long long int bigserial_t;
typedef boost::iterator_range<char const *> rValue;

string host;
string port;

std::mutex counters_mutex;
std::mutex reply_mutex;
unsigned int tests_success = 0;
unsigned int tests_failed = 0;

void response_callback_with_reply(boost::iterator_range<char const *> const &a,
                                  boost::system::error_code const &error_code,
                                  std::string *reply,
                                  std::condition_variable *cv)
{
    if (!error_code) {
        std::unique_lock<std::mutex> scoped_lock(reply_mutex);
        reply->append(a.begin());
        //cout << a.begin();
    } else {
        if (error_code == boost::asio::error::eof) {
            // notify request sender through condition variable
            cv->notify_all();
        }
    }
}

void do_get_request(bool auto_test = false,
                    bigserial_t _id = 0,
                    string _check_for_it = string(""))
{
    boost::shared_ptr<boost::asio::io_service> _io_service;
    _io_service = boost::make_shared<boost::asio::io_service>();
    bigserial_t id;
    std::string reply = "";
    if (!auto_test) {
        cout << "Type in id to get (0..MAX(ull)): ";
        cin >> id;
    } else {
        id = _id;
    }

    // create client
    http::client _client(http::client::options().io_service(_io_service));
    // create request string
    string uri = "http://";
    uri.append(host);
    uri.append(":");
    uri.append(port);
    uri.append("/users");
    if (id > 0) {
        uri.append("/");
        uri.append(to_string(id));
    }

    // create request object
    http::client::request _request(uri);

    // create response object
    http::client::response _response;
    std::condition_variable cv;
    std::unique_lock<std::mutex> reply_lock(reply_mutex);
    _response = _client.get(_request, boost::bind(response_callback_with_reply, _1, _2, &reply, &cv));
    cv.wait(reply_lock);

    while (!boost::network::http::ready(_response));

    if (auto_test) {
        cout << "GET request: '" << uri << "'" << endl;
        cout << "Response:" << endl
             << reply << endl;
        // check if _check_for_it is inside reply
        std::unique_lock<std::mutex> scoped_lock(counters_mutex);
        if (string::npos != reply.find(_check_for_it)) {
            ++tests_success;
            cout << "--- SUCCESS ---" << endl;
        } else {
            ++tests_failed;
            cout << "--- FAIL ---" << endl;
        }
    } else {
        cout << "GET request: '" << uri << "'" << endl;
        cout << "Response:" << endl
             << reply << endl;
    }

    _io_service->stop();
    _io_service.reset();
}

void do_delete_request(bool auto_test = false,
                       bigserial_t _id = 0,
                       string _check_for_it = string(""))
{
    boost::shared_ptr<boost::asio::io_service> _io_service;
    _io_service = boost::make_shared<boost::asio::io_service>();
    bigserial_t id;
    std::string reply = "";
    if (!auto_test) {
        cout << "Type in id to delete (0..MAX(ull)): ";
        cin >> id;
    } else {
        id = _id;
    }

    // create client
    http::client _client(http::client::options().io_service(_io_service));
    // create request string
    string uri = "http://";
    uri.append(host);
    uri.append(":");
    uri.append(port);
    uri.append("/users");
    if (id > 0) {
        uri.append("/");
        uri.append(to_string(id));
    }

    // create request object
    http::client::request _request(uri);

    // create response object
    http::client::response _response;
    std::condition_variable cv;
    std::unique_lock<std::mutex> reply_lock(reply_mutex);
    _response = _client.delete_(_request, boost::bind(response_callback_with_reply, _1, _2, &reply, &cv));
    cv.wait(reply_lock);

    while (!boost::network::http::ready(_response));

    if (auto_test) {
        cout << "DELETE request: '" << uri << "'" << endl;
        cout << "Response:" << endl
             << reply << endl;
        // check if _check_for_it is inside reply
        std::unique_lock<std::mutex> scoped_lock(counters_mutex);
        if (string::npos != reply.find(_check_for_it)) {
            ++tests_success;
            cout << "--- SUCCESS ---" << endl;
        } else {
            ++tests_failed;
            cout << "--- FAIL ---" << endl;
        }
    } else {
        cout << "DELETE request: '" << uri << "'" << endl;
        cout << "Response:" << endl
             << reply << endl;
    }

    _io_service->stop();
    _io_service.reset();
}

void do_post_request(bool auto_test = false,
                     bigserial_t _id = 0,
                     string _request_data = string(""),
                     string _check_for_it = string(""))
{
    boost::shared_ptr<boost::asio::io_service> _io_service;
    _io_service = boost::make_shared<boost::asio::io_service>();
    bigserial_t id;
    std::string reply = "";
    string _body;
    if (!auto_test) {
        cout << "Type in id to post (0..MAX(ull)): ";
        cin >> id;
        // flush the newline after latest >> operator
        cin.ignore();
        cout << "Data for the id: ";
        getline(cin, _body);
    } else {
        id = _id;
        _body = _request_data;
    }
    // create client
    http::client _client(http::client::options().io_service(_io_service));
    // create request string
    string uri = "http://";
    uri.append(host);
    uri.append(":");
    uri.append(port);
    uri.append("/users");
    if (id > 0) {
        uri.append("/");
        uri.append(to_string(id));
    }

    // create request object
    http::client::request _request(uri);

    // create response object
    http::client::response _response;
    std::condition_variable cv;
    std::unique_lock<std::mutex> reply_lock(reply_mutex);
    _response = _client.post(_request, _body, boost::bind(response_callback_with_reply, _1, _2, &reply, &cv));
    cv.wait(reply_lock);

    while (!boost::network::http::ready(_response));

    if (auto_test) {
        cout << "POST request: '" << uri << "'" << endl;
        cout << "POST data: '" << _body << "'" << endl;
        cout << "Response:" << endl
             << reply << endl;
        // check if _check_for_it is inside reply
        std::unique_lock<std::mutex> scoped_lock(counters_mutex);
        if (string::npos != reply.find(_check_for_it)) {
            ++tests_success;
            cout << "--- SUCCESS ---" << endl;
        } else {
            ++tests_failed;
            cout << "--- FAIL ---" << endl;
        }
    } else {
        cout << "POST request: '" << uri << "'" << endl;
        cout << "POST data: '" << _body << "'" << endl;
        cout << "Response:" << endl
             << reply << endl;
    }

    _io_service->stop();
    _io_service.reset();
}

void do_empty_post_request(bool auto_test = false,
                           bigserial_t _id = 0,
                           string _check_for_it = string(""))
{
    boost::shared_ptr<boost::asio::io_service> _io_service;
    _io_service = boost::make_shared<boost::asio::io_service>();
    bigserial_t id;
    string _body = " ";
    string reply = "";
    if (!auto_test) {
        cout << "Type in id to post (0..MAX(ull)): ";
        cin >> id;
    } else {
        id = _id;
    }

    // create client
    http::client _client(http::client::options().io_service(_io_service));
    // create request string
    string uri = "http://";
    uri.append(host);
    uri.append(":");
    uri.append(port);
    uri.append("/users");
    if (id > 0) {
        uri.append("/");
        uri.append(to_string(id));
    }

    // create request object
    http::client::request _request(uri);

    // create response object
    http::client::response _response;
    std::condition_variable cv;
    std::unique_lock<std::mutex> reply_lock(reply_mutex);
    _response = _client.post(_request, _body, boost::bind(response_callback_with_reply, _1, _2, &reply, &cv));
    cv.wait(reply_lock);

    while (!boost::network::http::ready(_response));

    if (auto_test) {
        cout << "POST request: '" << uri << "'" << endl;
        cout << "POST data: '" << _body << "'" << endl;
        cout << "Response:" << endl
             << reply << endl;
        // check if _check_for_it is inside reply
        std::unique_lock<std::mutex> scoped_lock(counters_mutex);
        if (string::npos != reply.find(_check_for_it)) {
            ++tests_success;
            cout << "--- SUCCESS ---" << endl;
        } else {
            ++tests_failed;
            cout << "--- FAIL ---" << endl;
        }
    } else {
        cout << "POST request: '" << uri << "'" << endl;
        cout << "POST data: '" << _body << "'" << endl;
        cout << "Response:" << endl
             << reply << endl;
    }

    _io_service->stop();
    _io_service.reset();
}

void do_auto_test(void)
{
    tests_success = tests_failed = 0;
    // single threaded testing
    do_get_request(true,
                   0,
                   "200 OK");
    do_get_request(true,
                   1000,
                   "404 Not Found");
    do_post_request(true,
                    0,
                    "{\"firstName\":\"a\", \"lastName\":\"b\", \"birthDate\":\"01-12-1900\"}",
                    "200 OK");
    do_get_request(true,
                   10000,
                   "404 Not Found");
    do_delete_request(true,
                      0,
                      "400 Bad Request");
    do_delete_request(true,
                      1000,
                      "404 Not Found");
    // assume id 10 exist
    do_get_request(true,
                   10,
                   "200 OK");
    do_delete_request(true,
                      10,
                      "200 OK");
    do_get_request(true,
                   10,
                   "404 Not Found");
    do_post_request(true,
                    10,
                    "{\"firstName\":\"a\", \"lastName\":\"b\", \"birthDate\":\"01-12-1900\"}",
                    "404 Not Found");
    // assume id 9 exist
    do_post_request(true,
                    9,
                    "{\"firstName\":\"changed\", \"lastName\":\"changed again\", \"birthDate\":\"13-12-1900\"}",
                    "200 OK");
    do_post_request(true,
                    9,
                    "{\"firstName\":\"a\", \"lastName\":\"b\", \"WrongNamedValue\":\"01-12-1900\"}",
                    "400 Bad Request");
    do_post_request(true,
                    9,
                    "{\"firstName\":\"a\", \"lastName\":\"b\", \"birthDate\":\"01-12-1900\",\"superfluous\":\"v\"}",
                    "400 Bad Request");
    do_post_request(true,
                    9,
                    "{\"firstName\":\"a\", \"lastName\":\"b\"}",
                    "400 Bad Request");

    do_post_request(true,
                    0,
                    "{\"firstName\":\"a\", \"lastName\":\"b\", \"WrongNamedValue\":\"01-12-1900\"}",
                    "400 Bad Request");
    do_post_request(true,
                    0,
                    "{\"firstName\":\"a\", \"lastName\":\"b\", \"birthDate\":\"01-12-1900\",\"superfluous\":\"v\"}",
                    "400 Bad Request");
    do_post_request(true,
                    0,
                    "{\"firstName\":\"a\", \"lastName\":\"b\"}",
                    "400 Bad Request");

    cout << "Tests:" << endl
         << "\tSuccess = " << tests_success << endl
         << "\tFail    = " << tests_failed << endl;
}

void do_auto_test_mt(void)
{
    tests_success = tests_failed = 0;

    boost::shared_ptr<boost::thread_group> _thread_group =
            boost::make_shared<boost::thread_group>();

    _thread_group->create_thread(boost::bind(
                                  do_get_request,true,
                                                 0,
                                                 "200 OK")
                                  );
    _thread_group->create_thread(boost::bind(
                                  do_get_request,true,
                                                 1000,
                                                 "404 Not Found"));
    _thread_group->create_thread(boost::bind(
                                  do_post_request,true,
                                                  0,
                                                  "{\"firstName\":\"c\", \"lastName\":\"d\", \"birthDate\":\"02-12-1900\"}",
                                                  "200 OK"
                                                  ));
    _thread_group->create_thread(boost::bind(
                                  do_get_request,true,
                                                 10000,
                                                 "404 Not Found"));
    _thread_group->create_thread(boost::bind(
                                  do_delete_request,true,
                                                    0,
                                                    "400 Bad Request"));
    _thread_group->create_thread(boost::bind(
                                  do_delete_request,true,
                                                    1000,
                                                    "404 Not Found"));
    // assume id 11 exists
    _thread_group->create_thread(boost::bind(
                                  do_get_request,true,
                                                 11,
                                                 "200 OK"));
    _thread_group->create_thread(boost::bind(
                                  do_delete_request,true,
                                                    11,
                                                    "200 OK"));
    _thread_group->create_thread(boost::bind(
                                  do_get_request,true,
                                                 11,
                                                 "404 Not Found"));
    _thread_group->create_thread(boost::bind(
                                  do_post_request,true,
                                                  11,
                                                  "{\"firstName\":\"a\", \"lastName\":\"b\", \"birthDate\":\"01-12-1900\"}",
                                                  "404 Not Found"));
    // assume id 8 exists
    _thread_group->create_thread(boost::bind(
                                  do_post_request,true,
                                                  8,
                                                  "{\"firstName\":\"changed\", \"lastName\":\"changed again\", \"birthDate\":\"13-12-1900\"}",
                                                  "200 OK"));
    _thread_group->create_thread(boost::bind(
                                  do_post_request,true,
                                                  8,
                                                  "{\"firstName\":\"a\", \"lastName\":\"b\", \"WrongNamedValue\":\"01-12-1900\"}",
                                                  "400 Bad Request"));
    _thread_group->create_thread(boost::bind(
                                  do_post_request,true,
                                                  8,
                                                  "{\"firstName\":\"a\", \"lastName\":\"b\", \"birthDate\":\"01-12-1900\",\"superfluous\":\"v\"}",
                                                  "400 Bad Request"));
    _thread_group->create_thread(boost::bind(
                                  do_post_request,true,
                                                  9,
                                                  "{\"firstName\":\"a\", \"lastName\":\"b\"}",
                                                  "400 Bad Request"));

    _thread_group->create_thread(boost::bind(
                                  do_post_request,true,
                                                  0,
                                                  "{\"firstName\":\"a\", \"lastName\":\"b\", \"WrongNamedValue\":\"01-12-1900\"}",
                                                  "400 Bad Request"));
    _thread_group->create_thread(boost::bind(
                                  do_post_request,true,
                                                  0,
                                                  "{\"firstName\":\"a\", \"lastName\":\"b\", \"birthDate\":\"01-12-1900\",\"superfluous\":\"v\"}",
                                                  "400 Bad Request"));
    _thread_group->create_thread(boost::bind(
                                  do_post_request,true,
                                                  0,
                                                  "{\"firstName\":\"a\", \"lastName\":\"b\"}",
                                                  "400 Bad Request"));

    _thread_group->join_all();
    cout << "Tests:" << endl
         << "\tSuccess = " << tests_success << endl
         << "\tFail    = " << tests_failed << endl;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        cout << "usage: " << argv[0] << " address port" << endl;
        return 0;
    }

    host = argv[1];
    port = argv[2];

    string request_kind; // get, post, delete

    cout << "Type in request kind (get, post, empty-post, delete, auto-test, auto-test-mt) :";
    cin >> request_kind;
    //request_kind = "auto-test";

    try {
        if (request_kind == "get") {
            do_get_request();
        } else if (request_kind == "delete") {
            do_delete_request();
        } else if (request_kind == "post") {
            do_post_request();
        } else if (request_kind == "empty-post") {
            do_empty_post_request();
        } else if (request_kind == "auto-test") {
            do_auto_test();
        } else if (request_kind == "auto-test-mt") {
            do_auto_test_mt();
        } else {
            cerr << "Wrong request type: " << request_kind << endl;
            return 1;
        }
    } catch (std::exception &e) {
        cerr << e.what() << endl;
        return 2;
    }

    return 0;
}
