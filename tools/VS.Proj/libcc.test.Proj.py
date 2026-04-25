import os
from VCXProj import VCXProj 

Sources = [
	"tests/test_atomic.c"
]
Headers = [
]

if __name__ == "__main__":
	vcxproj = VCXProj("libcc.test","Application","../../proj.Win/")
	vcxproj.OutDir = "..\\bin"
	vcxproj.Subsystem = "Console"

	vcxproj.addIncludePath(["C:\\third-party","C:\\libcc\\include"])
	vcxproj.addLibraryPath(["C:\\third-party\\lib\\$(Platform)","C:\\libcc\\lib\\$(Platform)\\$(Configuration)"])
	vcxproj.addSource(Sources)
	vcxproj.addHeader(Headers)

	Librarys = ["libcc.lib"]
	Macros = ["_WINDOWS","_CONSOLE","_CC_USE_OPENSSL_"]

	vcxproj.addLibrarys("Debug",Librarys)
	vcxproj.addLibrarys("Release",Librarys)
	vcxproj.addMacros("Debug",Macros)
	vcxproj.addMacros("Release",Macros)
	vcxproj.build()



