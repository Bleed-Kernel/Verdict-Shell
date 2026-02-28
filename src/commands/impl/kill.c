#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <commands/commands.h>

typedef struct sig_name {
    int sig;
    const char *name;
} sig_name_t;

static const sig_name_t sig_names[] = {
    { SIGHUP, "HUP" },
    { SIGINT, "INT" },
    { SIGQUIT, "QUIT" },
    { SIGILL, "ILL" },
    { SIGTRAP, "TRAP" },
    { SIGABRT, "ABRT" },
    { SIGBUS, "BUS" },
    { SIGFPE, "FPE" },
    { SIGKILL, "KILL" },
    { SIGUSR1, "USR1" },
    { SIGSEGV, "SEGV" },
    { SIGUSR2, "USR2" },
    { SIGPIPE, "PIPE" },
    { SIGALRM, "ALRM" },
    { SIGTERM, "TERM" },
    { SIGCHLD, "CHLD" },
    { SIGCONT, "CONT" },
    { SIGSTOP, "STOP" },
    { SIGTSTP, "TSTP" },
    { SIGTTIN, "TTIN" },
    { SIGTTOU, "TTOU" },
    { SIGWINCH, "WINCH" },
};

static void print_usage(void) {
    printf("kill: usage: kill [-s sigspec | -n signum | -sigspec] pid ... or kill -l [sigspec]\n");
}

static int parse_positive_int(const char *s) {
    int v = 0;
    if (!s || !*s)
        return -1;
    for (size_t i = 0; s[i]; i++) {
        if (!isdigit((unsigned char)s[i]))
            return -1;
        v = (v * 10) + (s[i] - '0');
    }
    return v;
}

static int parse_pid(const char *s, long *out) {
    int sign = 1;
    long v = 0;

    if (!s || !*s || !out)
        return -1;

    if (*s == '+') {
        s++;
    } else if (*s == '-') {
        sign = -1;
        s++;
    }

    if (!*s)
        return -1;

    for (; *s; s++) {
        if (!isdigit((unsigned char)*s))
            return -1;
        v = (v * 10) + (*s - '0');
    }

    *out = v * sign;
    return 0;
}

static const char *sig_to_name(int sig) {
    for (size_t i = 0; i < sizeof(sig_names) / sizeof(sig_names[0]); i++) {
        if (sig_names[i].sig == sig)
            return sig_names[i].name;
    }
    return NULL;
}

static int parse_sigspec(const char *s, int *out_sig) {
    const char *name = s;
    int sig;

    if (!s || !*s || !out_sig)
        return -1;

    sig = parse_positive_int(s);
    if (sig > 0 && sig < NSIG) {
        *out_sig = sig;
        return 0;
    }

    if (!strncasecmp(name, "SIG", 3))
        name += 3;

    for (size_t i = 0; i < sizeof(sig_names) / sizeof(sig_names[0]); i++) {
        if (!strcasecmp(name, sig_names[i].name)) {
            *out_sig = sig_names[i].sig;
            return 0;
        }
    }

    return -1;
}

static void print_sig_list(void) {
    for (size_t i = 0; i < sizeof(sig_names) / sizeof(sig_names[0]); i++) {
        printf("%s%s", sig_names[i].name,
               (i + 1 == sizeof(sig_names) / sizeof(sig_names[0])) ? "\n" : " ");
    }
}

int cmd_kill(shell_cmd_t *cmd) {
    int sig = SIGTERM;
    int i = 1;
    int any_fail = 0;

    if (!cmd || cmd->argc < 2) {
        print_usage();
        return -1;
    }

    if (!strcmp(cmd->argv[i], "-l")) {
        if (i + 1 >= cmd->argc) {
            print_sig_list();
            return 0;
        }

        int n = parse_positive_int(cmd->argv[i + 1]);
        if (n > 0 && n < NSIG) {
            const char *name = sig_to_name(n);
            if (name) {
                printf("%s\n", name);
                return 0;
            }
        }

        int parsed = 0;
        if (parse_sigspec(cmd->argv[i + 1], &parsed) == 0) {
            printf("%d\n", parsed);
            return 0;
        }

        printf("kill: %s: invalid signal\n", cmd->argv[i + 1]);
        return -1;
    }

    if (!strcmp(cmd->argv[i], "-s")) {
        if (i + 2 >= cmd->argc) {
            print_usage();
            return -1;
        }
        if (parse_sigspec(cmd->argv[i + 1], &sig) < 0) {
            printf("kill: %s: invalid signal\n", cmd->argv[i + 1]);
            return -1;
        }
        i += 2;
    } else if (!strcmp(cmd->argv[i], "-n")) {
        if (i + 2 >= cmd->argc) {
            print_usage();
            return -1;
        }
        sig = parse_positive_int(cmd->argv[i + 1]);
        if (!(sig > 0 && sig < NSIG)) {
            printf("kill: %s: invalid signal\n", cmd->argv[i + 1]);
            return -1;
        }
        i += 2;
    } else if (cmd->argv[i][0] == '-' && cmd->argv[i][1] && isalpha((unsigned char)cmd->argv[i][1])) {
        if (parse_sigspec(cmd->argv[i] + 1, &sig) < 0) {
            printf("kill: %s: invalid signal\n", cmd->argv[i]);
            return -1;
        }
        i += 1;
    }

    if (i >= cmd->argc) {
        print_usage();
        return -1;
    }

    for (; i < cmd->argc; i++) {
        long pid = 0;
        if (parse_pid(cmd->argv[i], &pid) < 0) {
            printf("kill: %s: arguments must be process or job IDs\n", cmd->argv[i]);
            any_fail = 1;
            continue;
        }

        if (kill((pid_t)pid, sig) < 0) {
            perror("kill");
            any_fail = 1;
        }
    }

    return any_fail ? -1 : 0;
}
