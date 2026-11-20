/*
Copywrite (C) stupid idiot known as alen or Astro on discord.
i will hunt you down if you steal my code if you do not credit me
im super duper evil >:) totaly totaly lol

Names of programers contrubuting to this file:
- Alen

*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "relay.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <mstcpip.h>
    #ifndef SIO_UDP_CONNRESET
        #define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
    #endif
    typedef SOCKET sock_t;
    #define BAD_SOCK INVALID_SOCKET
    #define close_sock closesocket
    #define poll WSAPoll
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <poll.h>
    #include <unistd.h>
    typedef int sock_t; // wow uh, its just that simple
    #define BAD_SOCK (-1)
    #define close_sock close
#endif

#define MAX_SESSIONS 256
#define IDLE_TIMEOUT 60

struct session {
    int in_use;
    struct sockaddr_in player;
    sock_t upstream;
    time_t last_seen;
};

static sock_t pub = BAD_SOCK; // very bad sock
static struct session sessions[MAX_SESSIONS];
static struct sockaddr_in server_addr;

static struct session *find_session(const struct sockaddr_in *from) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        struct session *s = &sessions[i];
        if (s->in_use &&
            s->player.sin_addr.s_addr == from->sin_addr.s_addr &&
            s->player.sin_port == from->sin_port)
            return s;
    }
    return NULL;
}

static struct session *new_session(const struct sockaddr_in *from) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        struct session *s = &sessions[i];
        if (s->in_use)
            continue;

        sock_t up = socket(AF_INET, SOCK_DGRAM, 0);
        if (up == BAD_SOCK)
            return NULL;

        if (connect(up, (struct sockaddr *)&server_addr, sizeof server_addr) != 0) {
            close_sock(up);
            return NULL;
        }

        s->in_use = 1;
        s->player = *from;
        s->upstream = up;
        s->last_seen = time(NULL);

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from->sin_addr, ip, sizeof ip);
        printf("[Relay] new session %d: %s:%d\n", i, ip, ntohs(from->sin_port)); // wow so fancy log
        fflush(stdout);
        return s;
    }
    printf("[Relay] full! dropping packet\n");
    fflush(stdout);
    return NULL;
}

static void end_session(struct session *s) {
    printf("[Relay] closed session %d (idle)\n", (int)(s - sessions));
    fflush(stdout);
    close_sock(s->upstream);
    s->in_use = 0;
}

static void *relay_thread(void *arg) {
    char buf[2048];
    struct pollfd fds[MAX_SESSIONS + 1];
    int owner[MAX_SESSIONS + 1];
    time_t last_cleanup = time(NULL);

    for (;;) {
        int count = 0;
        fds[count].fd = pub;
        fds[count].events = POLLIN;
        fds[count].revents = 0;
        owner[count++] = -1;
        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (!sessions[i].in_use)
                continue;
            fds[count].fd = sessions[i].upstream;
            fds[count].events = POLLIN;
            fds[count].revents = 0;
            owner[count++] = i;
        }

        if (poll(fds, count, 1000) < 0)
            continue;

        time_t now = time(NULL);

        if (fds[0].revents & POLLIN) {
            struct sockaddr_in from;
            socklen_t from_len = sizeof from;
            int len = recvfrom(pub, buf, sizeof buf, 0, (struct sockaddr *)&from, &from_len);
            if (len > 0) {
                struct session *s = find_session(&from);
                if (s == NULL)
                    s = new_session(&from);
                if (s != NULL) {
                    s->last_seen = now;
                    send(s->upstream, buf, len, 0);
                }
            }
        }

        for (int f = 1; f < count; f++) {
            if (!(fds[f].revents & POLLIN))
                continue;
            struct session *s = &sessions[owner[f]];
            int len = recv(s->upstream, buf, sizeof buf, 0);
            if (len > 0) {
                s->last_seen = now;
                sendto(pub, buf, len, 0, (struct sockaddr *)&s->player, sizeof s->player);
            }
        }

        if (now != last_cleanup) {
            last_cleanup = now;
            for (int i = 0; i < MAX_SESSIONS; i++)
                if (sessions[i].in_use && now - sessions[i].last_seen > IDLE_TIMEOUT)
                    end_session(&sessions[i]);
        }
    }
    return NULL;
}

int relay_start(int listen_port, int server_port) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return -1;
#endif
    memset(&server_addr, 0, sizeof server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    pub = socket(AF_INET, SOCK_DGRAM, 0);
    if (pub == BAD_SOCK) // BAD SOCK, BAD SOCK!
        return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(listen_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(pub, (struct sockaddr *)&addr, sizeof addr) != 0)
        return -1;
#ifdef _WIN32
    BOOL off = FALSE;
    DWORD ignored;
    WSAIoctl(pub, SIO_UDP_CONNRESET, &off, sizeof off, NULL, 0, &ignored, NULL, NULL);
#endif
    pthread_t t;
    if (pthread_create(&t, NULL, relay_thread, NULL) != 0)
        return -1;
    pthread_detach(t);
    printf("[Relay] listening on UDP %d -> 127.0.0.1:%d\n", listen_port, server_port);
    return 0;

}