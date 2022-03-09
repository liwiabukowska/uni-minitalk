
#include "algorithm.h"
#include "buffers.h"
#include "logs.h"
#include "netutils.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

void initialize_openssl()
{
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

void destroy_openssl()
{
    ERR_free_strings();
    EVP_cleanup();
}

void shutdown_openssl(SSL* ssl_conn)
{
    SSL_shutdown(ssl_conn);
    SSL_free(ssl_conn);
}

void sigint_handler(int signal)
{
    destroy_openssl();
    log_info("wychodze sygnalem");

    exit(0);
}

bool is_running = true;

SSL_CTX* create_ssl_server_context()
{
    SSL_CTX* ssl_context = SSL_CTX_new(TLS_server_method());
    if (!ssl_context) {
        log_error("ssl_ctx: %i", ssl_context);
        return 0;
    }

    SSL_CTX_set_options(ssl_context, SSL_OP_SINGLE_DH_USE);

    int used_cert = SSL_CTX_use_certificate_file(ssl_context, "data/cert.pem", SSL_FILETYPE_PEM);
    if (used_cert <= 0) {
        log_error("cert: %i", used_cert);
        return 0;
    }

    int used_private_key = SSL_CTX_use_PrivateKey_file(ssl_context, "data/key.pem", SSL_FILETYPE_PEM);
    if (used_private_key <= 0) {
        log_error("private key: %i", used_private_key);
        return 0;
    }

    return ssl_context;
}

int create_listening_socket(char* host, int port)
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        log_error("socket: %i", server_socket);
        return 0;
    }

    struct sockaddr_in sock_addr = create_sockaddr(host, port);
    int bind_status = bind(server_socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (bind_status == -1) {
        log_error("bind: %i", server_socket);
        return 0;
    }

    int listen_status = listen(server_socket, 10);
    if (listen_status < 0) {
        log_error("listen: %i", listen_status);
        return 0;
    }

    return server_socket;
}

void print_lobby(struct connection_info_buff* lobby)
{
    log_info("aktualnie podlaczeni do lobby klienci:");

    if (lobby->size) {
        for (uint64_t i = 0; i < lobby->size; ++i) {
            struct connection_info* info = &lobby->data[i];

            log_info("   \'%s\' -- na fd=%i", info->name, info->client_socket);
        }
    } else {
        log_info("   ---- NIKOGO NIE MA W LOBBY");
    }
}

