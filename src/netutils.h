#pragma once

struct sockaddr_in create_sockaddr(char* host, int port);

#define CONN_PORT 57312
#define CONN_HOST "127.0.0.1"