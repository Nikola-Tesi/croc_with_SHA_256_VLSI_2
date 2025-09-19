// Author: Edward Eldridge 
// Program: SHA-256 Algorithm implementation in C for SoC testing
// Resources: https://github.com/EddieEldridge/SHA256-in-C/blob/master/README.md
// Section Reference: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf

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

#define byteSwap32(x) (((x) >> 24) | (((x)&0x00FF0000) >> 8) | (((x)&0x0000FF00) << 8) | ((x) << 24))
#define byteSwap64(x)                                                      \
	((((x) >> 56) & 0x00000000000000FF) | (((x) >> 40) & 0x000000000000FF00) | \
	 (((x) >> 24) & 0x0000000000FF0000) | (((x) >> 8) & 0x00000000FF000000) |  \
	 (((x) << 8) & 0x000000FF00000000) | (((x) << 24) & 0x0000FF0000000000) |  \
	 (((x) << 40) & 0x00FF000000000000) | (((x) << 56) & 0xFF00000000000000))

// Function declarations
__uint32_t sig0(__uint32_t x);
__uint32_t sig1(__uint32_t x);
__uint32_t rotr(__uint32_t n, __uint16_t x);
__uint32_t shr(__uint32_t n, __uint16_t x);
__uint32_t SIG0(__uint32_t x);
__uint32_t SIG1(__uint32_t x);
__uint32_t Ch(__uint32_t x,__uint32_t y,__uint32_t z);
__uint32_t Maj(__uint32_t x,__uint32_t y,__uint32_t z);
void calculateHashFromData(__uint32_t *data, char *output_str);
void processOneRound(__uint32_t *input_block, __uint32_t *H, int round_num);
int my_strcmp(const char *str1, const char *str2);

// Expected hash for the 64-byte message:
static const char expected_hex[] = "3a9ae4e05baf9a09a0223d7c25f15a3c1459db858cf539ba4a3a88ffb7b0f47d";

