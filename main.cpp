#include "Server.hpp"
#include "Database.hpp"
#include <iostream>

int main(int argc, char **argv)
{
    if (argc < 9) {
        std::cout << "usage: " << argv[0]
                  << " host port username password"
                  << " database-name table-name server-host server-port"
                  << std::endl;
        exit(0);
    }
    std::string _host = argv[1],
                _port = argv[2],
                _username = argv[3],
                _password = argv[4],
                _db_name = argv[5],
                _table_name = argv[6],
                _s_host = argv[7],
                _s_port = argv[8];
    try {
        bool res =
            Database::getInstance().Connect(
                                            _host,
                                            _port,
                                            _username,
                                            _password,
                                            _db_name,
                                            _table_name
                                        );
        if (!res) {
            std::cout << "Cannot connect to database:\n"
                      << "\thost - " << _host << "\n"
                      << "\tport - " << _port << "\n"
                      << "\tusername - " << _username << "\n"
                      << "\tpassword - " << _password << "\n"
                      << "\t_db_name - " << _db_name << "\n"
                      << "\t_table_name - " << _table_name << "\n";
            return 1;
        }

/************************************************************
        // Do sample requests (for test of DB class
        DBRequest _request;

        _request.request_type = REQUEST_GET;
        _request.any_request.get_request.id = 0;
        Database::getInstance().QueueRequest(_request, async_server::connection_ptr());

        _request.request_type = REQUEST_GET;
        _request.any_request.get_request.id = 1000;
        Database::getInstance().QueueRequest(_request, async_server::connection_ptr());

        _request.request_type = REQUEST_GET;
        _request.any_request.get_request.id = 1;
        Database::getInstance().QueueRequest(_request, async_server::connection_ptr());

        _request.request_type = REQUEST_POST;
        _request.any_request.post_request.id = 0;
        _request.any_request.post_request.first_name = strdup("Sergey");
        _request.any_request.post_request.last_name = strdup("Kanaev");
        _request.any_request.post_request.birth_date = strdup("06-10-1991");
        Database::getInstance().QueueRequest(_request, async_server::connection_ptr());

        _request.request_type = REQUEST_POST;
        _request.any_request.post_request.id = 1;
        _request.any_request.post_request.first_name = strdup("fnameChanged");
        _request.any_request.post_request.last_name = strdup("lNameChanged");
        _request.any_request.post_request.birth_date = strdup("08-10-1900");
        Database::getInstance().QueueRequest(_request, async_server::connection_ptr());

        _request.request_type = REQUEST_DELETE;
        _request.any_request.delete_request.id = 10000;
        Database::getInstance().QueueRequest(_request, async_server::connection_ptr());

        _request.request_type = REQUEST_POST;
        _request.any_request.post_request.id = 0;
        _request.any_request.post_request.first_name = NULL;
        _request.any_request.post_request.last_name = strdup("Kanaev");
        _request.any_request.post_request.birth_date = strdup("06-10-1991");
        Database::getInstance().QueueRequest(_request, async_server::connection_ptr());

        _request.request_type = REQUEST_DELETE;
        _request.any_request.delete_request.id = 1;
        Database::getInstance().QueueRequest(_request, async_server::connection_ptr());
************************************************************/

        RunServer(_s_host, _s_port);

        Database::getInstance().Disconnect();
    }
    catch (std::string e) {
        std::cout << "String catched: " <<
                     e << std::endl;
        return 1;
    }
    catch (pqxx::usage_error e) {
        std::cout << "PQXX says: " <<
                     e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Abnormal termination: " <<
                     e.what() << std::endl;
        return 1;
    }

    std::cout << "Waiting for thread pool to empty" << std::endl;
    //sleep(5);
    threadPool.reset();

    std::cout << "Terminated normally" << std::endl;
    return 0;
}
