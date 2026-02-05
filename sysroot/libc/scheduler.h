#pragma once
#include <stdint.h>

typedef enum {
    TASK_FREE,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_DEAD
} task_state_t;

#define P_KERNEL    0
#define P_USER      1
typedef enum {
    PRIVILEGE_KERNEL,
    PRIVILEGE_USER
} task_privil_t;

typedef struct user_task_info {
    uint64_t        id;
    task_state_t    state;
    task_privil_t   privilege_level;
    uint32_t        quantum_remaining;
    char            name[128];
} user_task_info_t;