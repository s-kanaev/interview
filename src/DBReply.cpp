#include "DBReply.hpp"
#include "common.hpp"

#include <vector>
#include <memory>

DBReply::DBReply()
{
}

DBReply::~DBReply()
{
}

DBReply::DBReply(const DBReply &ref)
{
    m_kind = ref.Kind();
    m_records = ref.Records();
}

void DBReply::SetKind(DBReplyKind _kind) {
    m_kind = _kind;
};

void DBReply::SetRecords(std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> _records) {
    m_records = _records;
};

DBReplyKind DBReply::Kind(void) const {
    return m_kind;
};

std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> DBReply::Records(void) const {
    return m_records;
};

