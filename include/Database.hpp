#ifndef _DATABASE_HPP
#define _DATABASE_HPP

#include "Server.hpp"
#include "Cache.hpp"
#include "DBReply.hpp"
#include "common.hpp"
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <pqxx/pqxx>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

// synchronous database access class
// this class should be used with async (boost:asio) wrapper
// singleton class for database requests
class Database {
public:
    static Database& getInstance(void);

    /*
     * connect to database
     * return true if success, false otherwise or if already connected
     */
    bool Connect(std::string _host,
                 std::string _port,
                 std::string _username,
                 std::string _password,
                 std::string _db_name,
                 std::string _table);
    // disconnect from database
    bool Disconnect();
    // add request and connection object (for reply) to queue
    /*
     * add the request and connection object to working queue
     */
    void QueueRequest(DBRequest db_request, async_server::connection_ptr connection);

    /*
     * thread to process queued requests
     */
    void DoRequest(void);

protected:
    // explicitly do POST request
    void DoPostRequest(PostRequest *post_request);
    // explicitly do DELETE request
    void DoDeleteRequest(DeleteRequest *delete_request);
    // explicitly do GET request
    void DoGetRequest(GetRequest *get_request);
    // explicitly do request
    void Request(std::string request_string);
    // check for record by id
    bool CheckRecordByID(int id);

    bool m_connected = false;
    bool m_force_reply = false;

    std::string m_table;

    DBReply m_dbreply;
    std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> m_dbrecords;
    Cache<bigserial_t, std::shared_ptr<DBRecord>> m_cache;

    // db thread mutex
    std::mutex m_db_thread_mutex;
    std::condition_variable m_db_thread_cv;
    // parallel queues for request and connection_objects
    boost::mutex m_queue_mutex;
    std::queue<DBRequest> m_request_queue;
    std::queue<async_server::connection_ptr> m_connection_queue;

    std::shared_ptr<pqxx::connection> m_connection;      // connection to db
    pqxx::result m_result;              // result of the transaction

    boost::thread m_db_thread;

private:
    /*
     * create db_thread
     */
    Database();
    Database(Database const&);
    Database& operator=(Database const&);
};
#endif