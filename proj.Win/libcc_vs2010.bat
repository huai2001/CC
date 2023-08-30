@cls
@echo off
@cls
color 0a

@set path=D:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE;%path%

@cls
devenv "libcc_vs2010.vcxproj" /rebuild Release
devenv "libcc_vs2010.vcxproj" /rebuild debug
devenv "libcc.widgets.vcxproj" /rebuild debug

pause;