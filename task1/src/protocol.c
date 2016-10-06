#include "protocol.h"

const size_t PR_STRUCT_EXPECTED_SIZE[] = {
    [PR_REQUEST]        = sizeof(pr_request_t),
    [PR_RESPONSE]       = sizeof(pr_response_t),
    [PR_MSG]            = sizeof(pr_msg_t),
    [PR_VOTE]           = sizeof(pr_vote_t),
    [PR_RESET_MASTER]   = sizeof(pr_reset_master_t)
};
