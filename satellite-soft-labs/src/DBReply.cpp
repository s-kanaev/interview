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

// copy constructor (for use with vectors)
DBReply::DBReply(const DBReply &ref)
{
    m_kind = ref.Kind();
    m_records = ref.Records();
}

// set kind of reply
void DBReply::SetKind(DBReplyKind _kind)
{
    m_kind = _kind;
}

// set reply supplementary records
void DBReply::SetRecords(std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> _records)
{
    m_records = _records;
}

// retrieve reply type
DBReplyKind DBReply::Kind(void) const
{
    return m_kind;
}

// retrieve reply records
std::shared_ptr<std::vector<std::shared_ptr<DBRecord>>> DBReply::Records(void) const
{
    return m_records;
}
