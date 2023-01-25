#include "common.hpp"
#include "Database.hpp"

#include <cstdio>
#include <vector>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

Database::Database()
{
}

Database::Database(Database const&)
{
}

Database&
Database::operator=(const Database&)
{
}

bool
Database::Connect(std::string _host,
                  std::string _port,
                  std::string _username,
                  std::string _password,
                  std::string _db_name,
                  std::string _table)
{
    std::string connection_string = "";

    if (m_connected) return false;

    if (!_host.empty()) connection_string.append("host="+_host+" ");
    if (!_port.empty()) connection_string.append("port="+_port+" ");
    if (!_username.empty()) connection_string.append("username="+_username+" ");
    if (!_password.empty()) connection_string.append("password"+_password+" ");
    if (!_db_name.empty()) connection_string.append("dbname="+_db_name+" ");

    try {
        m_connection.reset(new pqxx::connection(connection_string));
    }
    catch (std::exception &e) {
        std::printf("%s\n", e.what());
        return false;
    }

    m_table = _table;
    m_connected = true;

    // initialize cache
    m_cache.SetInvalid();

    // create db_thread
    m_db_thread = boost::thread(&Database::DoRequest, this);

    return true;
}

void
Database::Request(std::string request_string)
{
    pqxx::work transaction(*m_connection, request_string);

    try {
        m_result = transaction.exec(request_string);
    }
    catch (pqxx::pqxx_exception &e) {
        std::printf("pqxx exception: %s\n", e.base().what());
    }
    catch (std::exception &e) {
        std::printf("standard exception: %s\n", e.what());
    }

    transaction.commit();
}

bool
Database::CheckRecordByID(int id)
{
    bool found;
    // check that there is a record for the user with the id provided
    if (m_cache.Valid()) {
        m_cache.FindValue(id, &found);
    } else {
        std::string _rs("");
        _rs.append("SELECT id, first_name, last_name, birth_date FROM ");
        _rs.append(m_table);
        _rs.append(" WHERE id = ");
        _rs.append(std::to_string(id));
        Request(_rs);
        if (m_result.empty()) found = false;
        else found = true;
    }
    return found;
}

void finalize_request_arguments(char *f_name, char *l_name, char *b_date)
{
    if (f_name) free((void*)f_name);
    if (l_name) free((void*)l_name);
    if (b_date) free((void*)b_date);
}

void
Database::DoPostRequest(PostRequest *post_request)
{
    std::string request_string("");
    bigserial_t id = post_request->id;
    char *first_name = post_request->first_name,
         *last_name = post_request->last_name,
         *birth_date = post_request->birth_date;

    m_force_reply = false;
    m_dbreply.Records()->clear();

    if (!first_name || !last_name || !birth_date) {
        m_dbreply.SetKind(REPLY_BAD_REQUEST);
        finalize_request_arguments(first_name, last_name, birth_date);
        return;
    }

    if (id > 0) {
        // try to check if it is the id exists in db
        bool found;
        found = CheckRecordByID(id);
        if (!found) {
            m_dbreply.SetKind(REPLY_NOT_FOUND);
            finalize_request_arguments(first_name, last_name, birth_date);
            return;
        } else {
            // POST /users/173
            request_string.append("UPDATE "+m_table+
                                  " SET first_name = '"+first_name+
                                  "' last_name = '"+last_name+
                                  "' birth_date = '"+birth_date+"'"+
                                  "WHERE id = "+std::to_string(id)+";");
        }
    } else {
        // POST /users
        request_string.append("INSERT INTO "+m_table+
                              " (first_name, last_name, birth_date) "+
                              "VALUES ('"+first_name+"', '"+
                              last_name+"', "+
                              birth_date+"');");
    }

    m_cache.SetInvalid();
    Request(request_string);
    // it was either update or insert
    m_dbreply.SetKind(m_result.affected_rows() > 0 ? REPLY_OK : REPLY_NOT_FOUND);
    finalize_request_arguments(first_name, last_name, birth_date);
}

