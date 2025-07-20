/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>
#include <time.h>

#include <libcc.h>
#include <libcc/widgets/widgets.h>

struct {
    _cc_folder_t folder;
    const tchar_t *folder_title;
} folders[] = {
    {_CC_FOLDER_HOME_, _T("Home")},
    {_CC_FOLDER_DESKTOP_, _T("Desktop")},
    {_CC_FOLDER_DOCUMENTS_, _T("Document")},
    {_CC_FOLDER_DOWNLOADS_, _T("Download")},
    {_CC_FOLDER_MUSIC_, _T("Music")},
    {_CC_FOLDER_PICTURES_, _T("Pictures")},
    {_CC_FOLDER_PUBLICSHARE_, _T("Publicshare")},
    {_CC_FOLDER_SAVEDGAMES_, _T("SavedGames")},
    {_CC_FOLDER_SCREENSHOTS_, _T("Screenshots")},
    {_CC_FOLDER_TEMPLATES_, _T("Templates")},
    {_CC_FOLDER_VIDEOS_, _T("Videos")}
};
#define _random()                                                          \
    ((long) (0x7fffffff & ( ((uint32_t) rand() << 16)                      \
                          ^ ((uint32_t) rand() << 8)                       \
                          ^ ((uint32_t) rand()) )))
int main (int argc, char* argv[]) {
    int i,r;
    tchar_t path[_CC_MAX_PATH_];
    time_t t = time(nullptr);
    srand(t);

    _cc_get_base_path(path,_CC_MAX_PATH_);
    _tprintf(_T("BasePath: %s\n"), path);
    _cc_get_executable_path(path,_CC_MAX_PATH_);
    _tprintf(_T("BasePath: %s\n"), path);

    _cc_fpath(path,_CC_MAX_PATH_,"../path");
    _tprintf(_T("fpath: %s\n"), path);

    for (i = 0; i < _cc_countof(folders);i++) {
        _cc_get_folder(folders[i].folder, path, _CC_MAX_PATH_);
        _tprintf(_T("%s: %s\n"), folders[i].folder_title, path);
    }
    // for (i = 0; i < 10;i++) {
    //     r = t;//(int)((float)(i * t) / 123.01230f) % 20000;
    //     _tprintf(_T("_cc_rand(%d): %d , %ld\n"), r, _cc_rand(r) %100,_random() %100);
    // }
    
    system("pause");
    return 0;
}
