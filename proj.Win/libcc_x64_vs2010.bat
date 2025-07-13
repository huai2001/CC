@cls
@echo off
@cls
color 0a

@set path=C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE;%path%

@cls
devenv "libcc_vs2010.vcxproj" /rebuild "Release|x64"
devenv "libcc_vs2010.vcxproj" /rebuild "Debug|x64"
devenv "libcc.widgets.vcxproj" /build "Debug|x64"

pause