#ifndef _DBREPLY_HPP_
#define _DBREPLY_HPP_

#include "common.hpp"

#include <vector>
#include <memory>

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

#endif