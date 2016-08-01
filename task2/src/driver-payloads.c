#include "driver-payloads.h"

#include <assert.h>
#include <string.h>

static
void one_o1(int idx,
            const char *name, size_t name_len,
            int argc,
            driver_command_argument_t *argv,
            buffer_t *response);
static
void one_o2(int idx,
            const char *name, size_t name_len,
            int argc,
            driver_command_argument_t *argv,
            buffer_t *response);
static
void one_o3(int idx,
            const char *name, size_t name_len,
            int argc,
            driver_command_argument_t *argv,
            buffer_t *response);

static
void two_t1(int idx,
            const char *name, size_t name_len,
            int argc,
            driver_command_argument_t *argv,
            buffer_t *response);
static
void two_t2(int idx,
            const char *name, size_t name_len,
            int argc,
            driver_command_argument_t *argv,
            buffer_t *response);


void one_o1(int idx,
            const char *name, size_t name_len,
            int argc, driver_command_argument_t *argv,
            buffer_t *response) {
    switch (argc) {
        case 0:
#define MSG "O1 without any arguments"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        case 1:
#define MSG "O1 with single argument"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        default:
#define MSG "O1 fails"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
    }
}

void one_o2(int idx,
            const char *name, size_t name_len,
            int argc, driver_command_argument_t *argv,
            buffer_t *response) {
    switch (argc) {
        case 0:
#define MSG "O2 without any arguments"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        case 1:
#define MSG "O2 with single argument"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        case 2:
#define MSG "O2 with two arguments"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        case 3:
#define MSG "O2 with even three arguments!"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        default:
#define MSG "O2 fails"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
    }
}

void one_o3(int idx,
            const char *name, size_t name_len,
            int argc, driver_command_argument_t *argv,
            buffer_t *response) {
    switch (argc) {
        case 0:
#define MSG "O3 without any arguments"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        case 1:
#define MSG "O3 with single argument"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        case 2:
#define MSG "O3 with two arguments"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        case 3:
#define MSG "O3 with even three arguments!"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        default:
#define MSG "O3 fails"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
    }
}

void two_t1(int idx,
            const char *name, size_t name_len,
            int argc, driver_command_argument_t *argv,
            buffer_t *response) {
    switch (argc) {
        case 0:
#define MSG "T1 without any arguments"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        case 1:
#define MSG "T1 with single argument"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        case 2:
#define MSG "T1 with two arguments"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        case 3:
#define MSG "T1 with even three arguments!"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        default:
#define MSG "T1 fails"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
    }
}

void two_t2(int idx,
            const char *name, size_t name_len,
            int argc, driver_command_argument_t *argv,
            buffer_t *response) {
    switch (argc) {
        case 0:
#define MSG "T2 without any arguments"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        case 1:
#define MSG "T2 with single argument"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
        default:
#define MSG "O3 fails"
            buffer_realloc(response, sizeof(MSG) - 1);
            memcpy(response->data, MSG, sizeof(MSG) - 1);
#undef MSG
            break;
    }
}

void driver_payload_one(driver_payload_t *dp,unsigned int slot) {
    driver_command_t *dc;
    assert(dp);

    dp->name = "one";
    dp->name_len = strlen(dp->name);
    dp->slot_number = slot;

    vector_init(&dp->commands, sizeof(driver_command_t), 0);
    dc = (driver_command_t *)vector_append(&dp->commands);
    dc->description = "O1 is the first command ever. For One driver only\n.";
    dc->description_len = strlen(dc->description);
    dc->handler = one_o1;
    dc->name = "o1";
    dc->name_len = strlen(dc->name);
    dc->max_arity = 1;

    dc = (driver_command_t *)vector_append(&dp->commands);
    dc->description = "O2 is the second command for One driver\n.";
    dc->description_len = strlen(dc->description);
    dc->handler = one_o1;
    dc->name = "o2";
    dc->name_len = strlen(dc->name);
    dc->max_arity = 3;

    dc = (driver_command_t *)vector_append(&dp->commands);
    dc->description = "O3 is the third command for One driver\n.";
    dc->description_len = strlen(dc->description);
    dc->handler = one_o1;
    dc->name = "o3";
    dc->name_len = strlen(dc->name);
    dc->max_arity = 2;
}

void driver_payload_two(driver_payload_t *dp, unsigned int slot) {
    driver_command_t *dc;
    assert(dp);

    dp->name = "two";
    dp->name_len = strlen(dp->name);
    dp->slot_number = slot;

    vector_init(&dp->commands, sizeof(driver_command_t), 0);
    dc = (driver_command_t *)vector_append(&dp->commands);
    dc->description = "T1 is the first command for Two driver";
    dc->description_len = strlen(dc->description);
    dc->handler = two_t1;
    dc->name = "t1";
    dc->name_len = strlen(dc->name);
    dc->max_arity = 2;

    dc = (driver_command_t *)vector_append(&dp->commands);
    dc->description = "T2 is the second command for Two driver\n.";
    dc->description_len = strlen(dc->description);
    dc->handler = two_t2;
    dc->name = "t2";
    dc->name_len = strlen(dc->name);
    dc->max_arity = 5;
}
