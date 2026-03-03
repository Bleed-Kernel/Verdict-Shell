#include <main.h>
#include <string.h>

static char *trim_space(char *s) {
    while (*s == ' ' || *s == '\t')
        s++;

    char *end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t'))
        end--;
    *end = '\0';
    return s;
}

static char *find_separator(char *line) {
    return strchr(line, '|');
}

static int is_digits_only(const char *s) {
    if (!s || !*s)
        return 0;
    for (const char *p = s; *p; p++) {
        if (*p < '0' || *p > '9')
            return 0;
    }
    return 1;
}

static int is_ipc_send_cmd(const char *s) {
    return s && strncmp(s, "ipc-send", 8) == 0 &&
           (s[8] == '\0' || s[8] == ' ' || s[8] == '\t');
}

static int parse_words(char *src, const char **argv, int *argc_out) {
    int argc = 0;
    char *p = src;

    while (*p) {
        while (*p == ' ' || *p == '\t')
            p++;
        if (!*p)
            break;
        if (argc >= SHELL_MAX_ARGS)
            break;

        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        if (*p)
            *p++ = '\0';
    }

    argv[argc] = NULL;
    *argc_out = argc;
    return argc > 0 ? 0 : -1;
}

int shell_parse(char *line, shell_cmd_t *cmd) {
    cmd->argc = 0;
    cmd->pipe_in = NULL;
    cmd->pipe_argc = 0;
    cmd->has_process_pipe = 0;
    cmd->reverse_process_pipe = 0;
    cmd->argv[0] = NULL;
    cmd->pipe_argv[0] = NULL;

    char *sep = find_separator(line);
    if (!sep)
        return parse_words(trim_space(line), cmd->argv, &cmd->argc);

    *sep = '\0';
    char *lhs = trim_space(line);
    char *rhs = trim_space(sep + 1);
    if (*lhs == '\0' || *rhs == '\0')
        return -1;

    // Legacy IPC shortcuts:
    //   taskman | 4
    //   hello | ipc-send 3
    if (is_digits_only(rhs)) {
        cmd->pipe_in = rhs;
        return parse_words(lhs, cmd->argv, &cmd->argc);
    }
    if (is_ipc_send_cmd(rhs)) {
        cmd->pipe_in = lhs;
        return parse_words(rhs, cmd->argv, &cmd->argc);
    }

    if (parse_words(lhs, cmd->argv, &cmd->argc) < 0)
        return -1;
    if (parse_words(rhs, cmd->pipe_argv, &cmd->pipe_argc) < 0)
        return -1;

    cmd->has_process_pipe = 1;
    cmd->reverse_process_pipe = 0;
    return 0;
}
