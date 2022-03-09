#include "buffers.h"

#include "logs.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct char_buff char_buff_create()
{
    struct char_buff buff = { 0, 0, NULL };
    return buff;
}

bool char_buff_alloc(struct char_buff* buff, size_t new_capacity)
{
    char* ptr = realloc(buff->data, new_capacity);
    if (!ptr) {
        log_error("malloc error!!!");
        return false;
    }

    buff->capacity = new_capacity;
    buff->data = ptr;
    return true;
}

bool char_buff_add(struct char_buff* buff, char c)
{
    if (buff->size == buff->capacity) {
        if (!char_buff_alloc(buff, 2 * buff->capacity)) {
            return false;
        }
    }

    buff->data[buff->size] = c;
    ++buff->size;
    return true;
}

void char_buff_memset(struct char_buff* buff, char c)
{
    memset(buff->data, c, buff->capacity);
}

struct char_buff char_buff_copy_span(char* begin, char* end)
{
    struct char_buff buff = char_buff_create();
    char_buff_alloc(&buff, end - begin);

    while (begin < end) {
        char_buff_add(&buff, *begin);
        ++begin;
    }

    return buff;
}

void char_buff_free(struct char_buff* buff)
{
    free(buff->data);
    buff->data = NULL;
    buff->capacity = 0;
    buff->size = 0;
}

struct connection_info connection_info_create(struct ssl_st* ssl_connection, int client_socket, char* name)
{
    struct connection_info info = {
        ssl_connection,
        client_socket,
        {}
    };

    memset(info.name, 0, 20);
    strncpy(info.name, name, 20);

    return info;
}

struct connection_info_buff connection_info_buff_create()
{
    struct connection_info_buff buff = { 0, 0, NULL };
    return buff;
}

bool connection_info_buff_alloc(struct connection_info_buff* buff, size_t new_capacity)
{
    struct connection_info* ptr = realloc(buff->data, new_capacity * sizeof(struct connection_info));
    if (!ptr) {
        log_error("malloc error!!!");
        return false;
    }

    buff->capacity = new_capacity;
    buff->data = ptr;
    return true;
}

bool connection_info_buff_add(struct connection_info_buff* buff, struct connection_info* info)
{
    if (buff->size == buff->capacity) {
        if (!connection_info_buff_alloc(buff, 2 * buff->capacity)) {
            return false;
        }
    }

    buff->data[buff->size] = *info;
    ++buff->size;
    return true;
}

void connection_info_buff_free(struct connection_info_buff* buff)
{
    free(buff->data);
    *buff = connection_info_buff_create();
}

struct connection_info* connection_info_buff_find(struct connection_info_buff* buff, char* name)
{
    for (uint32_t i = 0; i < buff->size; ++i) {
        struct connection_info* info = &buff->data[i];

        if (strncmp(info->name, name, 20) == 0) {
            return info;
        }
    }

    return NULL;
}

bool connection_info_buff_delete(struct connection_info_buff* buff, char* name)
{
    for (uint32_t i = 0; i < buff->size; ++i) {
        struct connection_info* info = &buff->data[i];

        if (strncmp(info->name, name, 20) == 0) {
            buff->data[i] = buff->data[buff->size - 1];
            --buff->size;

            return true;
        }
    }

    return false;
}