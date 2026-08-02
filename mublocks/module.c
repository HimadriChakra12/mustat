#define _POSIX_C_SOURCE 200809L

#include "module.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static void mod_clock(Block *b)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(b->output, sizeof(b->output), " %H:%M:%S", tm);
}

static void mod_cpu(Block *b)
{
    static long prev_busy = 0, prev_total = 0;

    FILE *f = fopen("/proc/stat", "r");
    if (!f) { snprintf(b->output, sizeof(b->output), " CPU ?"); return; }

    long user, nice, system, idle, iowait, irq, softirq;
    fscanf(f, "cpu %ld %ld %ld %ld %ld %ld %ld",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(f);

    long busy  = user + nice + system + irq + softirq;
    long total = busy + idle + iowait;
    long dt    = total - prev_total;
    long db    = busy  - prev_busy;

    prev_busy  = busy;
    prev_total = total;

    snprintf(b->output, sizeof(b->output), " CPU %d%%",
             dt > 0 ? (int)(100 * db / dt) : 0);
}

static void mod_mem(Block *b)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) { snprintf(b->output, sizeof(b->output), " ?"); return; }

    long total_kb = 0, available_kb = 0;
    char line[128];

    while (fgets(line, sizeof(line), f)) {
        if      (strncmp(line, "MemTotal:",     9)  == 0) sscanf(line + 9,  "%ld", &total_kb);
        else if (strncmp(line, "MemAvailable:", 13) == 0) sscanf(line + 13, "%ld", &available_kb);
        if (total_kb && available_kb) break;
    }
    fclose(f);

    long used_kb = total_kb - available_kb;
    char used_str[32], total_str[32];

    if (used_kb >= 1024 * 1024)
        snprintf(used_str, sizeof(used_str), "%.1fG", used_kb / (1024.0 * 1024));
    else
        snprintf(used_str, sizeof(used_str), "%ldM", used_kb / 1024);

    if (total_kb >= 1024 * 1024)
        snprintf(total_str, sizeof(total_str), "%.1fG", total_kb / (1024.0 * 1024));
    else
        snprintf(total_str, sizeof(total_str), "%ldM", total_kb / 1024);

    snprintf(b->output, sizeof(b->output), " %s/%s", used_str, total_str);
}

static void mod_script(Block *b)
{
    FILE *fp = popen(b->cmd, "r");
    if (!fp) return;
    fgets(b->output, sizeof(b->output), fp);
    b->output[strcspn(b->output, "\n")] = 0;
    pclose(fp);
}

void block_run(Block *b)
{
    switch (b->type) {
        case MOD_CLOCK:  mod_clock(b);  break;
        case MOD_CPU:    mod_cpu(b);    break;
        case MOD_MEM:    mod_mem(b);    break;
        case MOD_SCRIPT: mod_script(b); break;
    }
}
