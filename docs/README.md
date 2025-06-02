# 2025.5.31 use kali msfvenom bypass windows 11 defender. Only work on windows 11. Tested on Windows 10 but failed
用kali生成的msfvenom木马绕过win11的实时保护并且在运行的时候不被查杀
123.exe在其中一台win11被实时保护查杀了
123编译
x86_64-w64-mingw32-gcc loader.c -o 123.exe -mwindows

strip 123.exe

upx --best 123.exe

loader编译
x86_64-w64-mingw32-gcc loader.c -o loader.exe -mwindows -O2 -s -nostartfiles -Wl,-e,mainCRTStartup

strip loader.exe

upx --ultra-brute loader.exe


测试图
123测试
![123 Image](123.png)

loader测试
![Loader Image](loader.png)