void
Database::DoDeleteRequest(DeleteRequest *delete_request)
{
    std::string request_string("");
    bigserial_t id = delete_request->id;
    bool found;

    m_dbreply.Records()->clear();

    found = CheckRecordByID(id);

    if (!found) {
        m_dbreply.SetKind(REPLY_NOT_FOUND);
        return;
    } else {
        m_dbreply.SetKind(REPLY_OK);
        m_cache.SetInvalid();
        m_force_reply = false;
        // DELETE /users/173
        request_string.append("DELETE FROM "+m_table+
                              "WHERE id = "+std::to_string(id)+";");

        Request(request_string);
        // it was delete
        m_dbreply.SetKind(m_result.affected_rows() > 0 ? REPLY_OK : REPLY_NOT_FOUND);
    }
}

void
Database::DoGetRequest(GetRequest *get_request)
{
    std::string request_string("");
    bigserial_t id = get_request->id;

    if (!m_cache.Valid()) {
        request_string.append("SELECT id, first_name, last_name, birth_date FROM ");
        request_string.append(m_table);
        Request(request_string);

        // renew cache
        for (pqxx::result::const_iterator it = m_result.begin();
             it != m_result.end();
             ++it) {
            std::shared_ptr<DBRecord> record;
            record.reset(new DBRecord);
            record->id = it["id"].as<int>();
            record->first_name = it["first_name"].as<std::string>();
            record->last_name = it["last_name"].as<std::string>();
            record->birth_date = it["birth_date"].as<std::string>();
            m_cache.AddValue(record->id, record);
        }
        // set it valid
        m_cache.SetInvalid(false);
    }

    if (id > 0) {
        bool found;
        std::shared_ptr<DBRecord> element = m_cache.FindValue(id, &found);
        m_dbrecords.clear();
        if (found) {//element.get()) {
            m_dbrecords.push_back(element);
            m_dbreply.SetKind(REPLY_OK);
        } else {
            m_dbreply.SetKind(REPLY_NOT_FOUND);
        }
    } else {
        const std::vector<std::shared_ptr<DBRecord>>& res = m_cache.CachedValues();
        // should copy from cached records due to cache invalidation
        // in between to requests
        m_dbrecords = res;
    }
    m_force_reply = true;
}

void
Database::QueueRequest(DBRequest db_request, async_server::connection_ptr connection)
{
    // lock mutex
    boost::unique_lock<boost::mutex> scoped_lock(m_queue_mutex);
    // add request and connection object to queues
    m_request_queue.push(db_request);
    m_connection_queue.push(connection);
    // unlock mutex
}

void
Database::DoRequest(void)
{
    while (m_connected) {
        // wait for notification to do some requests
        std::unique_lock<std::mutex> locker(m_db_thread_mutex);
        m_db_thread_cv.wait(locker, [&](){
            boost::unique_lock<boost::mutex> sl(m_queue_mutex);
            return !m_request_queue.empty() || !m_connected;
        });

        boost::unique_lock<boost::mutex> scoped_lock(m_queue_mutex);
        DBRequest request;
        async_server::connection_ptr co;

        // retrieve request structure and connection_object
        // lock queue mutex
        while (!m_request_queue.empty()) {
            request = m_request_queue.front();
            m_request_queue.pop();
            co = m_connection_queue.front();
            m_connection_queue.pop();

            // we don't need the lock anymore
            scoped_lock.release();

            // execute the request
            switch (request.request_type) {
                case REQUEST_GET:
                    DoGetRequest(&(request.any_request.get_request));
                    // this request reply is already in m_dbrecords
                    break;
                case REQUEST_POST:
                    DoPostRequest(&(request.any_request.post_request));
                    break;
                case REQUEST_DELETE:
                    DoDeleteRequest(&(request.any_request.delete_request));
                    break;
                default:
                    m_dbreply.SetKind(REPLY_BAD_REQUEST);
                    break;
            }

            // TODO: launch thread to reply to client
            //threadPool.post(boost::bind(&Server::ReplyToClient, ServerInstance, &m_dbreply, async_server::connection_ptr));
            m_queue_mutex.lock();
        }
    }
}
