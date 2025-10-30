#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 4096

static int send_all(int sock, const void *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock, (const char*)buf + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 0;
}

static char* read_line(int sock) {
    char *buf = NULL;
    size_t cap = 0, len = 0;
    char c;
    while (1) {
        ssize_t n = recv(sock, &c, 1, 0);
        if (n <= 0) { free(buf); return NULL; }
        if (len + 1 >= cap) {
            cap = cap ? cap * 2 : 128;
            buf = realloc(buf, cap);
        }
        if (c == '\r') continue;
        if (c == '\n') { buf[len] = '\0'; return buf; }
        buf[len++] = c;
    }
}

/* read n bytes exactly */
static char* read_n(int sock, size_t n) {
    char *buf = malloc(n ? n : 1);
    size_t r = 0;
    while (r < n) {
        ssize_t nn = recv(sock, buf + r, n - r, 0);
        if (nn <= 0) { free(buf); return NULL; }
        r += nn;
    }
    return buf;
}

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char line[512];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) { perror("connect"); return 1; }

    /* read USERNAME prompt */
    char *srv = read_line(sock);
    if (!srv) { close(sock); return 1; }
    printf("%s\n", srv);
    free(srv);

    printf("Enter username: ");
    if (!fgets(line, sizeof(line), stdin)) return 1;
    line[strcspn(line, "\n")] = 0;
    char buf[512];
    snprintf(buf, sizeof(buf), "%s\n", line);
    send_all(sock, buf, strlen(buf));

    srv = read_line(sock);
    if (!srv) { close(sock); return 1; }
    printf("%s\n", srv);
    free(srv);

    printf("Enter password: ");
    if (!fgets(line, sizeof(line), stdin)) return 1;
    line[strcspn(line, "\n")] = 0;
    snprintf(buf, sizeof(buf), "%s\n", line);
    send_all(sock, buf, strlen(buf));

    srv = read_line(sock);
    if (!srv) { close(sock); return 1; }
    printf("%s\n", srv);
    free(srv);

    while (1) {
        printf("\nCommands:\nUPLOAD <local_file>\nDOWNLOAD <remote_file>\nLIST\nDELETE <remote_file>\nEXIT\n> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (strncmp(line, "UPLOAD ", 7) == 0) {
            char *local = line + 7;
            FILE *fp = fopen(local, "rb");
            if (!fp) { printf("Cannot open %s\n", local); continue; }
            struct stat st;
            if (stat(local, &st) != 0) { fclose(fp); printf("stat failed\n"); continue; }
            size_t sz = st.st_size;
            /* send header: UPLOAD <filename> <N>\n */
            char header[512];
            snprintf(header, sizeof(header), "UPLOAD %s %zu\n", local, sz);
            send_all(sock, header, strlen(header));
            /* send file bytes */
            char bufdata[BUF_SIZE];
            size_t r;
            while ((r = fread(bufdata, 1, sizeof(bufdata), fp)) > 0) {
                if (send_all(sock, bufdata, r) < 0) break;
            }
            fclose(fp);
            /* read server response (line) */
            char *resp = read_line(sock);
            if (!resp) { printf("Server disconnected\n"); break; }
            printf("Server: %s\n", resp);
            free(resp);
        }
        else if (strncmp(line, "DOWNLOAD ", 9) == 0) {
            char *remote = line + 9;
            char header[256];
            snprintf(header, sizeof(header), "DOWNLOAD %s\n", remote);
            send_all(sock, header, strlen(header));
            /* expect "SIZE <n>" or "FILE NOT FOUND" */
            char *resp = read_line(sock);
            if (!resp) { printf("Server disconnected\n"); break; }
            if (strncmp(resp, "SIZE ", 5) == 0) {
                size_t n;
                sscanf(resp + 5, "%zu", &n);
                free(resp);
                char *data = read_n(sock, n);
                if (!data) { printf("Failed to read file\n"); break; }
                /* read END\n */
                char *end = read_line(sock);
                /* write to local file with same name */
                FILE *fp = fopen(remote, "wb");
                if (!fp) { printf("Cannot create %s\n", remote); free(data); if (end) free(end); continue; }
                fwrite(data, 1, n, fp);
                fclose(fp);
                free(data);
                if (end) free(end);
                printf("Downloaded %s (%zu bytes)\n", remote, n);
            } else {
                printf("Server: %s\n", resp);
                free(resp);
            }
        }
        else if (strcmp(line, "LIST") == 0) {
            send_all(sock, "LIST\n", 5);
            /* server will send listing as text lines; read until blank prompt or until socket non-blocking? */
            /* For simplicity, read one response line */
            char *resp = read_line(sock);
            if (!resp) { printf("Server disconnected\n"); break; }
            printf("%s\n", resp);
            free(resp);
        }
        else if (strncmp(line, "DELETE ", 7) == 0) {
            char header[256];
            snprintf(header, sizeof(header), "%s\n", line);
            send_all(sock, header, strlen(header));
            char *resp = read_line(sock);
            if (!resp) { printf("Server disconnected\n"); break; }
            printf("Server: %s\n", resp);
            free(resp);
        }
        else if (strcmp(line, "EXIT") == 0) {
            send_all(sock, "EXIT\n", 5);
            break;
        }
        else {
            printf("Unknown command\n");
        }
    }

    close(sock);
    return 0;
}
