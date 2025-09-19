#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "uart.h"
#include "print.h"
#include "timer.h"
#include "gpio.h"
#include "util.h"

// ===== FAST STRING COMPARISON =====
int my_strcmp(const char* str1, const char* str2) {
    for (int i = 0; i < 64; i++) {
        if (str1[i] != str2[i]) return 1;
    }
    return 0;
}

static const char expected_hex[] = "3a9ae4e05baf9a09a0223d7c25f15a3c1459db858cf539ba4a3a88ffb7b0f47d";

// ===== AGGRESSIVE INLINE OPTIMIZATIONS =====
#define ROR(x, n) ((x >> n) | (x << (32 - n)))

// Combine operations for better instruction pipelining
static inline uint32_t sha256_sum0(uint32_t rs1) {
    return ROR(rs1, 2) ^ ROR(rs1, 13) ^ ROR(rs1, 22);
}

static inline uint32_t sha256_sum1(uint32_t rs1) {
    return ROR(rs1, 6) ^ ROR(rs1, 11) ^ ROR(rs1, 25);
}

static inline uint32_t sha256_sig0(uint32_t rs1) {
    return ROR(rs1, 7) ^ ROR(rs1, 18) ^ (rs1 >> 3);
}

static inline uint32_t sha256_sig1(uint32_t rs1) {
    return ROR(rs1, 17) ^ ROR(rs1, 19) ^ (rs1 >> 10);
}

// ===== OPTIMIZED K VALUES =====
static inline uint32_t get_k_value(int t) {
    if (t < 16) {
        switch(t) {
            case 0: return 0x428a2f98; case 1: return 0x71374491; case 2: return 0xb5c0fbcf; case 3: return 0xe9b5dba5;
            case 4: return 0x3956c25b; case 5: return 0x59f111f1; case 6: return 0x923f82a4; case 7: return 0xab1c5ed5;
            case 8: return 0xd807aa98; case 9: return 0x12835b01; case 10: return 0x243185be; case 11: return 0x550c7dc3;
            case 12: return 0x72be5d74; case 13: return 0x80deb1fe; case 14: return 0x9bdc06a7; default: return 0xc19bf174;
        }
    } else if (t < 32) {
        switch(t) {
            case 16: return 0xe49b69c1; case 17: return 0xefbe4786; case 18: return 0x0fc19dc6; case 19: return 0x240ca1cc;
            case 20: return 0x2de92c6f; case 21: return 0x4a7484aa; case 22: return 0x5cb0a9dc; case 23: return 0x76f988da;
            case 24: return 0x983e5152; case 25: return 0xa831c66d; case 26: return 0xb00327c8; case 27: return 0xbf597fc7;
            case 28: return 0xc6e00bf3; case 29: return 0xd5a79147; case 30: return 0x06ca6351; default: return 0x14292967;
        }
    } else if (t < 48) {
        switch(t) {
            case 32: return 0x27b70a85; case 33: return 0x2e1b2138; case 34: return 0x4d2c6dfc; case 35: return 0x53380d13;
            case 36: return 0x650a7354; case 37: return 0x766a0abb; case 38: return 0x81c2c92e; case 39: return 0x92722c85;
            case 40: return 0xa2bfe8a1; case 41: return 0xa81a664b; case 42: return 0xc24b8b70; case 43: return 0xc76c51a3;
            case 44: return 0xd192e819; case 45: return 0xd6990624; case 46: return 0xf40e3585; default: return 0x106aa070;
        }
    } else {
        switch(t) {
            case 48: return 0x19a4c116; case 49: return 0x1e376c08; case 50: return 0x2748774c; case 51: return 0x34b0bcb5;
            case 52: return 0x391c0cb3; case 53: return 0x4ed8aa4a; case 54: return 0x5b9cca4f; case 55: return 0x682e6ff3;
            case 56: return 0x748f82ee; case 57: return 0x78a5636f; case 58: return 0x84c87814; case 59: return 0x8cc70208;
            case 60: return 0x90befffa; case 61: return 0xa4506ceb; case 62: return 0xbef9a3f7; default: return 0xc67178f2;
        }
    }
}

