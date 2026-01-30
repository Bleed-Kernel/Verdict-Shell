#pragma once
#include <stdint.h>

typedef enum {
    TASK_FREE,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_DEAD
} task_state_t;

typedef struct user_task_info {
    uint64_t     id;
    task_state_t state;
    uint32_t     quantum_remaining;
    char         *name;
} user_task_info_t;