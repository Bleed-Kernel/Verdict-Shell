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

static char *find_pipe(char *line) {
    return strchr(line, '|');
}

static char *find_redirect_in(char *line) {
    return strchr(line, '<');
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

static int parse_words(char *src, const char **argv, int *argc_out,
                       int *subst_is_subst, char subst_inner[][SHELL_MAX_LINE],
                       int *subst_count) {
    int argc = 0;
    char *p = src;

    while (*p) {
        while (*p == ' ' || *p == '\t')
            p++;
        if (!*p)
            break;
        if (argc >= SHELL_MAX_ARGS)
            break;

        if (p[0] == '$' && p[1] == '(') {
            char *start = p + 2;
            char *end = strchr(start, ')');
            if (end && subst_count && *subst_count < SHELL_MAX_SUBST) {
                int idx = *subst_count;
                size_t len = (size_t)(end - start);
                if (len >= SHELL_MAX_LINE)
                    len = SHELL_MAX_LINE - 1;
                memcpy(subst_inner[idx], start, len);
                subst_inner[idx][len] = '\0';
                argv[argc] = subst_inner[idx];
                if (subst_is_subst)
                    subst_is_subst[argc] = idx + 1;
                (*subst_count)++;
                argc++;
                p = end + 1;
                continue;
            }
        }

        if (subst_is_subst)
            subst_is_subst[argc] = 0;

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
    cmd->subst_count = 0;

    for (int i = 0; i <= SHELL_MAX_ARGS; i++)
        cmd->subst_is_subst[i] = 0;

    char *redir = find_redirect_in(line);
    if (redir) {
        *redir = '\0';
        char *lhs = trim_space(line);
        char *rhs = trim_space(redir + 1);

        if (*lhs == '\0' || *rhs == '\0')
            return -1;

        if (parse_words(lhs, cmd->argv, &cmd->argc,
                        cmd->subst_is_subst, cmd->subst_inner, &cmd->subst_count) < 0)
            return -1;
        if (parse_words(rhs, cmd->pipe_argv, &cmd->pipe_argc,
                        NULL, cmd->subst_inner, &cmd->subst_count) < 0)
            return -1;

        cmd->has_process_pipe = 1;
        cmd->reverse_process_pipe = 1;
        return 0;
    }

    char *sep = find_pipe(line);
    if (!sep)
        return parse_words(trim_space(line), cmd->argv, &cmd->argc,
                           cmd->subst_is_subst, cmd->subst_inner, &cmd->subst_count);

    *sep = '\0';
    char *lhs = trim_space(line);
    char *rhs = trim_space(sep + 1);
    if (*lhs == '\0' || *rhs == '\0')
        return -1;

    if (is_digits_only(rhs)) {
        cmd->pipe_in = rhs;
        return parse_words(lhs, cmd->argv, &cmd->argc,
                           cmd->subst_is_subst, cmd->subst_inner, &cmd->subst_count);
    }

    if (parse_words(lhs, cmd->argv, &cmd->argc,
                    cmd->subst_is_subst, cmd->subst_inner, &cmd->subst_count) < 0)
        return -1;
    if (parse_words(rhs, cmd->pipe_argv, &cmd->pipe_argc,
                    NULL, cmd->subst_inner, &cmd->subst_count) < 0)
        return -1;

    cmd->has_process_pipe = 1;
    cmd->reverse_process_pipe = 0;
    return 0;
}