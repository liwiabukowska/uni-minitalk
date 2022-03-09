#include <arpa/inet.h>

// #include <strings.h>

struct sockaddr_in create_sockaddr(char* host, int port)
{
    struct sockaddr_in socket_address;

    // bzero((char*)&socket_address, sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    inet_aton(host, &socket_address.sin_addr);
    socket_address.sin_port = htons(port);

    return socket_address;
}