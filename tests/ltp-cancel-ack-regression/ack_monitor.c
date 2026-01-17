/*
 * ack_monitor.c: Monitor for LTP Cancel Acknowledgment segments
 *
 * This utility listens for LTP Cancel Acknowledgment segments to verify
 * that the LTP engine properly responds to cancel requests.
 *
 * Usage: ack_monitor <listen_port> <timeout_seconds>
 *   listen_port: UDP port to monitor
 *   timeout_seconds: how long to wait for acknowledgments
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

static int g_received_acks = 0;
static int g_running = 1;

/* SDNV decoding */
static int decode_sdnv(const unsigned char *buf, int max_len, unsigned long *value) {
    *value = 0;
    int bytes = 0;

    for (int i = 0; i < max_len && i < 8; i++) {
        *value = (*value << 7) | (buf[i] & 0x7F);
        bytes++;

        if ((buf[i] & 0x80) == 0) {
            return bytes;
        }
    }

    return -1; /* Invalid SDNV */
}

static void handle_timeout(int sig) {
    g_running = 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <listen_port> <timeout_seconds>\n", argv[0]);
        printf("  listen_port: UDP port to monitor for acks\n");
        printf("  timeout_seconds: maximum wait time\n");
        printf("\nExample: %s 2113 10\n", argv[0]);
        return 1;
    }

    int listen_port = atoi(argv[1]);
    int timeout_seconds = atoi(argv[2]);

    /* Create socket */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    /* Bind to listen port */
    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(listen_port);

    if (bind(sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    /* Set up timeout */
    signal(SIGALRM, handle_timeout);
    alarm(timeout_seconds);

    printf("Monitoring port %d for cancel acknowledgments (timeout: %d seconds)\n",
           listen_port, timeout_seconds);

    /* Monitor for packets */
    unsigned char buffer[1024];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    while (g_running) {
        ssize_t received = recvfrom(sock, buffer, sizeof(buffer), 0,
                                   (struct sockaddr*)&sender_addr, &sender_len);

        if (received < 0) {
            if (g_running) {
                perror("recvfrom");
            }
            break;
        }

        if (received < 4) {
            continue; /* Too small to be valid LTP */
        }

        /* Parse LTP header */
        unsigned char version = (buffer[0] >> 4) & 0x0F;
        unsigned char seg_type = buffer[0] & 0x0F;

        if (version != 0) {
            continue; /* Wrong LTP version */
        }

        /* Check if this is a cancel acknowledgment */
        int is_cancel_ack = 0;
        const char *ack_type = NULL;

        if (seg_type == 13) { /* LtpCAS - Cancel by Source Acknowledgment */
            is_cancel_ack = 1;
            ack_type = "CAS";
        } else if (seg_type == 15) { /* LtpCAR - Cancel by Receiver Acknowledgment */
            is_cancel_ack = 1;
            ack_type = "CAR";
        }

        if (is_cancel_ack) {
            /* Decode source engine ID and session number */
            unsigned long source_engine, session_nbr;
            int offset = 1;

            int engine_bytes = decode_sdnv(&buffer[offset], received - offset, &source_engine);
            if (engine_bytes < 0) continue;
            offset += engine_bytes;

            int session_bytes = decode_sdnv(&buffer[offset], received - offset, &session_nbr);
            if (session_bytes < 0) continue;

            g_received_acks++;

            printf("RECEIVED %s: engine=%lu session=%lu from %s:%d (packet #%d)\n",
                   ack_type, source_engine, session_nbr,
                   inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port),
                   g_received_acks);
        }
    }

    close(sock);

    printf("\nSummary: Received %d cancel acknowledgment(s)\n", g_received_acks);

    /* Return success if we received any acks */
    return (g_received_acks > 0) ? 0 : 1;
}
