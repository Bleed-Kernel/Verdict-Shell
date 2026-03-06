#pragma once
#include <stddef.h>
#include <stdint.h>

#define SHELL_MAX_LINE   512
#define SHELL_MAX_ARGS   32
#define SHELL_MAX_PATHS  32
#define SHELL_MAX_PATH_LEN 128

typedef struct {
    const char *argv[SHELL_MAX_ARGS + 1];
    int argc;
    const char *pipe_in;
    const char *pipe_argv[SHELL_MAX_ARGS + 1];
    int pipe_argc;
    int has_process_pipe;
    int reverse_process_pipe;
} shell_cmd_t;

const char *path_resolve(const char *name);

int shell_read_line(char *buf, size_t max);

int shell_parse(char *line, shell_cmd_t *cmd);

int shell_execute(shell_cmd_t *cmd);

int path_init(void);

int builtin_dispatch(shell_cmd_t *cmd);

void prompt(void);

void shell_sigint_handler(int sig);
int shell_install_signal_handlers(void);
