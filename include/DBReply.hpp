#ifndef _DBREPLY_HPP_
#define _DBREPLY_HPP_

#include "common.hpp"

#include <vector>
#include <memory>

// database reply class
class DBReply {
public:
    // construct an empty reply
    DBReply();
    // copy constructor
    DBReply(const DBReply &ref);
    // destructor
    ~DBReply();

    // set kind of reply (ok, not found, bad request)
    void SetKind(DBReplyKind _kind);
    // set db records for the reply
    void SetRecords(std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> _records);

    // retrieve reply kind
    DBReplyKind Kind(void) const;
    // retrieve reply db records
    std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> Records(void) const;

protected:
    // kind of the reply
    DBReplyKind m_kind;
    // supplementary records for this reply (used with GET request only)
    std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> m_records;
};

#endif