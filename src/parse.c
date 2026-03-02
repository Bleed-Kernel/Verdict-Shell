#include <string.h>
#include <main.h>

static char *trim_space(char *s) {
    while (*s == ' ' || *s == '\t')
        s++;

    char *end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t'))
        end--;
    *end = '\0';

    return s;
}

int shell_parse(char *line, shell_cmd_t *cmd) {
    cmd->argc = 0;
    cmd->pipe_in = NULL;

    char *pipe = strchr(line, '|');
    char *parse_src = line;

    if (pipe) {
        *pipe = '\0';
        char *lhs = trim_space(line);
        char *rhs = trim_space(pipe + 1);

        if (*rhs == '\0')
            return -1;

        if (*lhs != '\0')
            cmd->pipe_in = lhs;

        parse_src = rhs;
    }

    char *p = parse_src;
    while (*p) {
        while (*p == ' ' || *p == '\t')
            p++;

        if (!*p)
            break;

        if (cmd->argc >= SHELL_MAX_ARGS)
            break;

        cmd->argv[cmd->argc++] = p;

        while (*p && *p != ' ' && *p != '\t')
            p++;

        if (*p)
            *p++ = 0;
    }

    cmd->argv[cmd->argc] = 0;
    return cmd->argc > 0 ? 0 : -1;
}
