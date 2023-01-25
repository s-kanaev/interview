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

struct AsyncReplyHandler {
    std::string WriteDBRecordAsJSON(DBRecord *db_record)
    {
        json_spirit::Object db_record_obj;
        std::string str;
        db_record_obj.push_back(json_spirit::Pair("id", std::to_string(db_record->id)));
        db_record_obj.push_back(json_spirit::Pair("firstName", db_record->first_name));
        db_record_obj.push_back(json_spirit::Pair("lastName", db_record->last_name));
        db_record_obj.push_back(json_spirit::Pair("birthDate", db_record->birth_date));
        str = json_spirit::write(db_record_obj, json_spirit::pretty_print);
        return str;
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
                std::vector<std::shared_ptr<DBRecord>> *db_records;
                db_records = db_reply->Records();
                switch (db_records->size()) {
                    case 0:
                        break;
                    case 1:
                        reply_string = WriteDBRecordAsJSON(db_records->at(0).get());
                        break;
                    default:
                        // post '['
                        reply_string.append("[");
                        // post each of the record
                        for (int i = 0; i < db_records->size(); ++i) {
                            one_record_string = WriteDBRecordAsJSON(db_records->at(i).get());
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