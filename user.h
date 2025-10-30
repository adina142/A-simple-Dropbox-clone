#ifndef USER_H
#define USER_H

#include <pthread.h>

typedef struct file_lock_t {
    char *filename;
    pthread_mutex_t lock;
    int refcount;
    struct file_lock_t *next;
} file_lock_t;

void user_system_init(void);
void user_system_destroy(void);
int user_signup(const char* username, const char* password);
int user_login(const char* username, const char* password);
void user_update_storage_usage(const char* username);

file_lock_t* user_acquire_file_lock(const char* username, const char* filename);
void user_release_file_lock(file_lock_t* fl);

#endif
