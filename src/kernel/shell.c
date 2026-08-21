#include "shell.h"
#include "console.h"
#include "printf.h"
#include "kbd.h"
#include "fat32.h"
#include "ata.h"
#include "string.h"
#include "isr.h"

static fat32_t g_fs;

static int blk_read(uint64_t lba, void *buf, uint32_t count, void *ctx) {
    (void)ctx;
    return ata_read_sectors(lba, buf, count);
}

static int blk_write(uint64_t lba, const void *buf, uint32_t count, void *ctx) {
    (void)ctx;
    return ata_write_sectors(lba, buf, count);
}

static void readline(char *buf, int max) {
    int i = 0;
    for (;;) {
        char c = kbd_getc();
        if (c == '\n') {
            buf[i] = 0;
            console_putc('\n');
            return;
        }
        if (c == '\b') {
            if (i > 0) { i--; console_putc('\b'); }
        } else if (i < max - 1) {
            buf[i++] = c;
            console_putc(c);
        }
    }
}

static void dir_cb(const char *name, uint32_t size, void *ud) {
    (void)ud;
    printf("  %s %u bytes\n", name, (unsigned)size);
}

static void cmd_type(const char *name) {
    uint32_t cluster, size;
    if (fat32_lookup(&g_fs, name, &cluster, &size) != 0) {
        printf("File not found: %s\n", name);
        return;
    }
    char buf[512];
    memset(buf, 0, sizeof(buf));
    fat32_read_file(&g_fs, cluster, size, buf);
    printf("%s\n", buf);
}

void shell_run(void) {
    if (fat32_init(&g_fs, blk_read, blk_write, NULL) != 0) {
        printf("[shell] FAT32 init failed\n");
        return;
    }
    printf("Filesystem mounted (FAT32).\n");
    for (;;) {
        console_puts("> ");
        char line[128];
        readline(line, sizeof(line));

        char *cmd = line;
        while (*cmd == ' ') cmd++;
        char *arg = cmd;
        while (*arg && *arg != ' ') arg++;
        if (*arg) { *arg = 0; arg++; while (*arg == ' ') arg++; }

        if (cmd[0] == 0) continue;
        if (strcmp(cmd, "help") == 0) {
            printf("help  cls  echo  time  dir  type  mkfile  del\n");
        } else if (strcmp(cmd, "cls") == 0) {
            console_clear(0);
        } else if (strcmp(cmd, "echo") == 0) {
            printf("%s\n", arg);
        } else if (strcmp(cmd, "time") == 0) {
            printf("uptime: %lu s\n", (unsigned long)(g_ticks / 100));
        } else if (strcmp(cmd, "dir") == 0) {
            fat32_dir_list(&g_fs, dir_cb, NULL);
        } else if (strcmp(cmd, "type") == 0) {
            cmd_type(arg);
        } else if (strcmp(cmd, "mkfile") == 0) {
            if (fat32_create(&g_fs, arg) == 0) printf("Created %s\n", arg);
            else printf("mkfile failed\n");
        } else if (strcmp(cmd, "del") == 0) {
            if (fat32_delete(&g_fs, arg) == 0) printf("Deleted %s\n", arg);
            else printf("del failed\n");
        } else {
            printf("Unknown command: %s\n", cmd);
        }
    }
}
