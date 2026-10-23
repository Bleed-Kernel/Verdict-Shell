#include <main.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} word_t;

static int word_put(word_t *w, const char *s, size_t n) {
    if (w->len + n + 1 > w->cap) {
        size_t cap = w->cap ? w->cap : 64;
        while (w->len + n + 1 > cap)
            cap *= 2;
        char *grown = realloc(w->buf, cap);
        if (!grown)
            return -1;
        w->buf = grown;
        w->cap = cap;
    }

    memcpy(w->buf + w->len, s, n);
    w->len += n;
    w->buf[w->len] = '\0';
    return 0;
}

static int word_putc(word_t *w, char c) {
    return word_put(w, &c, 1);
}

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// characters that end an unquoted word
static int is_op_char(char c) {
    return c == '|' || c == '&' || c == ';' || c == '<' || c == '>';
}

static int is_name_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_name_char(char c) {
    return is_name_start(c) || (c >= '0' && c <= '9');
}

static int syntax_error(const char *what) {
    printf("verdict: %s\n", what);
    return -1;
}

// p sits just past "$(", find the ) that closes it, quotes and nested $( ) included
static const char *find_subst_end(const char *p) {
    int depth = 1;

    while (*p) {
        if (*p == '\\' && p[1]) {
            p += 2;
            continue;
        }
        if (*p == '\'' || *p == '"') {
            char q = *p++;
            while (*p && *p != q) {
                if (q == '"' && *p == '\\' && p[1])
                    p++;
                p++;
            }
            if (!*p)
                return NULL;
            p++;
            continue;
        }
        if (*p == '(')
            depth++;
        else if (*p == ')' && --depth == 0)
            return p;
        p++;
    }

    return NULL;
}

static long expand_dollar(const char *p, word_t *w, int skip) {
    if (p[1] == '(') {
        const char *end = find_subst_end(p + 2);
        if (!end)
            return syntax_error("unterminated $(");

        if (!skip) {
            char *inner = strndup(p + 2, (size_t)(end - (p + 2)));
            if (!inner)
                return -1;

            char *out = shell_substitute(inner);
            free(inner);
            if (out) {
                int r = word_put(w, out, strlen(out));
                free(out);
                if (r < 0)
                    return -1;
            }
        }
        return end + 1 - p;
    }

    if (p[1] == '?' || p[1] == '$') {
        char num[24];
        snprintf(num, sizeof(num), "%d", p[1] == '?' ? shell_last_status : (int)getpid());
        if (word_put(w, num, strlen(num)) < 0)
            return -1;
        return 2;
    }

    const char *name = p + 1;
    size_t len = 0;
    long eaten;

    if (p[1] == '{') {
        name = p + 2;
        while (is_name_char(name[len]))
            len++;
        if (name[len] != '}' || len == 0)
            return syntax_error("bad ${ }");
        eaten = 3 + (long)len;
    } else {
        if (!is_name_start(p[1]))
            return word_putc(w, '$') < 0 ? -1 : 1;
        while (is_name_char(name[len]))
            len++;
        eaten = 1 + (long)len;
    }

    char key[128];
    if (len >= sizeof(key))
        len = sizeof(key) - 1;
    memcpy(key, name, len);
    key[len] = '\0';

    const char *val = getenv(key);
    if (val && word_put(w, val, strlen(val)) < 0)
        return -1;
    return eaten;
}

static int read_word(const char **pp, char **out, int skip) {
    const char *p = *pp;
    word_t w = {0};
    int quoted = 0;
    long n;

    *out = NULL;

    if (*p == '~' && (p[1] == '/' || !p[1] || is_space(p[1]) || is_op_char(p[1]))) {
        const char *home = getenv("HOME");
        if (!home)
            home = "/";
        if (word_put(&w, home, strlen(home)) < 0)
            goto fail;
        p++;
    }

    while (*p && !is_space(*p) && !is_op_char(*p)) {
        if (*p == '\\') {
            if (p[1] && word_putc(&w, p[1]) < 0)
                goto fail;
            p += p[1] ? 2 : 1;
        } else if (*p == '\'') {
            const char *end = strchr(p + 1, '\'');
            if (!end) {
                syntax_error("unterminated '");
                goto fail;
            }
            if (word_put(&w, p + 1, (size_t)(end - p - 1)) < 0)
                goto fail;
            quoted = 1;
            p = end + 1;
        } else if (*p == '"') {
            p++;
            quoted = 1;
            while (*p && *p != '"') {
                if (*p == '\\' && (p[1] == '"' || p[1] == '\\' || p[1] == '$')) {
                    if (word_putc(&w, p[1]) < 0)
                        goto fail;
                    p += 2;
                } else if (*p == '$') {
                    if ((n = expand_dollar(p, &w, skip)) < 0)
                        goto fail;
                    p += n;
                } else if (word_putc(&w, *p++) < 0) {
                    goto fail;
                }
            }
            if (*p != '"') {
                syntax_error("unterminated \"");
                goto fail;
            }
            p++;
        } else if (*p == '$') {
            if ((n = expand_dollar(p, &w, skip)) < 0)
                goto fail;
            p += n;
        } else if (word_putc(&w, *p++) < 0) {
            goto fail;
        }
    }

    *pp = p;

    if (!w.len && !quoted) {
        free(w.buf);
        return 0;
    }
    if (!w.buf && word_put(&w, "", 0) < 0)
        goto fail;

    *out = w.buf;
    return 1;

fail:
    free(w.buf);
    return -1;
}

