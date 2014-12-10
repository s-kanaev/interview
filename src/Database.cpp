#include "common.hpp"
#include "Database.hpp"
#include "Server.hpp"

#include <cstdio>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <pqxx/pqxx>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

// constructor
Database::Database()
{
    // we're not connected initialy to any database
    m_connected = false;
}

Database::Database(Database const&)
{
}

Database&
Database::operator=(const Database&)
{
}

Database&
Database::getInstance(void)
{
    static Database m_instance;
    return m_instance;
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

    // generate connection string
    if (!_host.empty()) connection_string.append("host="+_host+" ");
    if (!_port.empty()) connection_string.append("port="+_port+" ");
    if (!_username.empty()) connection_string.append("user="+_username+" ");
    if (!_password.empty()) connection_string.append("password="+_password+" ");
    if (!_db_name.empty()) connection_string.append("dbname="+_db_name+" ");

    try {
        m_connection.reset(new pqxx::connection(connection_string));
    }
    catch (std::exception &e) {
        printf("%s\n", e.what());
        return false;
    }

    // remember table name
    m_table = _table;
    m_connected = true;

    // initialize cache - set it invalid only
    m_cache.SetInvalid();

    // create db_thread
    m_db_thread = boost::thread(&Database::DoRequest, this);

    return true;
}

void
Database::Disconnect()
{
    m_db_thread.interrupt();
    m_db_thread.join();
    boost::unique_lock<boost::mutex> scoped_lock(m_queue_mutex);
    // we do disconnect here
    m_connected = false;
    m_connection.reset();
    // force connection and request queue to empty
    while (!m_connection_queue.empty()) m_connection_queue.pop();
    while (!m_request_queue.empty()) m_request_queue.pop();
}

void
Database::Request(std::string request_string)
{
    // create transaction to execute and commit
    pqxx::work transaction(*m_connection, request_string);

    try {
        m_result = transaction.exec(request_string);
    }
    catch (pqxx::pqxx_exception &e) {
        printf("pqxx exception: %s\n", e.base().what());
    }
    catch (std::exception &e) {
        printf("standard exception: %s\n", e.what());
    }

    transaction.commit();
}

// free those strings from post request
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

    // reply will not have any records supplied
    m_dbrecords->clear();

    // check if request is valid
    if (!first_name || !last_name || !birth_date) {
        m_dbreply.SetKind(REPLY_BAD_REQUEST);
        finalize_request_arguments(first_name, last_name, birth_date);
        return;
    }

    if (id > 0) {
        // id is provided
        // POST /users/173
        request_string.append("UPDATE ");
        request_string.append(m_table);
        request_string.append(" SET first_name = '");
        request_string.append(first_name);
        request_string.append("', last_name = '");
        request_string.append(last_name);
        request_string.append("', birth_date = '");
        request_string.append(birth_date);
        request_string.append("' WHERE id = ");
        request_string.append(std::to_string(id));
        request_string.append(";");
    } else {
        // add new record to table
        // POST /users
        request_string.append("INSERT INTO ");
        request_string.append(m_table);
        request_string.append(" (first_name, last_name, birth_date) VALUES ('");
        request_string.append(first_name);
        request_string.append("', '");
        request_string.append(last_name);
        request_string.append("', '");
        request_string.append(birth_date);
        request_string.append("');");
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

    m_dbrecords->clear();

    // check if request is valid
    if (id == 0) {
        m_dbreply.SetKind(REPLY_BAD_REQUEST);
        return;
    }

    // do the request
    m_dbreply.SetKind(REPLY_OK);
    m_cache.SetInvalid();
    // DELETE /users/173
    request_string.append("DELETE FROM ");
    request_string.append(m_table);
    request_string.append(" WHERE id = ");
    request_string.append(std::to_string(id));
    request_string.append(";");

    Request(request_string);
    // it was delete
    m_dbreply.SetKind(m_result.affected_rows() > 0 ? REPLY_OK : REPLY_NOT_FOUND);
}

void
Database::DoGetRequest(GetRequest *get_request)
{
    std::string request_string("");
    bigserial_t id = get_request->id;

    // check if cache is valid
    if (!m_cache.Valid()) {
        // renew cache
        request_string.append("SELECT id, first_name, last_name, birth_date FROM ");
        request_string.append(m_table);
        request_string.append(";");
        Request(request_string);

        // copy result of the select to cache
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
        // set cache valid
        m_cache.SetInvalid(false);
    }

    // now we only do get requests with cache
    if (id > 0) {
        // id provided
        bool found;
        std::shared_ptr<DBRecord> element = m_cache.FindValue(id, &found);
        m_dbrecords->clear();
        if (found) {//element.get()) {
            m_dbrecords->push_back(element);
            m_dbreply.SetKind(REPLY_OK);
        } else {
            m_dbreply.SetKind(REPLY_NOT_FOUND);
        }
    } else {
        const std::vector<std::shared_ptr<DBRecord>>& res = m_cache.CachedValues();
        // should copy from cached records due to cache invalidation
        // in between to requests
        m_dbrecords->clear();
        m_dbrecords->assign(res.begin(), res.end());
    }
}

void
Database::QueueRequest(DBRequest db_request, async_server::connection_ptr &connection)
{
    // lock mutex
    boost::unique_lock<boost::mutex> scoped_lock(m_queue_mutex);
    // add request and connection object to queues
    m_request_queue.push(db_request);
    m_connection_queue.push(async_server::connection_ptr(connection));
    // notify m_db_thread
    m_db_thread_cv.notify_all();
    // unlock mutex
}

void
Database::DoRequest(void)
{
    boost::unique_lock<boost::mutex> scoped_lock(m_queue_mutex);
    //std::unique_lock<std::mutex> locker(m_db_thread_mutex);
    while (m_connected && !boost::this_thread::interruption_requested()) {
        // wait for notification to do some requests
        m_db_thread_cv.wait(scoped_lock, [&](){
            //boost::unique_lock<boost::mutex> sl(m_queue_mutex);
            return !m_request_queue.empty() ||
                   !m_connected ||
                   boost::this_thread::interruption_requested();
        });
        DBRequest request;
        async_server::connection_ptr co;

        // queue mutex is still locked at this point
        while (!m_request_queue.empty() &&
               m_connected &&
               !boost::this_thread::interruption_requested()) {
            // retrieve request structure and connection_object
            request = m_request_queue.front();
            m_request_queue.pop();
            co = m_connection_queue.front();
            m_connection_queue.pop();

            // we don't need the lock anymore
            scoped_lock.unlock();

            m_dbrecords.reset(new std::vector<std::shared_ptr<DBRecord>>);

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
                    m_dbrecords->clear();
                    m_dbreply.SetKind(REPLY_BAD_REQUEST);
                    break;
            }

            // launch thread to reply to client
            //threadPool.post(boost::bind(&Server::ReplyToClient, ServerInstance, &m_dbreply, async_server::connection_ptr));
            m_dbreply.SetRecords(m_dbrecords);
            threadPool->post(boost::bind(ServerSendReply, m_dbreply, co));

            // lock queue mutex
            scoped_lock.lock();
        }
    }
}