int main(int argc, char** argv)
{
    initialize_openssl();
    SSL_CTX* ssl_context = create_ssl_server_context();
    if (!ssl_context) {
        log_error("nie mozna bylo zaincjalizowac kontekstu ssl");
        return 1;
    }

    char* host = CONN_HOST;
    int port = argc > 1 ? atoi(argv[1]) : CONN_PORT;
    log_info("ustawiam nasluchiwanie na %s:%i", host, port);
    int server_socket = create_listening_socket(host, port);
    if (!server_socket) {
        log_error("nie mozna bylo zainicjalizowac socketu nasluchujacego");
        return 1;
    }

    // struct timeval tv;
    // tv.tv_sec = 0;
    // tv.tv_usec = 0;

    struct connection_info_buff lobby = connection_info_buff_create();
    connection_info_buff_alloc(&lobby, 10);

    while (true) {

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
        // FD_SET(STDIN_FILENO, &read_fds);
        int read_max_fd = server_socket;

        for (uint64_t i = 0; i < lobby.size; ++i) {
            struct connection_info* info = &lobby.data[i];
            FD_SET(info->client_socket, &read_fds);

            read_max_fd = info->client_socket > read_max_fd ? info->client_socket : read_max_fd;
        }

        log_info("wywoluje select");
        // int sel_num = select(read_max_fd + 1, &read_fds, NULL, NULL, &tv);
        int sel_num = select(read_max_fd + 1, &read_fds, NULL, NULL, NULL);
        log_info("select zwraca %i", sel_num);

        while (sel_num > 0) {
            if (sel_num == -1) {
                log_error("select erorr: %i", sel_num);
                return 1;
            } else if (sel_num == 0) {
                log_info("select timeout!");
                continue;
            } else {
                if (FD_ISSET(server_socket, &read_fds)) {
                    // nawiazano nowe polaczenie -- wydarzenie z server socketa

                    int client_socket = accept(server_socket, NULL, NULL);
                    log_info("serwer zaakceptowal polaczenie: fd=%i", client_socket);

                    SSL* ssl_connection = SSL_new(ssl_context);
                    {
                        SSL_set_fd(ssl_connection, client_socket);
                        int ssl_status = SSL_accept(ssl_connection);
                        if (ssl_status <= 0) {
                            shutdown_openssl(ssl_connection);
                            destroy_openssl();
                            log_error("nie mozna zaincjalizowac ssl na polaczeniu!: "
                                      "status=%i",
                                ssl_status);
                            return 1;
                        }
                        log_info("udalo sie zaakceptowac handshake ssl: fd=%i stat=%i", client_socket, ssl_status);
                    }

                    char name[20];
                    name[19] = '\0';
                    SSL_read(ssl_connection, name, 20);
                    log_info("nowo-polaczony uzytkownik wybral imie \'%s\'", name);

                    if (!connection_info_buff_find(&lobby, name)) {
                        // zezwol na dolaczenie
                        char response[16] = "ok";
                        SSL_write(ssl_connection, response, 16);
                        struct connection_info info = connection_info_create(ssl_connection, client_socket, name);
                        connection_info_buff_add(&lobby, &info);

                        log_info("zezwolono uzytkownikowi dolaczyc");

                        read_max_fd = client_socket > read_max_fd ? client_socket : read_max_fd;
                        FD_SET(client_socket, &read_fds);
                    } else {
                        // odmow dolacznia
                        char response[16] = "no";
                        SSL_write(ssl_connection, response, 16);
                        log_info("odmowiono uzytkownikowi dolaczenia");
                    }

                    print_lobby(&lobby);

                } else {
                    // event z ktoregos z socketow klientow

                    bool found = false;
                    for (uint64_t sender_lobby_index = 0; sender_lobby_index < lobby.size; ++sender_lobby_index) {
                        struct connection_info* info = &lobby.data[sender_lobby_index];

                        if (FD_ISSET(info->client_socket, &read_fds)) {
                            // to od klienta o tym sockecie
                            found = true;

                            char message[256];
                            memset(message, 0, 256);
                            int len = SSL_read(info->ssl_connection, message, 256);
                            // log_info("message \'%s\'", message);

                            if (len == 0) {
                                log_info("klient \'%s\'  sie rozlaczyl -- usuwam go z lobby", info->name);

                                shutdown_openssl(info->ssl_connection);
                                close(info->client_socket);

                                connection_info_buff_delete(&lobby, info->name);
                                print_lobby(&lobby);
                            } else {
                                // wiadomosc tekstowa

                                message[19] = '\0';
                                message[255] = '\0';

                                char message_recipient[20];
                                memset(message_recipient, 0, 20);
                                memcpy(message_recipient, message, 19);

                                // log_info("wiadomosc od: \'%s\'", message_recipient);
                                // log_info("przesylasz wiadomosc o tresci \"%s\"", message_text);

                                memset(message, 0, 20);
                                strncpy(message, info->name, 20); // podmieniam odbiorce na nadawce

                                if (message_recipient[0] != '\0') {
                                    // jest adresowana do kogos

                                    struct connection_info* recipient_info = connection_info_buff_find(&lobby, message_recipient);
                                    if (recipient_info) {
                                        log_info("przesylam wiadomosc od \'%s\'do \'%s\'", info->name, message_recipient);
                                        // log_info("o tresci \"%s\"", message_text);

                                        SSL_write(recipient_info->ssl_connection, message, 256);
                                    } else {
                                        log_error("\'%s\' nie ma takiego odbiorcy w lobby -- ignoruje przekazanie wiadomosci", message_recipient);
                                    }

                                } else {
                                    log_info("przesylam wiadomosc od \'%s\'do wszystkich poza wysylajacym");
                                    for (uint64_t j = 0; j < lobby.size; ++j) {
                                        if (sender_lobby_index != j) {
                                            SSL_write(lobby.data[j].ssl_connection, message, 256);
                                        }
                                    }
                                }
                            }

                            // log_info("zakonczono obsluge wiadomosci od \'%s\'", info->name);
                        }
                    }
                    if (!found) {
                        log_error("nadawcy wiadomosci nie ma w lobby!"); // powinien byc
                    }
                }
            }

            --sel_num;
        }
    }

    connection_info_buff_free(&lobby);

    destroy_openssl();
    close(server_socket);
    // close(client_socket);

    log_info("wychodze");
    return 0;
}

/*
bugi

otrzymuje tylko rozlaczenie od ostatniego polaczonego
klient nie clearuje stdina po anulowaniu
*/