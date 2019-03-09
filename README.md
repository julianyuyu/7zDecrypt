# 7zDecrypt

7zDecrypt is a simple Windows decrypting tool for compressed archives, like zip/7z/rar/...
build on top of [7zip](https://www.7-zip.org/)(Ver 19.00). 

## Usage
- 7zDecrypt.exe <archive.ext> (not ready)
- 7zDecrypt.exe -f Dict.txt <archive.ext> (TBD)

## Build
1. download 7zip source code, put the folder of C and CPP in the root folder of 7zDecrypt.
2. Open and build projects with Visual Studio 2017.
3. download 7zip binary and put 7z.dll in the same folder as application.

## Dependency
7zip/7z.dll

