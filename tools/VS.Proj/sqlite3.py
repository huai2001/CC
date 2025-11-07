import os
from VCXProj import VCXProj 

LIBSources = [
"include/sqlite3/sqlite3.c"
]
LIBHeaders = [
"include/sqlite3/sqlite3.h"
]

if __name__ == "__main__":
	vcxproj = VCXProj("sqlite3","DynamicLibrary","./")
	vcxproj.OutDir = "..\\bin"
	vcxproj.ImportLibrary = "..\\lib"
	vcxproj.addSource(LIBSources)
	vcxproj.addHeader(LIBHeaders)
	vcxproj.addMacros("Debug",["SQLITE3_EXPORTS","_WINDOWS","_USRDLL"])
	vcxproj.addMacros("Release",["SQLITE3_EXPORTS","_WINDOWS","_USRDLL"])
	vcxproj.build()



