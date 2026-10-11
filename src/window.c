#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syscalls.h>
#include <scheduler.h>
#include <main.h>

#define DESKTOP_PID_FILE "/tmp/crime-scene.pid"
#define DESKTOP_NAME     "crime-scene"
// the desktop is its own terminal
#define TERMINAL_NAME    "crime-scene"

// the pid file can outlive a desktop that crashed, so check the pid in it is still the desktop
static int desktop_running(void) {
    char buf[24] = {0};

    int fd = (int)_open(DESKTOP_PID_FILE, O_RDONLY);
    if (fd < 0)
        return 0;
    long n = _read((uint64_t)fd, buf, sizeof(buf) - 1);
    _close((uint64_t)fd);
    if (n <= 0)
        return 0;

    long pid = strtol(buf, NULL, 10);
    if (pid <= 0)
        return 0;

    user_task_info_t info;
    if (_taskinfo((uint64_t)pid, &info) != 0)
        return 0;
    if (info.state == TASK_ZOMBIE || info.state == TASK_DEAD || info.state == TASK_FREE)
        return 0;

    return strstr(info.name, DESKTOP_NAME) != NULL;
}

int shell_open_window(void) {
    if (!desktop_running())
        return 0;

    const char *terminal = path_resolve(TERMINAL_NAME);
    if (!terminal)
        return 0;

    // the terminal starts a shell of its own in the new window, all we do is get it going.
    // we dont wait for it either, whoever ran us gets their prompt straight back
    pid_t pid = fork();
    if (pid < 0)
        return 0;
    if (pid == 0) {
        char *const argv[] = { (char *)terminal, NULL };
        execv(terminal, argv);
        _exit(127);
    }
    return 1;
}
