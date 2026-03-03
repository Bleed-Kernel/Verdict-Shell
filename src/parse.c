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

static char *find_ipc_separator(char *line) {
    char *pipe = strchr(line, '|');
    char *gt = strchr(line, '>');

    if (!pipe)
        return gt;
    if (!gt)
        return pipe;
    return (pipe < gt) ? pipe : gt;
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

int shell_parse(char *line, shell_cmd_t *cmd) {
    cmd->argc = 0;
    cmd->pipe_in = NULL;

    char *sep = find_ipc_separator(line);
    char *parse_src = line;

    if (sep) {
        *sep = '\0';
        char *lhs = trim_space(line);
        char *rhs = trim_space(sep + 1);

        if (*rhs == '\0' || *lhs == '\0')
            return -1;

        // Support both:
        //   <text> | ipc-send <pid>
        //   taskman | <pid>
        if (is_digits_only(rhs)) {
            cmd->pipe_in = rhs;
            parse_src = lhs;
        } else {
            cmd->pipe_in = lhs;
            parse_src = rhs;
        }
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
