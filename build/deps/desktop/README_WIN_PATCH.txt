User Account Control (UAC) on Windows can block running patch.exe
To use patch on windows do the following:

1. Install GnuWin32 patch from https://gnuwin32.sourceforge.net/packages/patch.htm to custom folder, e.g. C:\Programs\GnuWin32

2. Go to <install folder>\bin (e.g. C:\Programs\GnuWin32\bin) and copy patch.exe to pp.exe

3. Add <install folder>\bin to PATH in env-msvc.bat

4. SET PATCH=<install folder>\bin\pp.exe