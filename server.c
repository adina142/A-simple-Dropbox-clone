#include "queue.h"
#include "threadpool.h"
#include "task.h"
#include "user.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define CLIENT_THREAD_COUNT 4
#define WORKER_THREAD_COUNT 4

// Global Queues and Thread Pools
Queue client_queue;
Queue task_queue;
ThreadPool client_pool;
ThreadPool worker_pool;

int server_running = 1;

/* ===================== CLIENT HANDLER ===================== */
static void* handle_client_session(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);

    char username[64];
    char password[64];
    char command[256];

    // --- Authentication (Sign Up or Login) ---
    send(client_socket, "USERNAME:\n", 11, 0);
    recv(client_socket, username, sizeof(username), 0);
    username[strcspn(username, "\n")] = '\0';

    send(client_socket, "PASSWORD:\n", 11, 0);
    recv(client_socket, password, sizeof(password), 0);
    password[strcspn(password, "\n")] = '\0';

    int result = user_login(username, password);
    if (result == 1)
        send(client_socket, "LOGGED IN\n", 10, 0);
    else if (result == 0)
        send(client_socket, "SIGNED UP\n", 10, 0);
    else
        send(client_socket, "LOGIN FAILED\n", 13, 0);

    const char* menu =
        "\nCommands:\n"
        "UPLOAD <local_file>\n"
        "DOWNLOAD <remote_file>\n"
        "LIST\n"
        "DELETE <remote_file>\n"
        "EXIT\n> ";
    send(client_socket, menu, strlen(menu), 0);

    while (1) {
        memset(command, 0, sizeof(command));
        int n = recv(client_socket, command, sizeof(command) - 1, 0);
        if (n <= 0) break;
        command[strcspn(command, "\n")] = '\0';

        if (strncmp(command, "EXIT", 4) == 0)
            break;

        // Parse command
        Task* t = malloc(sizeof(Task));
        memset(t, 0, sizeof(Task));
        strcpy(t->username, username);
        t->client_socket = client_socket;
        pthread_mutex_init(&t->done_lock, NULL);
        pthread_cond_init(&t->done_cond, NULL);
        t->done = 0;

        char* cmd = strtok(command, " ");
        char* arg2 = strtok(NULL, " ");

        if (!cmd) {
            free(t);
            continue;
        }

        if (strcasecmp(cmd, "UPLOAD") == 0 && arg2) {
            strcpy(t->command, "UPLOAD");
            strcpy(t->filename, arg2);

            // Read file from client (client sends size + data)
            char sizebuf[32];
            recv(client_socket, sizebuf, sizeof(sizebuf), 0);
            size_t sz = atoi(sizebuf);
            t->upload_buf = malloc(sz);
            t->upload_len = sz;
            recv(client_socket, t->upload_buf, sz, 0);
        }
        else if (strcasecmp(cmd, "DOWNLOAD") == 0 && arg2) {
            strcpy(t->command, "DOWNLOAD");
            strcpy(t->filename, arg2);
        }
        else if (strcasecmp(cmd, "LIST") == 0) {
            strcpy(t->command, "LIST");
        }
        else if (strcasecmp(cmd, "DELETE") == 0 && arg2) {
            strcpy(t->command, "DELETE");
            strcpy(t->filename, arg2);
        }
        else {
            send(client_socket, "Invalid command\n> ", 19, 0);
            free(t);
            continue;
        }

        // Enqueue task for workers
        queue_push(&task_queue, t);

        // Wait for result from worker
        pthread_mutex_lock(&t->done_lock);
        while (!t->done)
            pthread_cond_wait(&t->done_cond, &t->done_lock);
        pthread_mutex_unlock(&t->done_lock);

        // Send result back
        if (t->result_buf && t->result_len > 0)
            send(client_socket, t->result_buf, t->result_len, 0);

        // Cleanup
        free(t->upload_buf);
        free(t->result_buf);
        pthread_mutex_destroy(&t->done_lock);
        pthread_cond_destroy(&t->done_cond);
        free(t);

        send(client_socket, menu, strlen(menu), 0);
    }

    close(client_socket);
    return NULL;
}

/* ===================== ACCEPT LOOP ===================== */
static void* accept_loop(void* arg) {
    int server_fd = *(int*)arg;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (server_running) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) continue;
        int* pclient = malloc(sizeof(int));
        *pclient = client_fd;
        queue_push(&client_queue, pclient);
    }

    return NULL;
}

/* ===================== CLIENT THREADPOOL HANDLER ===================== */
static void* client_thread_worker(void* arg) {
    (void)arg;
    while (server_running) {
        int* client_fd_ptr = (int*)queue_pop(&client_queue);
        if (!client_fd_ptr) continue;
        handle_client_session(client_fd_ptr);
    }
    return NULL;
}

/* ===================== MAIN ===================== */
int main() {
    int server_fd;
    struct sockaddr_in addr;

    // --- Setup ---
    queue_init(&client_queue);
    queue_init(&task_queue);
    threadpool_init(&worker_pool, WORKER_THREAD_COUNT, &task_queue, handle_task);

    user_system_init();

    // --- Create server socket ---
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    printf("Server running on port %d...\n", PORT);

    // --- Start accept thread ---
    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, accept_loop, &server_fd);

    // --- Client threadpool (to process accepted sockets) ---
    pthread_t client_threads[CLIENT_THREAD_COUNT];
    for (int i = 0; i < CLIENT_THREAD_COUNT; i++)
        pthread_create(&client_threads[i], NULL, client_thread_worker, NULL);

    // --- Wait for termination signal (Ctrl+C or manual stop) ---
    while (server_running) {
        sleep(1);
    }

    // --- Shutdown ---
    threadpool_shutdown(&worker_pool);
    threadpool_destroy(&worker_pool);

    queue_close(&client_queue);
    queue_close(&task_queue);

    user_system_destroy();

    close(server_fd);
    printf("Server shutdown cleanly.\n");
    return 0;
}
