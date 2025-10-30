#include "task.h"
#include "user.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

/* Ensure user directory exists */
static void ensure_user_dir(const char* username) {
    char path[512];
    snprintf(path, sizeof(path), "data");
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "data/%s", username);
    mkdir(path, 0755);
}

/* Helper to set result on task safely (allocates copy) */
static void set_result(Task* task, const char* msg) {
    if (task->result_buf) {
        free(task->result_buf);
        task->result_buf = NULL;
        task->result_len = 0;
    }
    task->result_buf = strdup(msg ? msg : "");
    task->result_len = task->result_buf ? strlen(task->result_buf) : 0;
}

/* Actual worker entrypoint called by threadpool */
void handle_task(void* arg) {
    Task* task = (Task*)arg;
    if (!task) return;

    ensure_user_dir(task->username);

    char path[512];
    snprintf(path, sizeof(path), "data/%s/%s", task->username, task->filename);

    /* Acquire per-file lock */
    file_lock_t* fl = user_acquire_file_lock(task->username, task->filename);
    if (!fl) {
        set_result(task, "SERVER ERROR: cannot acquire file lock\n");
        return;
    }

    if (strcmp(task->command, "UPLOAD") == 0) {
        FILE* fp = fopen(path, "wb");
        if (!fp) {
            set_result(task, "UPLOAD FAILED\n");
        } else {
            if (task->upload_buf && task->upload_len > 0) {
                size_t w = fwrite(task->upload_buf, 1, task->upload_len, fp);
                (void)w;
            }
            fclose(fp);
            user_update_storage_usage(task->username);
            set_result(task, "UPLOAD DONE\n");
        }
    }
    else if (strcmp(task->command, "DOWNLOAD") == 0) {
        FILE* fp = fopen(path, "rb");
        if (!fp) {
            set_result(task, "FILE NOT FOUND\n");
        } else {
            fseek(fp, 0, SEEK_END);
            long sz = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            if (sz < 0) sz = 0;
            char* buf = malloc((size_t)sz + 1);
            size_t r = fread(buf, 1, (size_t)sz, fp);
            buf[r] = '\0';
            fclose(fp);

            /* Compose result: header then raw bytes — client expects SIZE <n>\n then raw bytes */
            size_t hdr_sz = 32;
            char hdr[64];
            int n = snprintf(hdr, sizeof(hdr), "SIZE %zu\n", r);
            task->result_buf = malloc(r + n + 8);
            memcpy(task->result_buf, hdr, n);
            memcpy(task->result_buf + n, buf, r);
            task->result_len = n + r;
            free(buf);
            /* Note: do not append END here; client thread sends END after receiving bytes */
        }
    }
    else if (strcmp(task->command, "DELETE") == 0) {
        int rc = remove(path);
        if (rc == 0) {
            user_update_storage_usage(task->username);
            set_result(task, "DELETED\n");
        } else {
            set_result(task, "DELETE FAILED\n");
        }
    }
    else if (strcmp(task->command, "LIST") == 0) {
        char dirpath[512];
        snprintf(dirpath, sizeof(dirpath), "data/%s", task->username);
        DIR* d = opendir(dirpath);
        if (!d) {
            set_result(task, "NO FILES\n");
        } else {
            size_t cap = 1024;
            size_t len = 0;
            char* out = malloc(cap);
            struct dirent* entry;
            while ((entry = readdir(d)) != NULL) {
                if (entry->d_name[0] == '.') continue;
                size_t need = strlen(entry->d_name) + 2;
                if (len + need + 1 > cap) {
                    cap = (cap + need) * 2;
                    out = realloc(out, cap);
                }
                memcpy(out + len, entry->d_name, strlen(entry->d_name));
                len += strlen(entry->d_name);
                out[len++] = '\n';
            }
            closedir(d);
            if (len == 0) {
                free(out);
                set_result(task, "NO FILES\n");
            } else {
                out[len] = '\0';
                task->result_buf = out;
                task->result_len = len;
            }
        }
    }
    else {
        set_result(task, "UNKNOWN COMMAND\n");
    }

    /* release lock */
    user_release_file_lock(fl);

    /* Worker does not free Task; it only fills result_buf/result_len.
       The client thread that created the Task must wait on the cond
       and then send result and free Task memory. */
}
