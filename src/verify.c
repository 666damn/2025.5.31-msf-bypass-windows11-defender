#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "enc_shellcode.h"

// 解密函数声明，和encrypt对应（逆过程）
#define ROTL(a,b) (((a) << (b)) | ((a) >> (32 - (b))))

void quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    *a += *b; *d ^= *a; *d = ROTL(*d,16);
    *c += *d; *b ^= *c; *b = ROTL(*b,12);
    *a += *b; *d ^= *a; *d = ROTL(*d,8);
    *c += *d; *b ^= *c; *b = ROTL(*b,7);
}

void chacha20_block(uint32_t out[16], const uint32_t in[16]) {
    memcpy(out, in, 64);
    for (int i=0; i<10; i++) {
        quarter_round(&out[0], &out[4], &out[8], &out[12]);
        quarter_round(&out[1], &out[5], &out[9], &out[13]);
        quarter_round(&out[2], &out[6], &out[10], &out[14]);
        quarter_round(&out[3], &out[7], &out[11], &out[15]);
        quarter_round(&out[0], &out[5], &out[10], &out[15]);
        quarter_round(&out[1], &out[6], &out[11], &out[12]);
        quarter_round(&out[2], &out[7], &out[8], &out[13]);
        quarter_round(&out[3], &out[4], &out[9], &out[14]);
    }
    for (int i=0; i<16; i++) out[i] += in[i];
}

void chacha20_decrypt(unsigned char *data, size_t len, const unsigned char key[32], const unsigned char nonce[12]) {
    uint32_t state[16], block[16];
    unsigned char keystream[64];
    uint32_t counter=0;
    size_t offset=0;

    while (len > 0) {
        state[0] = 0x61707865; state[1] = 0x3320646e;
        state[2] = 0x79622d32; state[3] = 0x6b206574;
        for (int i=0; i<8; i++) {
            state[4+i] = (uint32_t)key[i*4] | ((uint32_t)key[i*4+1]<<8) | ((uint32_t)key[i*4+2]<<16) | ((uint32_t)key[i*4+3]<<24);
        }
        state[12] = counter++;
        state[13] = (uint32_t)nonce[0] | ((uint32_t)nonce[1]<<8) | ((uint32_t)nonce[2]<<16) | ((uint32_t)nonce[3]<<24);
        state[14] = (uint32_t)nonce[4] | ((uint32_t)nonce[5]<<8) | ((uint32_t)nonce[6]<<16) | ((uint32_t)nonce[7]<<24);
        state[15] = (uint32_t)nonce[8] | ((uint32_t)nonce[9]<<8) | ((uint32_t)nonce[10]<<16) | ((uint32_t)nonce[11]<<24);

        chacha20_block(block, state);

        for (int i=0; i<16; i++) {
            keystream[i*4+0] = (block[i] >> 0) & 0xFF;
            keystream[i*4+1] = (block[i] >> 8) & 0xFF;
            keystream[i*4+2] = (block[i] >> 16) & 0xFF;
            keystream[i*4+3] = (block[i] >> 24) & 0xFF;
        }

        size_t blocksize = (len > 64) ? 64 : len;
        for (size_t i=0; i<blocksize; i++) {
            data[offset + i] ^= keystream[i];
        }

        offset += blocksize;
        len -= blocksize;
    }
}

// RC4 解密
void rc4_ksa(unsigned char *state, const unsigned char *key, size_t keylen) {
    for (int i=0; i<256; i++) state[i] = i;
    int j=0;
    for (int i=0; i<256; i++) {
        j = (j + state[i] + key[i % keylen]) & 0xFF;
        unsigned char tmp = state[i];
        state[i] = state[j];
        state[j] = tmp;
    }
}

void rc4_prga(unsigned char *state, unsigned char *data, size_t datalen) {
    int i=0,j=0;
    for (size_t k=0; k<datalen; k++) {
        i = (i + 1) & 0xFF;
        j = (j + state[i]) & 0xFF;
        unsigned char tmp = state[i];
        state[i] = state[j];
        state[j] = tmp;
        unsigned char rnd = state[(state[i] + state[j]) & 0xFF];
        data[k] ^= rnd;
    }
}

void rc4_decrypt(unsigned char *data, size_t len, const unsigned char *key, size_t keylen) {
    unsigned char state[256];
    rc4_ksa(state, key, keylen);
    rc4_prga(state, data, len);
}

// XOR 解密
void xor_decrypt(unsigned char *data, size_t len, unsigned char key) {
    for (size_t i=0; i<len; i++) data[i] ^= key;
}

int main() {
    unsigned char *buf = malloc(shellcode_len);
    if (!buf) {
        perror("malloc");
        return 1;
    }
    memcpy(buf, shellcode, shellcode_len);

    unsigned char rc4_key[] = "mysecretkey";
    unsigned char xor_key = 0x5C;
    unsigned char chacha_key[32] = "12345678901234567890123456789012";
    unsigned char chacha_nonce[12] = "123456789012";

    rc4_decrypt(buf, shellcode_len, rc4_key, strlen((char*)rc4_key));
    xor_decrypt(buf, shellcode_len, xor_key);
    chacha20_decrypt(buf, shellcode_len, chacha_key, chacha_nonce);

    printf("[+] Decrypted shellcode first 16 bytes:\n");
    for (int i=0; i<16 && i<shellcode_len; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");

    free(buf);
    return 0;
}
