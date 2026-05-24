#include <stdio.h>
#include <syscalls/open.h>
#include <syscalls/ioctl.h>
#include <syscalls/close.h>
#include <commands/commands.h>
#include <fcntl.h>

int cmd_reboot(shell_cmd_t *cmd){
    (void)cmd;
    int pwr = _open("/dev/power", O_RDWR);
    _ioctl(pwr, 0xEEEEEEEE, NULL);
    _close(pwr);
    return 0;
}