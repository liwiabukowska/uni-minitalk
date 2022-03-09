#include "algorithm.h"
#include "buffers.h"
#include "netutils.h"

#include "logs.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

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

char input()
{
    return getchar();
}

int unblock = 0;
int block_check = 0;

void cancel_input()
{
    if (unblock) {
        write(unblock, "", 1); // trzeba zapisac cokolwiek do pipa
        close(unblock);
        unblock = 0;
    }
}

/**
 * @brief czeka na wydarzenie
 *
 * @return int 0 blad 1 nastapil cancel 2 stdin 3 polaczenie
 */
int wait_event(int client_connection)
{
    if (!unblock) {
        // jezeli nie ma pipow to stworz
        int unblock_pipe[2];
        if (pipe(unblock_pipe)) {
            log_error("nie mozna stworzyc pipa cancellowania czekania");
            return 0;
        }
        unblock = unblock_pipe[1];
        block_check = unblock_pipe[0];
    }

    int nfds = unblock > block_check ? unblock : block_check;
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(STDIN_FILENO, &read_set);
    FD_SET(block_check, &read_set);
    FD_SET(client_connection, &read_set);

    int select_status = select(nfds + 1, &read_set, NULL, NULL, NULL);
    if (select_status == -1) {
        log_error("select zwraca blad");
        return 0;
    } else if (select_status == 0) {
        log_info("select timeout");
        return 0;
    } else {
        if (FD_ISSET(STDIN_FILENO, &read_set)) {
            // na stdinie
            return 2;
        } else if (FD_ISSET(block_check, &read_set)) {
            // nastapil cancel wait
            if (block_check) {
                close(block_check);
            }
            block_check = 0;
            return 1;
        } else if (FD_ISSET(client_connection, &read_set)) {
            return 3;
        } else {
            log_error("nie znany wzbudznia selecta");
            return 0;
        }
    }
}

void read_stdin(struct char_buff* buff)
{
    while (true) {
        char c = input();

        if (c == '\n' || c == '\0') {
            char_buff_add(buff, '\0');
            return;
        }
        if (c == (char)EOF) {
            if (buff->size == 0) {
                char_buff_add(buff, (char)EOF);
                return;
            } else {
                // aby nacisniecie ctrld jezeli cos jest napisane
                // nie powodowalo zamkniecia stdina
                // dokladnie tak jak w bashu
                clearerr(stdin);
                continue;
            }
        }

        char_buff_add(buff, c);
    }
}

#include <signal.h>

void sigint_handler(int signal)
{
    printf("\n");
    cancel_input();
}

int main(int argc, char** argv)
{
    signal(SIGINT, sigint_handler);

    if (argc < 2) {
        log_error("musisz podac argument -- nazwe uzytkownika, opcjonalnie [ip serwera, port]");
        return 1;
    }

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    char* host = argc > 3 ? argv[2] : CONN_HOST;
    int port = argc > 3 ? atoi(argv[3]) : CONN_PORT;
    log_info("lacze sie z %s:%i", host, port);
    struct sockaddr_in sock_addr = create_sockaddr(host, port);
    int status_connect = connect(client_socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (status_connect == -1) {
        log_error("nie mozna polaczyc sie do socketu");
        return 1;
    }
    log_info("polaczono: %i", status_connect);

    SSL_CTX* ssl_ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_options(ssl_ctx, SSL_OP_SINGLE_DH_USE);

    SSL* ssl_connection = SSL_new(ssl_ctx);
    SSL_set_fd(ssl_connection, client_socket);
    int ssl_status = SSL_connect(ssl_connection);
    if (ssl_status <= 0) {
        shutdown_openssl(ssl_connection);
        destroy_openssl();
        log_error("nie mozna zaincjalizowac ssl na polaczeniu!: "
                  "stat=%i",
            ssl_status);
        return 1;
    }
    log_info("ssl handshake sukces: %i", ssl_status);

    char my_name[20];
    strncpy(my_name, argv[1], 20);
    my_name[19] = '\0';
    SSL_write(ssl_connection, my_name, 20);

    char response[16];
    SSL_read(ssl_connection, response, 16);
    response[15] = '\0';
    if (strncmp(response, "ok", 16) == 0) {
        log_info("serwer zezwolil na dolaczenie z nazwa %s", my_name);
    } else if (strncmp(response, "no", 16) == 0) {
        log_error("serwer NIE zezwolil na dolaczenie z nazwa %s", my_name);
        return 1;
    } else {
        log_error("nie znane dto!: \'%s\'", response);
        return 1;
    }

    while (true) {
        log_info("podaj adresat:wiadomosc> ");
        int reason = wait_event(client_socket);
        // log_info("zakonczono oczekiwanie w seleccie reason=%i", reason);

        struct char_buff input_message = char_buff_create();
        char_buff_alloc(&input_message, 256);
        if (reason == 0) {
            // blad
            log_error("wystapil blad podczas czytania inputu");
        } else if (reason == 1) {
            // cancel
            log_info("anulowano czekanie na wiadomosc");
            char_buff_add(&input_message, '\0');
        } else if (reason == 2) {
            // stdin`
            read_stdin(&input_message);
            if (input_message.data[0] == (char)EOF) {
                break;
            }

            char new_message[256];
            memset(new_message, 0, 256);

            char* name_search_end = input_message.data + (input_message.size > 20 ? 20 : input_message.size);
            char* name_end = find_substr(input_message.data, name_search_end, ":");

            if (name_end != name_search_end) {
                // *name_end = '\0';
                char* message_text = name_end + 1;
                backward_copy(input_message.data, name_end, new_message);

                char* input_end = input_message.data + input_message.size;
                input_end = message_text + 256 < input_end ? message_text + 256 : input_end;
                backward_copy(message_text, input_end, new_message + 20);

                log_info("wiadomosc zostanie przeslana do: \'%s\'", new_message);
            } else {
                log_info("nie znaleziono nadawcy wiadomosci do wyslania. zostanie odebrana przez wszystkich");

                char* input_end = input_message.data + input_message.size;
                input_end = input_end < input_message.data + 256 ? input_end : input_message.data + 256;
                backward_copy(input_message.data, input_end, new_message + 20);
            }

            new_message[255] = '\0';

            // log_info("wiadomosc zostanie przeslana do: \'%s\'", new_message);
            // log_info("przesylasz wiadomosc o tresci \"%s\"", new_message + 20);

            // swap_char(new_message, new_message + 20, '\0', '#');
            // log_info("message \'%s\'", new_message);
            SSL_write(ssl_connection, new_message, 256);
        } else {
            // otrzymano wiadomosc
            char message[256];
            memset(message, 0, 256);
            int len = SSL_read(ssl_connection, message, 256);
            message[255] = '\0';

            char* message_sender = message;
            char* message_text = message + 20;

            if (len == 0) {
                log_info("serwer sie rozlaczyl");
                break;
            } else {
                log_message("\'%s\' o tresci:\n\"%s\"", message_sender, message_text);
            }
        }
        char_buff_free(&input_message);
    }

    shutdown_openssl(ssl_connection);
    destroy_openssl();
    close(client_socket);

    log_info("wychodze");
    return 0;
}