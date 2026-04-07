#include <stdint.h>
#include <stddef.h>
#include "io.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "timer.h"
#include "terminal.h"
#include "debug.h"

static uint32_t kstrlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return (uint32_t)n;
}

static int kstrcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static int kstrncmp(const char *a, const char *b, uint32_t n) {
    while (n-- && *a && *a == *b) { a++; b++; }
    return n == (uint32_t)-1 ? 0 : (unsigned char)*a - (unsigned char)*b;
}

static const char *kskip_spaces(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

#define CMD_BUF_SIZE 128

static void cmd_help(uint8_t dry_run) {
    if (dry_run) return;

    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_println("Commands:");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_println("  help              - show this help");
    terminal_println("  clear             - clear terminal");
    terminal_println("  echo <text>       - print text");
    terminal_println("  reboot            - reboot system");
}

static void cmd_echo(const char *args, uint8_t dry_run) {
    if (dry_run) return;
    terminal_println(args);
}

static void cmd_reboot(void) {
    terminal_println("Rebooting...");
    debug_println("SHELL_REBOOT");
    io_wait();
    outb(0x64, 0xFE);
    io_wait();

    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static int shell_dispatch(const char *line, uint8_t dry_run) {
    line = kskip_spaces(line);

    if (*line == '\0') return 0;

    if (kstrcmp(line, "help") == 0) {
        cmd_help(dry_run);
        debug_println("SHELL_HELP_OK");
        return 1;
    } else if (kstrcmp(line, "clear") == 0) {
        if (!dry_run) terminal_clear();
        debug_println("SHELL_CLEAR_OK");
        return 1;
    } else if (kstrncmp(line, "echo ", 5) == 0) {
        cmd_echo(kskip_spaces(line + 5), dry_run);
        debug_println("SHELL_ECHO_OK");
        return 1;
    } else if (kstrcmp(line, "reboot") == 0) {
        cmd_reboot();
    } else {
        if (!dry_run) {
            terminal_print("Unknown command: ");
            terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            terminal_println(line);
            terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            terminal_println("Type 'help' for commands.");
        }
        return 0;
    }

    return 1;
}

static void shell_run(void) {
    static char cmd_buf[CMD_BUF_SIZE];

    while (1) {
        terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        terminal_print("ubaidos");
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_print("@kernel:~$ ");
        terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

        keyboard_readline_irq(cmd_buf, CMD_BUF_SIZE);

        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        shell_dispatch(cmd_buf, 0);
    }
}

static void print_banner(void) {
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_println("");
    terminal_println("  UbaidOS");
    terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_println("  32-bit protected mode kernel online");
    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_println("  Type 'help' to see commands.");
    terminal_println("");
}

static void run_selftests(void) {
    int ok = 1;

    ok &= (kstrlen("abc") == 3);
    ok &= (kstrcmp("help", "help") == 0);
    ok &= (shell_dispatch("help", 1) == 1);
    ok &= (shell_dispatch("echo test", 1) == 1);
    ok &= (shell_dispatch("clear", 1) == 1);
    ok &= (shell_dispatch("  help", 1) == 1);

    if (ok) {
        debug_println("INPUT_OK");
    } else {
        debug_println("SELFTEST_FAIL");
    }
}

void kernel_main(void) {
    terminal_init();
    debug_init();

    debug_println("BOOT_OK");
    debug_println("KERNEL_OK");
    debug_println("UbaidOS");

    print_banner();

    idt_init();
    pic_remap(0x20, 0x28);
    keyboard_init();
    timer_init(100);

    run_selftests();

    __asm__ volatile ("sti");
    shell_run();

    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
