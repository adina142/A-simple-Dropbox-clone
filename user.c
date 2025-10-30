#include "user.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>

typedef struct user_info_t {
    char username[64];
    char password[64];
    struct user_info_t* next;
} user_info_t;

static user_info_t* users = NULL;
static pthread_mutex_t users_lock = PTHREAD_MUTEX_INITIALIZER;

/* Linked list of file locks */
static file_lock_t* file_locks = NULL;
static pthread_mutex_t file_locks_lock = PTHREAD_MUTEX_INITIALIZER;

void user_system_init(void) {
    mkdir("data", 0755);
}

/* Add user or check password */
int user_signup(const char* username, const char* password) {
    pthread_mutex_lock(&users_lock);
    user_info_t* cur = users;
    while (cur) {
        if (strcmp(cur->username, username) == 0) {
            pthread_mutex_unlock(&users_lock);
            return 0; // already exists
        }
        cur = cur->next;
    }
    user_info_t* u = malloc(sizeof(user_info_t));
    strcpy(u->username, username);
    strcpy(u->password, password);
    u->next = users;
    users = u;
    pthread_mutex_unlock(&users_lock);

    char path[256];
    snprintf(path, sizeof(path), "data/%s", username);
    mkdir(path, 0755);
    return 1;
}

int user_login(const char* username, const char* password) {
    pthread_mutex_lock(&users_lock);
    user_info_t* cur = users;
    while (cur) {
        if (strcmp(cur->username, username) == 0 && strcmp(cur->password, password) == 0) {
            pthread_mutex_unlock(&users_lock);
            return 1;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&users_lock);
    return 0;
}

void user_update_storage_usage(const char* username) {
    /* In this simplified phase, we skip real accounting */
    (void)username;
}

/* ----------- FILE LOCK MANAGEMENT ----------- */

file_lock_t* user_acquire_file_lock(const char* username, const char* filename) {
    pthread_mutex_lock(&file_locks_lock);
    file_lock_t* cur = file_locks;
    while (cur) {
        if (strcmp(cur->filename, filename) == 0) {
            cur->refcount++;
            pthread_mutex_unlock(&file_locks_lock);
            pthread_mutex_lock(&cur->lock);
            return cur;
        }
        cur = cur->next;
    }
    /* not found -> create new lock */
    file_lock_t* fl = malloc(sizeof(file_lock_t));
    fl->filename = strdup(filename);
    pthread_mutex_init(&fl->lock, NULL);
    fl->refcount = 1;
    fl->next = file_locks;
    file_locks = fl;
    pthread_mutex_unlock(&file_locks_lock);

    pthread_mutex_lock(&fl->lock);
    return fl;
}

void user_release_file_lock(file_lock_t* fl) {
    if (!fl) return;
    pthread_mutex_unlock(&fl->lock);
}
void user_system_destroy(void) {
    /* Free all user records */
    pthread_mutex_lock(&users_lock);
    user_info_t* u = users;
    while (u) {
        user_info_t* tmp = u->next;
        free(u);
        u = tmp;
    }
    users = NULL;
    pthread_mutex_unlock(&users_lock);
    pthread_mutex_destroy(&users_lock);

    /* Free all file locks */
    pthread_mutex_lock(&file_locks_lock);
    file_lock_t* f = file_locks;
    while (f) {
        file_lock_t* tmp = f->next;
        pthread_mutex_destroy(&f->lock);
        free(f->filename);
        free(f);
        f = tmp;
    }
    file_locks = NULL;
    pthread_mutex_unlock(&file_locks_lock);
    pthread_mutex_destroy(&file_locks_lock);
}
