# 🛡️ 2025.5.31 – Kali + msfvenom Bypass for Windows 11 Defender

This repository demonstrates how to generate a Windows 11-compatible payload using `msfvenom`, and execute it via custom C loaders.  
Tested successfully on **Windows 11**. **⚠️ Does NOT work on Windows 10**.

## ⚠️ WARNING

- The file `loader_for_123.c` enables **startup folder persistence** and **registry Run key persistence**
- **DO NOT** run `loader_for_123.c` on your host machine
- Only test in **isolated virtual Windows 11 environments**
- Cleanup requires manual removal from:
  - `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\123.exe`
  - `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run`

## 🧪 loader.c – Basic Test (No Persistence)

Use `loader.c` to test memory-only shellcode execution on your own machine.  
This version **does not create persistence** and is safer for local testing.

## 📷 Screenshots

### `loader_for_123.c` demo
![Loader Structure](../assets/images/loader.png)

### `loader.c` demo
![Example Screenshot](../assets/images/123.png)
