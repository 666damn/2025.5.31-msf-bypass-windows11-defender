🛡️ 2025.5.31 – Kali + msfvenom Bypass for Windows 11 Defender
（2025.5.31 使用 Kali + msfvenom 绕过 Windows 11 Defender）

This repository demonstrates how to generate a Windows 11-compatible payload using msfvenom, and execute it via custom C loaders.
Tested successfully on Windows 11.
⚠️ Does NOT work on Windows 10.

本项目展示了如何使用 msfvenom 生成可在 Windows 11 上运行的免杀载荷，并通过自定义 C 编写的 loader 进行执行。
已在 Windows 11 测试成功。
⚠️ 无法在 Windows 10 上运行

⚠️ WARNING
（⚠️ 警告）

The file loader_for_123.c enables startup folder persistence and registry Run key persistence

DO NOT run loader_for_123.c on your host machine

Only test in isolated virtual Windows 11 environments

Cleanup requires manual removal from:

%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\123.exe

HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run

loader_for_123.c 会在系统中设置 启动文件夹持久化 和 注册表自启动持久化

请不要在你本地真实系统上运行 loader_for_123.c

请仅在隔离的虚拟 Windows 11 环境中进行测试

清理需要手动删除以下位置中的持久化项：

%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\123.exe

HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run

🧪 loader.c – Basic Test (No Persistence)
（🧪 loader.c – 基础测试版本，无持久化）

Use loader.c to test memory-only shellcode execution on your own machine.
This version does not create persistence and is safer for local testing.

使用 loader.c 可在本机测试纯内存 shellcode 加载执行。
此版本不含任何持久化机制，适合进行相对安全的本地测试。

## 📷 Screenshots

### `loader_for_123.c` demo
![Loader Structure](../assets/images/loader.png)

### `loader.c` demo
![Example Screenshot](../assets/images/123.png)

## 📌 Disclaimer / 免责声明

This project is intended solely for educational and security research purposes.

- Do not use this code, or any derived executables, on unauthorized systems.
- The author is not responsible for any illegal use or damage caused.
- Always test in isolated virtual environments only.
- By using this code, you agree to comply with all applicable laws and ethical guidelines.

本项目仅用于网络安全教育和研究目的。

- 请勿将本代码或其生成的程序用于任何未经授权的系统。
- 作者对任何非法用途或由此引发的后果概不负责。
- 所有测试请仅在受控虚拟环境中进行。
- 使用本项目即视为同意遵守相关法律法规及道德规范。
