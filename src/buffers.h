#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct char_buff {
    size_t capacity;
    size_t size;
    char* data;
};

struct char_buff char_buff_create();

bool char_buff_alloc(struct char_buff* buff, size_t new_capacity);

bool char_buff_add(struct char_buff* buff, char c);

void char_buff_memset(struct char_buff* buff, char c);

struct char_buff char_buff_copy_span(char* begin, char* end);

void char_buff_free(struct char_buff* buff);

struct connection_info {
    struct ssl_st* ssl_connection;
    int client_socket;
    char name[20];
};

struct connection_info connection_info_create(struct ssl_st* ssl_connection_, int client_socket_, char* name);

struct connection_info_buff {
    size_t capacity;
    size_t size;
    struct connection_info* data;
};

struct connection_info_buff connection_info_buff_create();

bool connection_info_buff_alloc(struct connection_info_buff* buff, size_t new_capacity);

bool connection_info_buff_add(struct connection_info_buff* buff, struct connection_info* info);

void connection_info_buff_free(struct connection_info_buff* buff);

struct connection_info* connection_info_buff_find(struct connection_info_buff* buff, char* name);

bool connection_info_buff_delete(struct connection_info_buff* buff, char* name);