#include <stdio.h>
#include "UpdateBuilder.h"

int32_t updateDirectoryLen = 0;
tchar_t updateDirectory[_CC_MAX_PATH_] = {0};


int32_t sourceDirectoryLen = 0;
tchar_t sourceDirectory[_CC_MAX_PATH_] = {0};

#ifdef __CC_WINDOWS__
//  从 Windows 头文件中排除极少使用的信息
#define WIN32_LEAN_AND_MEAN
// Windows 头文件:
#include <windows.h>
/*
// dllmain.c : 定义 DLL 应用程序的入口点。
BOOL APIENTRY DllMain( HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		_cc_init_sqlite(&sqldelegate);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return true;
}
*/
#else
static __attribute__((constructor)) void _dynamic_attach(void) {
    _cc_init_sqlite(&sqldelegate);
}

static __attribute__((destructor)) void _dynamic_detach(void) {

}
#endif
/*
void DeleteDeepDirectory(const tchar_t *directory) {
    tchar_t sourceFile[_CC_MAX_PATH_] = {0};
    DIR *dpath = nullptr;
    struct dirent *d;
    struct stat stat_buf;
    
    if( (dpath = opendir(directory)) == nullptr) {
        return;
    }
    
    //读取目录
    while ((d = readdir(dpath)) != nullptr) {
        //
        if ((d->d_name[0]=='.' && d->d_name[1] == 0) ||
            (d->d_name[0]=='.' && d->d_name[1] == '.' && d->d_name[2] == 0)) {
            continue;
        }
        
        sourceFile[0] = 0;
        _tcscat(sourceFile,directory);
        _tcscat(sourceFile,_T("/"));
        _tcscat(sourceFile,d->d_name);
        _tstat( sourceFile, &stat_buf);
        
        if (S_ISDIR(stat_buf.st_mode) == 0) {
            //_tprintf("delete %s\n", sourceFile);
            _tunlink(sourceFile);
        } else {
            DeleteDeepDirectory(sourceFile);
            _trmdir(sourceFile);
        }
    }
    closedir(dpath);
}*/

int main(int argc, char const *argv[]) {
    _cc_init_sqlite(&sqldelegate);

    if (argc < 3) {
        return 0;
    }

    if (!_cc_isdir(argv[1])) {
        _cc_logger_warin("%s directory does not exist", argv[1]);
        return 0;
    }
    
    if (!_cc_isdir(argv[2])) {
        _cc_logger_warin("%s directory does not exist", argv[2]);
        return 0;
    }

    _tcsncpy(sourceDirectory, argv[1], _cc_countof(sourceDirectory));
    sourceDirectory[_cc_countof(sourceDirectory) - 1] = 0;
    _tcsncpy(updateDirectory, argv[2], _cc_countof(updateDirectory));
    updateDirectory[_cc_countof(updateDirectory) - 1] = 0;

    sourceDirectoryLen = _tcslen(sourceDirectory);
    updateDirectoryLen = _tcslen(updateDirectory);

    builder_ReloadList();
    builder_UpdateList();
    system("pause");
    return 0;
}