#include <boost/range/iterator_range.hpp>
#include <boost/network/protocol/http/client.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <string>

namespace http = boost::network::http;

using namespace std;

typedef unsigned long long int bigserial_t;
typedef boost::iterator_range<char const *> rValue;

boost::shared_ptr<boost::asio::io_service> _io_service = boost::make_shared<boost::asio::io_service>();
string host;
string port;

unsigned int tests_success = 0;
unsigned int tests_failed = 0;

void response_callback(boost::iterator_range<char const *> const &a,
                       boost::system::error_code const &error_code)
{
    for(rValue::difference_type i = 0; i < a.size(); ++i) {
        cout << a[i];
    }
    cout << endl;
}

void response_callback_with_reply(boost::iterator_range<char const *> const &a,
                                  boost::system::error_code const &error_code,
                                  std::string *reply)
{
    reply->append(a.begin());
    cout << a.begin();
}

void do_get_request(bool auto_test = false,
                    bigserial_t _id = 0,
                    string _check_for_it = string(""))
{
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

    cout << "GET request: '" << uri << "'" << endl;
    // create request object
    http::client::request _request(uri);

    // create response object
    http::client::response _response;
    if (!auto_test) {
        _response = _client.get(_request, response_callback);
    } else {
        _response = _client.get(_request, boost::bind(response_callback, _1, _2, &reply));
    }

    while (!ready(_response));

    if (auto_test) {
        // check if _check_for_it is inside reply
        if (string::npos != reply.find(_check_for_it)) {
            ++tests_success;
            cout << "--- SUCCESS ---" << endl;
        } else {
            ++tests_failed;
            cout << "--- FAIL ---" << endl;
        }
    }

    _io_service->stop();
}

void do_delete_request(bool auto_test = false,
                       bigserial_t _id = 0,
                       string _check_for_it = string(""))
{
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

    cout << "DELETE request: '" << uri << "'" << endl;
    // create request object
    http::client::request _request(uri);

    // create response object
    http::client::response _response;
    if (!auto_test) {
        _response = _client.get(_request, response_callback);
    } else {
        _response = _client.get(_request, boost::bind(response_callback, _1, _2, &reply));
    }

    while (!ready(_response));

    if (auto_test) {
        // check if _check_for_it is inside reply
        if (string::npos != reply.find(_check_for_it)) {
            ++tests_success;
            cout << "--- SUCCESS ---" << endl;
        } else {
            ++tests_failed;
            cout << "--- FAIL ---" << endl;
        }
    }

    _io_service->stop();
}

void do_post_request(bool auto_test = false,
                     bigserial_t _id = 0,
                     string _request_data = string(""),
                     string _check_for_it = string(""))
{
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

    cout << "POST request: '" << uri << "'" << endl;
    cout << "POST data: '" << _body << "'" << endl;
    // create request object
    http::client::request _request(uri);

    // create response object
    http::client::response _response;
    if (!auto_test) {
        _response = _client.get(_request, _body, response_callback);
    } else {
        _response = _client.get(_request, _body, boost::bind(response_callback, _1, _2, &reply));
    }

    while (!ready(_response));

    if (auto_test) {
        // check if _check_for_it is inside reply
        if (string::npos != reply.find(_check_for_it)) {
            ++tests_success;
            cout << "--- SUCCESS ---" << endl;
        } else {
            ++tests_failed;
            cout << "--- FAIL ---" << endl;
        }
    }

    _io_service->stop();
}

void do_empty_post_request(bool auto_test = false,
                           bigserial_t _id = 0,
                           string _check_for_it = string(""))
{
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

    cout << "POST request: '" << uri << "'" << endl;
    cout << "POST data: '" << _body << "'" << endl;
    // create request object
    http::client::request _request(uri);

    // create response object
    http::client::response _response;
    if (!auto_test) {
        _response = _client.get(_request, response_callback);
    } else {
        _response = _client.get(_request, boost::bind(response_callback, _1, _2, &reply));
    }

    while (!ready(_response));

    if (auto_test) {
        // check if _check_for_it is inside reply
        if (string::npos != reply.find(_check_for_it)) {
            ++tests_success;
            cout << "--- SUCCESS ---" << endl;
        } else {
            ++tests_failed;
            cout << "--- FAIL ---" << endl;
        }
    }

    _io_service->stop();
}

void do_auto_test(void)
{
    tests_success = tests_failed = 0;
    // single threaded testing
    do_get_request(true,
                   0,
                   "200 OK");
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
                   "404 Not Found");
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
}

void do_auto_test_mt(void)
{
    // TODO multi threaded testing
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
