#ifndef _DBREPLY_HPP_
#define _DBREPLY_HPP_

#include "common.hpp"

#include <vector>
#include <memory>

class DBReply {
public:
    DBReply();
    DBReply(const DBReply &ref);
    ~DBReply();

    void SetKind(DBReplyKind _kind);
    void SetRecords(std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> _records);
    DBReplyKind Kind(void) const;
    std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> Records(void) const;

protected:
    DBReplyKind m_kind;
    std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> m_records;
};

#endif