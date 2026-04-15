# Verdict-Shell (For the Bleed Kernel)
![hero image](https://github.com/Bleed-Kernel/Bleed-Resources/blob/main/Bleed-Taskman.png?raw=true)


Verdict is a shell designed to make interfacing with Bleed Easy.
I want to grow Verdict with the kernel increasing capabilities as the operating system becomes more complex

Verdict is the usermode entrypoint and will controll how usermode starts, for example verdict may start programs or services that must start
when usermode is entered, essentially controlling the machine and how it behaves.

## Theme file

Verdict loads theme colors from `/initrd/etc/verdict/default.conf` on startup.

```ini
primary=212,44,44
secondary=69,133,237
background=0,0,0
```

You can also use hex values:

```ini
primary=#d42c2c
secondary=#4585ed
background=#000000
```