// ===== IMPROVED COMPRESSION WITH BETTER PIPELINING =====
void rv32_sha256_compress_optimized(uint32_t H[8], uint32_t W[16]) {
    // Use local variables for better register allocation
    uint32_t a = H[0], b = H[1], c = H[2], d = H[3];
    uint32_t e = H[4], f = H[5], g = H[6], h = H[7];
    
    // Process in blocks of 16 for better instruction cache usage
    // First 16 rounds
    for (int t = 0; t < 16; t++) {
        uint32_t T1 = h + sha256_sum1(e) + (g ^ (e & (f ^ g))) + get_k_value(t) + W[t];
        uint32_t T2 = sha256_sum0(a) + (((a | c) & b) | (c & a));
        h = g; g = f; f = e; e = d + T1; d = c; c = b; b = a; a = T1 + T2;
    }
    
    // Remaining 48 rounds with message schedule
    for (int t = 16; t < 64; t++) {
        int i = t & 15;
        W[i] += sha256_sig1(W[(i + 14) & 15]) + W[(i + 9) & 15] + sha256_sig0(W[(i + 1) & 15]);
        
        uint32_t T1 = h + sha256_sum1(e) + (g ^ (e & (f ^ g))) + get_k_value(t) + W[i];
        uint32_t T2 = sha256_sum0(a) + (((a | c) & b) | (c & a));
        h = g; g = f; f = e; e = d + T1; d = c; c = b; b = a; a = T1 + T2;
    }
    
    H[0] += a; H[1] += b; H[2] += c; H[3] += d;
    H[4] += e; H[5] += f; H[6] += g; H[7] += h;
}

// ===== OPTIMIZED HASH CALCULATION =====
void calculateHashFromData(uint32_t *m1_data, char *computed_hash_str) {
    uint32_t H[8];
    H[0] = 0x6a09e667; H[1] = 0xbb67ae85; H[2] = 0x3c6ef372; H[3] = 0xa54ff53a;
    H[4] = 0x510e527f; H[5] = 0x9b05688c; H[6] = 0x1f83d9ab; H[7] = 0x5be0cd19;
    
    uint32_t W[16];
    
    // First block - manual copy for speed
    W[0] = m1_data[0]; W[1] = m1_data[1]; W[2] = m1_data[2]; W[3] = m1_data[3];
    W[4] = m1_data[4]; W[5] = m1_data[5]; W[6] = m1_data[6]; W[7] = m1_data[7];
    W[8] = m1_data[8]; W[9] = m1_data[9]; W[10] = m1_data[10]; W[11] = m1_data[11];
    W[12] = m1_data[12]; W[13] = m1_data[13]; W[14] = m1_data[14]; W[15] = m1_data[15];
    
    rv32_sha256_compress_optimized(H, W);
    
    // Second block - padding
    W[0] = 0x80000000; W[1] = 0; W[2] = 0; W[3] = 0;
    W[4] = 0; W[5] = 0; W[6] = 0; W[7] = 0;
    W[8] = 0; W[9] = 0; W[10] = 0; W[11] = 0;
    W[12] = 0; W[13] = 0; W[14] = 0; W[15] = 0x00000200;
    
    rv32_sha256_compress_optimized(H, W);
    
    // Optimized hex conversion - unroll inner loop
    for (int i = 0; i < 8; i++) {
        uint32_t word = H[i];
        uint8_t *p = (uint8_t*)&computed_hash_str[i * 8];
        
        uint8_t n = (word >> 28); p[0] = (n < 10) ? '0' + n : 'a' + n - 10;
        n = (word >> 24) & 0xF; p[1] = (n < 10) ? '0' + n : 'a' + n - 10;
        n = (word >> 20) & 0xF; p[2] = (n < 10) ? '0' + n : 'a' + n - 10;
        n = (word >> 16) & 0xF; p[3] = (n < 10) ? '0' + n : 'a' + n - 10;
        n = (word >> 12) & 0xF; p[4] = (n < 10) ? '0' + n : 'a' + n - 10;
        n = (word >> 8) & 0xF;  p[5] = (n < 10) ? '0' + n : 'a' + n - 10;
        n = (word >> 4) & 0xF;  p[6] = (n < 10) ? '0' + n : 'a' + n - 10;
        n = word & 0xF;         p[7] = (n < 10) ? '0' + n : 'a' + n - 10;
    }
    computed_hash_str[64] = '\0';
}

// ===== MAIN FUNCTION =====
int main(int argc, char *argv[]) {
    uart_init();
    
    uint32_t start_cycle, end_cycle, duration_cycle;
    char computed_hash_str[65];
    
    uint32_t m1_data[16];
    m1_data[0] = 0; m1_data[1] = 0; m1_data[2] = 0; m1_data[3] = 0;
    m1_data[4] = 0; m1_data[5] = 0; m1_data[6] = 0; m1_data[7] = 0;
    m1_data[8] = 0; m1_data[9] = 0; m1_data[10] = 0; m1_data[11] = 0;
    m1_data[12] = 0; m1_data[13] = 0; m1_data[14] = 0; m1_data[15] = 0x0000002F;
    
    start_cycle = get_mcycle();
    calculateHashFromData(m1_data, computed_hash_str);
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