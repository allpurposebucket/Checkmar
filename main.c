#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#define PORT 8888
#define MAX_CLIENTS 30
#define MAX_BUFFER 1025

int main(int argc, char *argv[]) {
    int opt = 1;
    int master_socket, new_socket, client_sockets[MAX_CLIENTS];
    int addrlen, max_clients = MAX_CLIENTS, activity, valread, sd;

    int max_sd;
    struct sockaddr_in address;

    char buffer[MAX_BUFFER];

    fd_set readfds;

    char *message = "ECHO daemon \r\n";

    for (int i = 0; i < max_clients; i++) {
        client_sockets[i] = 0;
    }

    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        return EXIT_FAILURE;
    }

    printf("Listening on port %d\n", PORT);

    if (listen(master_socket, 3) < 0) {
        perror("listen");
        return EXIT_FAILURE;
    }

    addrlen = sizeof(address);
    printf("Waiting for connection...\n");

    while (1) {
        FD_ZERO(&readfds);

        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        for (int i = 0; i < max_clients; i++) {
            sd = client_sockets[i];

            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && errno != EINTR) {
            perror("select");
        }

        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                return EXIT_FAILURE;
            }

            printf("New connection: \n\tFD=%d\n\tIP=%s\n\tport=%d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            if (send(new_socket, message, strlen(message), 0) != strlen(message)) {
                perror("send");
            }

            printf("Message sent.\n");

            for (int i = 0; i < max_clients; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    printf("Hosts connected: %d\n", i);
                    break;
                }
            }
        }

        for (int i = 0; i < max_clients; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) { // Checks to see if client is still connected
                if ((valread = read(sd, buffer, 1024)) == 0) {
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("Host disconnected: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    close(sd);
                    client_sockets[i] = 0;
                }

                else {
                    buffer[valread] = '\0';
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}