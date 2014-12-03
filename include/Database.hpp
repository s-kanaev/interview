#ifndef _DATABASE_HPP
#define _DATABASE_HPP

#include "Server.hpp"
#include "common.hpp"
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <pqxx/pqxx>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

typedef enum _RequestType {
    REQUEST_POST,
    REQUEST_DELETE,
    REQUEST_GET,
    REQUEST_INVALID
} RequestType;

typedef struct _PostRequest {
    bigserial_t id; // -1 if not set
    char *first_name, *last_name, *birth_date; // NULL if not set
} PostRequest;

typedef struct _DeleteRequest {
    bigserial_t id; // should be no less than nil
} DeleteRequest;

typedef struct _GetRequest {
    bigserial_t id; // -1 to retrieve all records
} GetRequest;

// request to db structure
typedef struct _DBRequest {
    RequestType request_type;
    union {
        PostRequest post_request;
        DeleteRequest delete_request;
        GetRequest get_request;
    } any_request;
} DBRequest;

typedef enum _DBReplyKind {
    REPLY_OK,           // 200
    REPLY_BAD_REQUEST,  // 400
    REPLY_NOT_FOUND     // 404
} DBReplyKind;

class DBRecord {
public:
    bigserial_t id;
    std::string first_name, last_name, birth_date;
};

// typedef struct _DBRecord {
//     int id;
//     char *first_name, *last_name, *birth_date;
// } DBRecord;

class DBReply {
public:
    DBReply();
    ~DBReply();

    void SetKind(DBReplyKind _kind) {
        m_kind = _kind;
    };
    void SetRecords(std::vector<std::shared_ptr<DBRecord>>* _records) {
        m_records = _records;
    };

    DBReplyKind Kind(void) const {
        return m_kind;
    };
    std::vector<std::shared_ptr<DBRecord>>* Records(void) const {
        return m_records;
    };

protected:
    DBReplyKind m_kind;
    std::vector<std::shared_ptr<DBRecord>> *m_records;
};

// synchronous database access class
// this class should be used with async (boost:asio) wrapper
// singleton class for database requests
class Database {
public:
    Database& getInstance(void);

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
    void QueueRequest(DBRequest request, connection_object);

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
    std::vector<std::shared_ptr<DBRecord>> m_dbrecords;
    Cache<int, std::shared_ptr<DBRecord>> m_cache;

    // db thread mutex
    std::mutex m_db_thread_mutex;
    std::condition_variable m_db_thread_cv;
    // parallel queues for request and connection_objects
    boost::mutex m_queue_mutex;
    std::queue<DBRequest> m_request_queue;
    std::queue<connection_object&> m_connection_queue;

    std::shared_ptr<pqxx::connection> m_connection;      // connection to db
    pqxx::result m_result;              // result of the transaction

    boost::thread m_db_thread;

private:
    /*
     * create db_thread
     */
    Database();
    Database(Database const&);
    Database& operator=();
};

template<typename Key, typename T>
class Cache {
protected:
    bool m_isValid;
    std::vector<T> m_cached_values;
    std::map<Key, T> m_cached_values_map;

public:
    Cache();
    ~Cache();

    // check whether the cache is valid
    bool Valid(void) const {
        return m_isValid;
    };
    // set cache invalid and clear it
    void SetInvalid(bool invalid = true) {
        m_isValid = !invalid;
        m_cached_values.clear();
        m_cached_values_map.clear();
    };

    // add value to cache
    bool AddValue(Key key, T value) {
        m_cached_values.push_back(value);
        m_cached_values_map[key] = value;
        return true;
    };

    // find cached value by key
    T FindValue(Key key, bool *found) {
        if (!m_isValid) {
            *found = false;
            return T();
        }
        std::map<Key, T>::iterator it = m_cached_values_map.find(key);
        if (it == m_cached_values_map.end()) {
            *found = false;
            return T();
        }
        return m_cached_values_map[key];
    };

    // return array of cached values
    std::vector<T> const& CachedValues(void) {
        return m_cached_values;
    };
};

#endif