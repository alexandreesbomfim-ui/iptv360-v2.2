#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <input/input.h>
#include <usb/usbmain.h>
#include <ppc/cache.h>
#include <time/time.h>
#include "diag.h"

extern char bss_start[];
extern char pagetable_end[];

static uint8_t* g_copy = 0;
static uint32_t g_lo = 0, g_hi = 0;
static int g_shown = 0;

static uint32_t crc32(const uint8_t* p, uint32_t n) {
    uint32_t c = 0xFFFFFFFFu;
    while (n--) {
        c ^= *p++;
        for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1)));
    }
    return ~c;
}

void diag_snapshot() {
    g_lo = ((uint32_t)(uintptr_t)pagetable_end + 127u) & ~127u;
    g_hi = (uint32_t)(uintptr_t)bss_start & ~127u;
    g_copy = (uint8_t*)malloc(g_hi - g_lo);
    if (!g_copy) { printf("diag: sem memoria\n"); return; }
    memcpy(g_copy, (void*)(uintptr_t)g_lo, g_hi - g_lo);
    uint32_t* w = (uint32_t*)0x8003bf00u;
    printf("diag: regiao %08x-%08x crc=%08x\n", g_lo, g_hi,
           crc32((const uint8_t*)(uintptr_t)g_lo, g_hi - g_lo));
    printf("diag: @8003bef0: %08x %08x %08x %08x | %08x\n", w[-4], w[-3], w[-2], w[-1], w[0]);
}

int diag_check(const char* stage, bool restore) {
    if (!g_copy) return -1;
    int n = 0;
    uint32_t first = 0;
    for (uint32_t a = g_lo; a < g_hi; a += 128) {
        if (memcmp((void*)(uintptr_t)a, g_copy + (a - g_lo), 128) != 0) {
            if (n == 0) first = a;
            if (g_shown < 6) {
                uint32_t* c = (uint32_t*)(uintptr_t)a;
                uint32_t* o = (uint32_t*)(g_copy + (a - g_lo));
                int idx = 0;
                while (idx < 31 && c[idx] == o[idx]) ++idx;
                printf("diag[%s] linha %08x mudou: w%d velho=%08x novo=%08x\n",
                       stage, a, idx, o[idx], c[idx]);
                ++g_shown;
            }
            ++n;
            if (restore) {
                memcpy((void*)(uintptr_t)a, g_copy + (a - g_lo), 128);
                memdcbst((void*)(uintptr_t)a, 128);
                memicbi((void*)(uintptr_t)a, 128);
            }
        }
    }
    printf("diag[%s]: %d linhas alteradas%s\n", stage, n, (n && first) ? "" : "");
    return n;
}

void diag_wait_a() {
    printf("diag: aperte A para continuar\n");
    controller_data_s pad, prev;
    memset(&pad, 0, sizeof(pad));
    memset(&prev, 0, sizeof(prev));
    for (;;) {
        usb_do_poll();
        if (get_controller_data(&pad, 0) && pad.a && !prev.a) return;
        prev = pad;
        mdelay(20);
    }
}
