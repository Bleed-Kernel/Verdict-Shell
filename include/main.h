#pragma once
#include <stddef.h>
#include <stdint.h>

#define SHELL_MAX_LINE      512
#define SHELL_MAX_ARGS      32
#define SHELL_MAX_PATHS     32
#define SHELL_MAX_PATH_LEN  128
#define SHELL_MAX_SUBST_LEN (2 * 1024 * 1024)
#define SHELL_MAX_STAGES    8
#define SHELL_MAX_OWNED     (SHELL_MAX_STAGES * (SHELL_MAX_ARGS + 4))


typedef enum {
    SEP_END,
    SEP_SEMI,
    SEP_AND,
    SEP_OR,
} shell_sep_t;

typedef struct {
    const char *argv[SHELL_MAX_ARGS + 1];
    int argc;
    const char *in_path;
    const char *out_path;
    const char *err_path;
    int out_append;
    int err_append;
    int err_to_out;     // 2>&1
} shell_cmd_t;

typedef struct {
    shell_cmd_t cmds[SHELL_MAX_STAGES];
    int count;
    const char *pipe_in;    // cmd | <pid> sends the text to that task over ipc
    char *owned[SHELL_MAX_OWNED];
    int owned_count;
} shell_pipeline_t;

extern int shell_last_status;

const char *path_resolve(const char *name);

int shell_read_line(char *buf, size_t max);

// parse the next pipeline off *line and expand it, skip parses without running any $( )
int shell_parse(const char **line, shell_pipeline_t *pl, shell_sep_t *sep, int skip);
void shell_pipeline_free(shell_pipeline_t *pl);

int shell_run_line(const char *line);
char *shell_substitute(const char *inner);

int path_init(void);

// what the terminal passes the shell it starts, so that one stays put in its window
#define SHELL_WINDOW_ARG "--window-shell"
int shell_open_window(void);

void prompt(void);

void shell_sigint_handler(int sig);
int shell_install_signal_handlers(void);