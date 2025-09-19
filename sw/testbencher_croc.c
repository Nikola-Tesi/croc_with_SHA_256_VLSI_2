// Copyright 2021 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// Author: Nikola Tesic, ETH Zurich
#include "uart.h"
#include "print.h"
#include "timer.h"
#include "gpio.h"
#include "util.h"

#include <stdint.h>
#include <stddef.h>
#include <inttypes.h> 

// Input data for the accelerator (two blocks of 16 uint32_t values)
static uint32_t input_1[32] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x0000002F,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000093
};

static uint32_t values[16] = {
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
};

// Expected outputs for the two iterations
static const uint32_t expected_results[16] = {
    0x3A9AE4E0, 0x5BAF9A09, 0xA0223D7C, 0x25F15A3C,
    0x1459DB85, 0x8CF539BA, 0x4A3A88FF, 0xB7B0F47D,
    0x9541C07D, 0x1959658E, 0x0096D350, 0x3771A8C9,
    0x83547F20, 0x5A7013D3, 0xD57BE8F1, 0xD4325C3A
};

// Hardware peripheral addresses
static volatile uint32_t* acc_in_ptr_addr  = (volatile uint32_t*)0x20000000;
static volatile uint32_t* acc_out_ptr_addr = (volatile uint32_t*)0x20000004;
static volatile uint32_t* acc_start_addr   = (volatile uint32_t*)0x20000008;
static volatile uint32_t* acc_done_flag    = (volatile uint32_t*)0x2000000C;

// Implementation of memcmp
int my_memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    return 0;
}

int main() {
    uart_init();

    for (int iter = 0; iter < 2; ++iter) {

        uint32_t start_cycle, end_cycle, duration_cycle;
        uint32_t wait_counter = 0;

        printf("Inizia:\n");
        uart_write_flush();

        *acc_in_ptr_addr  = (uint32_t)(uintptr_t)&input_1[iter * 16];
        *acc_out_ptr_addr = (uint32_t)(uintptr_t)&values[iter * 8];
        *acc_start_addr = 0x1;

        start_cycle = get_mcycle();

        while (*acc_done_flag != 0x00000001) {
            wait_counter++;
        }
        end_cycle = get_mcycle();

        printf("Finisce.\n");
        uart_write_flush();

        duration_cycle = end_cycle - start_cycle;
        printf("Durata cicli: %x (wait_counter: %u)\n", duration_cycle, wait_counter);
        uart_write_flush();

        if (my_memcmp(&values[iter * 8], &expected_results[iter * 8], 8 * sizeof(uint32_t)) == 0) {
            printf("Successo! L'output corrisponde per l'iterazione %d.\n", iter + 1);
        } else {
            printf("Errore: L'output NON corrisponde per l'iterazione %d.\n", iter + 1);
        }
        uart_write_flush();


        if (*acc_done_flag == 0x00000001) {
            *acc_done_flag = 0;
            printf("Core ha letto l'output e resettato acc_done_flag per l'iterazione %d.\n", iter + 1);
        } else {
            printf("Attenzione: acc_done_flag non era 1 come atteso dopo l'elaborazione!\n");
        }
        printf("---- Fine Iterazione %d ----\n\n", iter + 1);
        uart_write_flush();
    }

    printf("Completate tutte le iterazioni.\n");
    uart_write_flush();

    return 1;
}