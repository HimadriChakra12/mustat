#define _POSIX_C_SOURCE 200809L

#include "module.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ── clock ────────────────────────────────────────────────── */

static void mod_clock(Module *m)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(m->output, sizeof(m->output), " %H:%M:%S", tm);
}

/* ── cpu ──────────────────────────────────────────────────── */

static void mod_cpu(Module *m)
{
    static long prev_busy = 0, prev_total = 0;

    FILE *f = fopen("/proc/stat", "r");
    if (!f) { snprintf(m->output, sizeof(m->output), " CPU ?"); return; }

    long user, nice, system, idle, iowait, irq, softirq;
    fscanf(f, "cpu %ld %ld %ld %ld %ld %ld %ld",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(f);

    long busy  = user + nice + system + irq + softirq;
    long total = busy + idle + iowait;

    long dt = total - prev_total;
    long db = busy  - prev_busy;

    int usage = (dt > 0) ? (int)(100 * db / dt) : 0;

    prev_busy  = busy;
    prev_total = total;

    snprintf(m->output, sizeof(m->output), " CPU %d%%", usage);
}

/* ── mem ──────────────────────────────────────────────────── */

static void mod_mem(Module *m)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) { snprintf(m->output, sizeof(m->output), " ? "); return; }

    long total_kb = 0, available_kb = 0;
    char line[128];

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0)
            sscanf(line + 9, "%ld", &total_kb);
        else if (strncmp(line, "MemAvailable:", 13) == 0)
            sscanf(line + 13, "%ld", &available_kb);
        if (total_kb && available_kb) break;
    }
    fclose(f);

    long used_kb = total_kb - available_kb;

    /* convert to human-readable (MiB / GiB) */
    char used_str[32], total_str[32];

    if (used_kb >= 1024 * 1024)
        snprintf(used_str, sizeof(used_str), "%.1fG", used_kb / (1024.0 * 1024));
    else
        snprintf(used_str, sizeof(used_str), "%ldM", used_kb / 1024);

    if (total_kb >= 1024 * 1024)
        snprintf(total_str, sizeof(total_str), "%.1fG", total_kb / (1024.0 * 1024));
    else
        snprintf(total_str, sizeof(total_str), "%ldM", total_kb / 1024);

    snprintf(m->output, sizeof(m->output), " %s/%s", used_str, total_str);
}

/* ── script fallback ──────────────────────────────────────── */

static void mod_script(Module *m)
{
    FILE *fp = popen(m->script, "r");
    if (!fp) return;
    fgets(m->output, sizeof(m->output), fp);
    m->output[strcspn(m->output, "\n")] = 0;
    pclose(fp);
}

/* ── dispatcher ───────────────────────────────────────────── */

void module_run(Module *m)
{
    switch (m->type) {
        case MOD_CLOCK:  mod_clock(m);  break;
        case MOD_CPU:    mod_cpu(m);    break;
        case MOD_MEM:    mod_mem(m);    break;
        case MOD_SCRIPT: mod_script(m); break;
        default: break;
    }
}