// ==== Main ===
int main(int argc, char *argv[]) 
{   
    uart_init();

    uint32_t start_cycle, end_cycle, duration_cycle;
    char computed_hash_str[65]; // 64 chars + null terminator

    // Input data as requested - manual initialization to avoid memcpy
    __uint32_t m1_data[16];
    m1_data[0] = 0x00000000; m1_data[1] = 0x00000000; 
    m1_data[2] = 0x00000000; m1_data[3] = 0x00000000;
    m1_data[4] = 0x00000000; m1_data[5] = 0x00000000; 
    m1_data[6] = 0x00000000; m1_data[7] = 0x00000000;
    m1_data[8] = 0x00000000; m1_data[9] = 0x00000000; 
    m1_data[10] = 0x00000000; m1_data[11] = 0x00000000;
    m1_data[12] = 0x00000000; m1_data[13] = 0x00000000; 
    m1_data[14] = 0x00000000; m1_data[15] = 0x0000002F;

    start_cycle = get_mcycle();

    // Calculate hash from the input data
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

// Simple string comparison function
int my_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Function to process one SHA-256 round
void processOneRound(__uint32_t *input_block, __uint32_t *H, int round_num)
{
    // Declare the K constant as static to avoid stack allocation
    static const __uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    __uint32_t W[64];
    __uint32_t a, b, c, d, e, f, g, h;
    __uint32_t T1, T2;
    int j;

    // Copy input data to message schedule (first 16 words) - manual copy to avoid memcpy
    for(j=0; j<16; j++) {   
        W[j] = input_block[j];  // Data is already big endian
    }

    // Extend the first 16 words into the remaining 48 words W[16..63]
    for (j=16; j<64; j++) {
        W[j] = sig1(W[j-2]) + W[j-7] + sig0(W[j-15]) + W[j-16];
    }

    // Initialize working variables with current hash values - manual assignment
    a=H[0]; b=H[1]; c=H[2]; d=H[3]; 
    e=H[4]; f=H[5]; g=H[6]; h=H[7];

    // Main computation loop (64 iterations)
    for(j = 0; j < 64; j++) {
        T1 = h + SIG1(e) + Ch(e,f,g) + K[j] + W[j];
        T2 = SIG0(a) + Maj(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    // Add this chunk's hash to result so far
    H[0] = a + H[0];
    H[1] = b + H[1];
    H[2] = c + H[2];
    H[3] = d + H[3];
    H[4] = e + H[4];
    H[5] = f + H[5];
    H[6] = g + H[6];
    H[7] = h + H[7];
}

// Function to calculate hash from raw 32-bit data array (2 rounds)
void calculateHashFromData(__uint32_t *data, char *output_str)
{   
    // Initial hash values (SHA-256 constants) - manual initialization
    __uint32_t H[8];
    H[0] = 0x6a09e667; H[1] = 0xbb67ae85; H[2] = 0x3c6ef372; H[3] = 0xa54ff53a;
    H[4] = 0x510e527f; H[5] = 0x9b05688c; H[6] = 0x1f83d9ab; H[7] = 0x5be0cd19;

    // Round 1: Process the input data
    processOneRound(data, H, 1);

    // Round 2: Process the metadata block (padding + length)
    // Manual initialization to avoid memcpy
    __uint32_t metadata_block[16];
    metadata_block[0] = 0x80000000; metadata_block[1] = 0x00000000; 
    metadata_block[2] = 0x00000000; metadata_block[3] = 0x00000000;
    metadata_block[4] = 0x00000000; metadata_block[5] = 0x00000000; 
    metadata_block[6] = 0x00000000; metadata_block[7] = 0x00000000;
    metadata_block[8] = 0x00000000; metadata_block[9] = 0x00000000; 
    metadata_block[10] = 0x00000000; metadata_block[11] = 0x00000000;
    metadata_block[12] = 0x00000000; metadata_block[13] = 0x00000000; 
    metadata_block[14] = 0x00000000; metadata_block[15] = 0x00000200;

    processOneRound(metadata_block, H, 2);
    
    // Format hash as hexadecimal string
    // Use a simple hex conversion instead of sprintf to avoid stdio conflicts
    int pos = 0;
    for(int i = 0; i < 8; i++) {
        uint32_t val = H[i];
        for(int j = 7; j >= 0; j--) {
            uint8_t hex_digit = (val >> (j * 4)) & 0xF;
            output_str[pos++] = (hex_digit < 10) ? ('0' + hex_digit) : ('a' + hex_digit - 10);
        }
    }
    output_str[64] = '\0';  // Null terminator
    
    // Print the hash using SoC printf
    printf("%s\n", output_str);
}

// Section 4.1.2  
// ROTR = Rotate Right 
// SHR = Shift Right
// ROTR_n(x) = (x >> n) | (x << (32-n))
// SHR_n(x) = (x >> n)
__uint32_t sig0(__uint32_t x)
{
    // Section 3.2
	return (rotr(x, 7) ^ rotr(x, 18) ^ shr(x, 3));
};

__uint32_t sig1(__uint32_t x)
{
	return (rotr(x, 17) ^ rotr(x, 19) ^ shr(x, 10));
};

// Rotate bits right
__uint32_t rotr(__uint32_t x, __uint16_t a)
{
	return (x >> a) | (x << (32 - a));
};

// Shift bits right
__uint32_t shr(__uint32_t x, __uint16_t b)
{
	return (x >> b);
};

__uint32_t SIG0(__uint32_t x)
{
	return (rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22));
};

__uint32_t SIG1(__uint32_t x)
{
	return (rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25));
};

// Choose
__uint32_t Ch(__uint32_t x,__uint32_t y,__uint32_t z)
{
	return ((x & y) ^ (~(x)&z));
};

// Majority decision
__uint32_t Maj(__uint32_t x,__uint32_t y,__uint32_t z)
{
	return ((x & y) ^ (x & z) ^ (y & z));
};