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
// this class should be used with async (boost:asio) wrapper (???)
// singleton class for database requests
class Database {
public:
    // return singleton instance of the database handler
    static Database& getInstance(void);

    /*
     * connect to database and create db talker thread
     * return true if success, false otherwise or if already connected
     */
    bool Connect(std::string _host,
                 std::string _port,
                 std::string _username,
                 std::string _password,
                 std::string _db_name,
                 std::string _table);
    // queue disconnect from database
    void Disconnect();
    /*
     * add the request and connection object to queue
     */
    void QueueRequest(DBRequest db_request, async_server::connection_ptr &connection);

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
    // do disconnect from db
    void DoDisconnect(void);

    bool m_connected;

    std::string m_table;

    bool m_do_disconnect;

    // database reply object
    DBReply m_dbreply;
    // database records vector for use with reply object
    std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> m_dbrecords;
    // cache object
    Cache<bigserial_t, std::shared_ptr<DBRecord>> m_cache;

    // db thread mutex
    std::mutex m_db_thread_mutex;
    // condition variable for database thread
    std::condition_variable m_db_thread_cv;
    // parallel queues for request and connection_objects
    std::queue<DBRequest> m_request_queue;
    std::queue<async_server::connection_ptr> m_connection_queue;
    // mutex for request and connection queues
    boost::mutex m_queue_mutex;

    // database connection
    std::shared_ptr<pqxx::connection> m_connection;
    // result of transaction to database
    pqxx::result m_result;

    // thread to talk with database
    boost::thread m_db_thread;

private:
    // as a singleton - no construction from outside, no copy
    Database();
    Database(Database const&);
    Database& operator=(Database const&);
};
#endif