static int own(shell_pipeline_t *pl, char *s) {
    if (pl->owned_count >= SHELL_MAX_OWNED) {
        free(s);
        return syntax_error("line too long");
    }
    pl->owned[pl->owned_count++] = s;
    return 0;
}

static const char *ipc_pipe_target(const char *p) {
    while (is_space(*p))
        p++;

    const char *d = p;
    while (*d >= '0' && *d <= '9')
        d++;
    if (d == p)
        return NULL;

    const char *rest = d;
    while (is_space(*rest))
        rest++;
    return (!*rest || *rest == ';' || *rest == '&' || *rest == '|') ? d : NULL;
}

void shell_pipeline_free(shell_pipeline_t *pl) {
    for (int i = 0; i < pl->owned_count; i++)
        free(pl->owned[i]);
    pl->owned_count = 0;
    pl->count = 0;
}

int shell_parse(const char **line, shell_pipeline_t *pl, shell_sep_t *sep, int skip) {
    const char *p = *line;

    memset(pl, 0, sizeof(*pl));
    pl->count = 1;
    shell_cmd_t *cmd = &pl->cmds[0];
    *sep = SEP_END;

    for (;;) {
        while (is_space(*p))
            p++;

        if (!*p || *p == '#') {
            p += strlen(p);
            break;
        }

        if (p[0] == '&' && p[1] == '&') { *sep = SEP_AND; p += 2; break; }
        if (p[0] == '|' && p[1] == '|') { *sep = SEP_OR; p += 2; break; }
        if (*p == ';') { *sep = SEP_SEMI; p++; break; }
        if (*p == '&') {
            syntax_error("background jobs arent supported");
            goto fail;
        }

        if (*p == '|') {
            p++;
            if (!cmd->argc) {
                syntax_error("missing command before |");
                goto fail;
            }

            const char *end = ipc_pipe_target(p);
            if (end) {
                const char *start = p;
                while (is_space(*start))
                    start++;
                char *pid = strndup(start, (size_t)(end - start));
                if (!pid || own(pl, pid) < 0)
                    goto fail;
                pl->pipe_in = pid;
                p = end;
                continue;
            }

            if (pl->count >= SHELL_MAX_STAGES) {
                syntax_error("too many pipes");
                goto fail;
            }
            cmd = &pl->cmds[pl->count++];
            continue;
        }

        const char **target = NULL;
        int *append = NULL;
        if (p[0] == '2' && p[1] == '>' && p[2] == '&' && p[3] == '1') {
            cmd->err_to_out = 1;
            p += 4;
            continue;
        } else if (p[0] == '2' && p[1] == '>') {
            target = &cmd->err_path;
            append = &cmd->err_append;
            p += 2;
        } else if (*p == '>') {
            target = &cmd->out_path;
            append = &cmd->out_append;
            p++;
        } else if (*p == '<') {
            target = &cmd->in_path;
            p++;
        }

        char *word = NULL;
        if (target) {
            if (append && *p == '>') {
                *append = 1;
                p++;
            }
            while (is_space(*p))
                p++;

            int r = read_word(&p, &word, skip);
            if (r < 0)
                goto fail;
            if (r == 0) {
                syntax_error("redirect needs a file");
                goto fail;
            }
            if (own(pl, word) < 0)
                goto fail;
            *target = word;
            continue;
        }

        int r = read_word(&p, &word, skip);
        if (r < 0)
            goto fail;
        if (r == 0)
            continue;
        if (own(pl, word) < 0)
            goto fail;
        if (cmd->argc >= SHELL_MAX_ARGS) {
            syntax_error("too many arguments");
            goto fail;
        }
        cmd->argv[cmd->argc++] = word;
        cmd->argv[cmd->argc] = NULL;
    }

    if (pl->count > 1 && !cmd->argc) {
        syntax_error("missing command after |");
        goto fail;
    }
    if (pl->pipe_in && pl->count > 1) {
        syntax_error("an ipc pipe only works on a single command");
        goto fail;
    }
    if (!pl->cmds[0].argc)
        pl->count = 0;
    if (!pl->count && (*sep == SEP_AND || *sep == SEP_OR)) {
        syntax_error("missing command before && or ||");
        goto fail;
    }

    *line = p;
    return 0;

fail:
    shell_pipeline_free(pl);
    return -1;
}