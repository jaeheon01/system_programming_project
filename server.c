#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "shared.h"
#include "camera.h"

int vibration_detected = 0;
int motion_detected = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int step = 0;

void send_alert_to_client(int clnt_sock) {
    char alert_msg[] = "on\n";

    // sunding message to client
    if (write(clnt_sock, alert_msg, strlen(alert_msg)) == -1) {
        perror("[Server] Failed to send alert");
    } else {
        printf("[Server] Alert sent to client: %s\n", alert_msg);
    }
}


int main(int argc, char *argv[]) {
    pthread_t vibration_thread, motion_thread;
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return -1;
    }

    // Create server socket
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) {
        perror("[Server] socket() error");
        return -1;
    }

    // Setting server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    // socket binding
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("[Server] bind() error");
        close(serv_sock);
        return -1;
    }

    // Waiting for client connection
    if (listen(serv_sock, 5) == -1) {
        perror("[Server] listen() error");
        close(serv_sock);
        return -1;
    }

    // Accept client connection
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1) {
        perror("[Server] accept() error");
        close(serv_sock);
        return -1;
    }
    printf("[Server] Client connected\n");

    // create vibrate & motion thread
    if (pthread_create(&vibration_thread, NULL, vibration_sensor, NULL) != 0) {
        fprintf(stderr, "Error creating vibration sensor thread\n");
        close(serv_sock);
        close(clnt_sock);
        return -1;
    }

    if (pthread_create(&motion_thread, NULL, motion_sensor, NULL) != 0) {
        fprintf(stderr, "Error creating motion sensor thread\n");
        close(serv_sock);
        close(clnt_sock);
        return -1;
    }

    while (1) {
        pthread_mutex_lock(&lock);
        while (step != 2) {
            pthread_cond_wait(&cond, &lock);
        }
        if (vibration_detected && motion_detected) {
            printf("[Main] Invade detected by both sensors!\n");
            send_alert_to_client(clnt_sock); // sending alarm to client
            captureImage(); // Image capturing
        } else {
            printf("[Main] No invade detected!\n");
        }
        vibration_detected = 0;
        motion_detected = 0;
        step = 0;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&lock);
    }

    pthread_join(vibration_thread, NULL);
    pthread_join(motion_thread, NULL);
    close(clnt_sock);
    close(serv_sock);
    return 0;
}