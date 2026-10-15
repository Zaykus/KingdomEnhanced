/* version.dll export stubs: jump to the real system version.dll */
    .text
.macro STUB name, idx
    .globl \name
\name:
    jmp *(g_real + 8*\idx)(%rip)
.endm
    STUB GetFileVersionInfoA, 0
    STUB GetFileVersionInfoByHandle, 1
    STUB GetFileVersionInfoExA, 2
    STUB GetFileVersionInfoExW, 3
    STUB GetFileVersionInfoSizeA, 4
    STUB GetFileVersionInfoSizeExA, 5
    STUB GetFileVersionInfoSizeExW, 6
    STUB GetFileVersionInfoSizeW, 7
    STUB GetFileVersionInfoW, 8
    STUB VerFindFileA, 9
    STUB VerFindFileW, 10
    STUB VerInstallFileA, 11
    STUB VerInstallFileW, 12
    STUB VerLanguageNameA, 13
    STUB VerLanguageNameW, 14
    STUB VerQueryValueA, 15
    STUB VerQueryValueW, 16
