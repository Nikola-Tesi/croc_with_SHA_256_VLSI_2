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

#define uchar unsigned char

#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const uint32_t k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

// Expected hash for the 64-byte message:
static const char expected_hex[] = "3a9ae4e05baf9a09a0223d7c25f15a3c1459db858cf539ba4a3a88ffb7b0f47d";

int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// Processes one 64-byte block (data) and updates the hash_state.
void sha256_block(const uint32_t *data, uint32_t *hash_state) {
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t t1, t2;
    int i;
    
    w[0] = data[0];   w[1] = data[1];   w[2] = data[2];   w[3] = data[3];
    w[4] = data[4];   w[5] = data[5];   w[6] = data[6];   w[7] = data[7];
    w[8] = data[8];   w[9] = data[9];   w[10] = data[10]; w[11] = data[11];
    w[12] = data[12]; w[13] = data[13]; w[14] = data[14]; w[15] = data[15];
    
    for (i = 16; i < 64; i++) {
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
    }
    
    a = hash_state[0]; b = hash_state[1]; c = hash_state[2]; d = hash_state[3];
    e = hash_state[4]; f = hash_state[5]; g = hash_state[6]; h = hash_state[7];
    
    for (i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + k[i] + w[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    hash_state[0] += a; hash_state[1] += b; hash_state[2] += c; hash_state[3] += d;
    hash_state[4] += e; hash_state[5] += f; hash_state[6] += g; hash_state[7] += h;
}

void hash_state_to_hex_string(const uint32_t hash_state[8], char hex_str[65]) {
    int i, j;
    for (i = 0; i < 8; i++) {
        uint32_t word = hash_state[i];
        for (j = 0; j < 4; j++) {
            uchar byte = (word >> (24 - j*8)) & 0xFF;
            hex_str[i*8 + j*2]     = "0123456789abcdef"[byte >> 4];
            hex_str[i*8 + j*2 + 1] = "0123456789abcdef"[byte & 0x0F];
        }
    }
    hex_str[64] = '\0';
}

int main() {
    uart_init();

    char cycle_buffer[17]; 
    uint32_t start_cycle, end_cycle, duration_cycle;
    
    uint32_t current_hash_state[8];
    current_hash_state[0] = 0x6a09e667; current_hash_state[1] = 0xbb67ae85;
    current_hash_state[2] = 0x3c6ef372; current_hash_state[3] = 0xa54ff53a;
    current_hash_state[4] = 0x510e527f; current_hash_state[5] = 0x9b05688c;
    current_hash_state[6] = 0x1f83d9ab; current_hash_state[7] = 0x5be0cd19;

    uint32_t m1_data[16];
    m1_data[0]  = 0x00000000; m1_data[1]  = 0x00000000; m1_data[2]  = 0x00000000; m1_data[3]  = 0x00000000;
    m1_data[4]  = 0x00000000; m1_data[5]  = 0x00000000; m1_data[6]  = 0x00000000; m1_data[7]  = 0x00000000;
    m1_data[8]  = 0x00000000; m1_data[9]  = 0x00000000; m1_data[10] = 0x00000000; m1_data[11] = 0x00000000;
    m1_data[12] = 0x00000000; m1_data[13] = 0x00000000; m1_data[14] = 0x00000000;
    m1_data[15] = 0x0000002F; 

    start_cycle = get_mcycle();
    
    sha256_block(m1_data, current_hash_state);
    
    uint32_t m2_data[16];
    m2_data[0]  = 0x80000000; 
    m2_data[1]  = 0x00000000; m2_data[2]  = 0x00000000; m2_data[3]  = 0x00000000;
    m2_data[4]  = 0x00000000; m2_data[5]  = 0x00000000; m2_data[6]  = 0x00000000; m2_data[7]  = 0x00000000;
    m2_data[8]  = 0x00000000; m2_data[9]  = 0x00000000; m2_data[10] = 0x00000000; m2_data[11] = 0x00000000;
    m2_data[12] = 0x00000000; m2_data[13] = 0x00000000;
    m2_data[14] = 0x00000000; 
    m2_data[15] = 0x00000200; 

    sha256_block(m2_data, current_hash_state);
    
    char computed_hash_str[65];
    hash_state_to_hex_string(current_hash_state, computed_hash_str);

    end_cycle = get_mcycle();

    duration_cycle = end_cycle - start_cycle;

    printf("Duration (hex32): %x\n", duration_cycle);
    uart_write_flush();
    
    if (my_strcmp(computed_hash_str, expected_hex) == 0) {
        printf("MATCH!\n");
    } else {
        printf("NO MATCH\n");
    }
    uart_write_flush();
    
    return 1;
}