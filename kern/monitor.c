/* Simple command-line kernel monitor useful for
 * controlling the kernel and exploring the system interactively. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/env.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/tsc.h>
#include <kern/timer.h>
#include <kern/env.h>
#include <kern/trap.h>
#include <kern/kclock.h>

#define WHITESPACE "\t\r\n "
#define MAXARGS    16

/* Functions implementing monitor commands */
int mon_hello(int argc, char **argv, struct Trapframe *tf);
int mon_help(int argc, char **argv, struct Trapframe *tf);
int mon_kerninfo(int argc, char **argv, struct Trapframe *tf);
int mon_backtrace(int argc, char **argv, struct Trapframe *tf);
int mon_dumpcmos(int argc, char **argv, struct Trapframe *tf);
int mon_start(int argc, char **argv, struct Trapframe *tf);
int mon_stop(int argc, char **argv, struct Trapframe *tf);
int mon_frequency(int argc, char **argv, struct Trapframe *tf);

struct Command {
    const char *name;
    const char *desc;
    /* return -1 to force monitor to exit */
    int (*func)(int argc, char **argv, struct Trapframe *tf);
};

static struct Command commands[] = {
        {"hello", "Display welcome message", mon_hello},
        {"help", "Display this list of commands", mon_help},
        {"kerninfo", "Display information about the kernel", mon_kerninfo},
        {"backtrace", "Print stack backtrace", mon_backtrace},
        {"dumpcmos", "Print CMOS contents", mon_dumpcmos},
        {"timer_start", "Start timer", mon_start},
        {"timer_stop", "Stop timer", mon_stop},
        {"timer_freq", "Count frequency", mon_frequency},
};
#define NCOMMANDS (sizeof(commands) / sizeof(commands[0]))

/* Implementations of basic kernel monitor commands */

int
mon_hello(int argc, char **argv, struct Trapframe *tf) {
    cprintf("Hello:)\n");
    return 0;
}

int
mon_help(int argc, char **argv, struct Trapframe *tf) {
    for (size_t i = 0; i < NCOMMANDS; i++)
        cprintf("%s - %s\n", commands[i].name, commands[i].desc);
    return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf) {
    extern char _head64[], entry[], etext[], edata[], end[];

    cprintf("Special kernel symbols:\n");
    cprintf("  _head64 %16lx (virt)  %16lx (phys)\n", (unsigned long)_head64, (unsigned long)_head64);
    cprintf("  entry   %16lx (virt)  %16lx (phys)\n", (unsigned long)entry, (unsigned long)entry - KERN_BASE_ADDR);
    cprintf("  etext   %16lx (virt)  %16lx (phys)\n", (unsigned long)etext, (unsigned long)etext - KERN_BASE_ADDR);
    cprintf("  edata   %16lx (virt)  %16lx (phys)\n", (unsigned long)edata, (unsigned long)edata - KERN_BASE_ADDR);
    cprintf("  end     %16lx (virt)  %16lx (phys)\n", (unsigned long)end, (unsigned long)end - KERN_BASE_ADDR);
    cprintf("Kernel executable memory footprint: %luKB\n", (unsigned long)ROUNDUP(end - entry, 1024) / 1024);
    return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf) {
    // LAB 2: Your code here
    uint64_t *rbp = 0x0;
    uint64_t rip  = 0x0;

    struct Ripdebuginfo info;
    typedef long unsigned int lui;

    cprintf("Stack backtrace:\n");
    rbp = (uint64_t *)read_rbp();
    rip = rbp[1];

    if (rbp == 0x0 || rip == 0x0) {
        cprintf("JOS: ERR: Couldn't obtain backtrace...\n");
        return -1;
    }

    do {
        rip = rbp[1];
        debuginfo_rip(rip, &info);

        cprintf("  rbp %016lx  rip %016lx\n", (lui)rbp, (lui)rip);
        cprintf("         %.256s:%d: %.*s+%ld\n", info.rip_file, info.rip_line,
                info.rip_fn_namelen, info.rip_fn_name, (rip - info.rip_fn_addr));

        rbp = (uint64_t *)rbp[0];

    } while (rbp);

    return 0;
}

int
mon_dumpcmos(int argc, char **argv, struct Trapframe *tf) {
    // Dump CMOS memory in the following format:
    // 00: 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
    // 10: 00 ..
    // Make sure you understand the values read.
    // Hint: Use cmos_read8()/cmos_write8() functions.
    // LAB 4: Your code here
    size_t offset = 0x0;
    for (size_t i = CMOS_START; i < CMOS_SIZE + CMOS_START; ++i) {
        if (offset % 0x10 == 0) {
            if (offset) {
                cprintf("\n");
            }
            cprintf("%02lx: ", offset);
        }
        cprintf("%02x ", cmos_read8(i));
        offset++;
    }
    cprintf("\n");
    return 0;
}

/* Implement timer_start (mon_start), timer_stop (mon_stop), timer_freq (mon_frequency) commands. */
// LAB 5: Your code here:
int mon_start(int argc, char **argv, struct Trapframe *tf) {
    if (argc != 2) {
        return 1;
    }
    timer_start(argv[1]);
    return 0;
}

int mon_stop(int argc, char **argv, struct Trapframe *tf) {
    timer_stop();
    return 0;
}

int mon_frequency(int argc, char **argv, struct Trapframe *tf) {
    if (argc != 2) {
        return 1;
    }
    timer_cpu_frequency(argv[1]);
    return 0;
}
// LAB 5: end code
/* Kernel monitor command interpreter */

static int
runcmd(char *buf, struct Trapframe *tf) {
    int argc = 0;
    char *argv[MAXARGS];

    argv[0] = NULL;

    /* Parse the command buffer into whitespace-separated arguments */
    for (;;) {
        /* gobble whitespace */
        while (*buf && strchr(WHITESPACE, *buf)) *buf++ = 0;
        if (!*buf) break;

        /* save and scan past next arg */
        if (argc == MAXARGS - 1) {
            cprintf("Too many arguments (max %d)\n", MAXARGS);
            return 0;
        }
        argv[argc++] = buf;
        while (*buf && !strchr(WHITESPACE, *buf)) buf++;
    }
    argv[argc] = NULL;

    /* Lookup and invoke the command */
    if (!argc) return 0;
    for (size_t i = 0; i < NCOMMANDS; i++) {
        if (strcmp(argv[0], commands[i].name) == 0)
            return commands[i].func(argc, argv, tf);
    }

    cprintf("Unknown command '%s'\n", argv[0]);
    return 0;
}

void
monitor(struct Trapframe *tf) {

    cprintf("Welcome to the JOS kernel monitor!\n");
    cprintf("Type 'help' for a list of commands.\n");

    if (tf) print_trapframe(tf);

    char *buf;
    do buf = readline("K> ");
    while (!buf || runcmd(buf, tf) >= 0);
}
