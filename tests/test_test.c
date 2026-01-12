#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libcc.h>

void read_remove_comments(_cc_buf_t *buf) {
	size_t length = 0;
	tchar_t *p = (tchar_t*)buf->bytes;
	tchar_t *endpos = p + (buf->length + 1) / sizeof(tchar_t);
	bool_t is_remove = true;
	tchar_t *ptr = p;
    tchar_t *start_ptr = NULL;

	while (p < endpos) {
		if (is_remove) {
            if (_CC_ISSPACE(*p)) {
                while (_CC_ISSPACE(*p)) {p++;}
            }
            if (*p == _T('/') && (p + 1) < endpos) {
				/*double-slash comments, to end of line.*/
				if (*(p + 1) == _T('/')) {
					while (++p < endpos) {
	                    if (*p == _CC_LF_ || *p == _CC_CR_) {
	                    	p += 1;
	                        break;
	                    }
	                }
                    continue;
				} else if (*(p + 1) == _T('*')) {
					/*multiline comments.*/
					while (++p < endpos) {
	                    if ((*p == _T('*') && *(p + 1) == _T('/'))) {
	                    	p += 2;
	                        break;
	                    }
	                }
                    continue;
				}
			}
		}
        if (start_ptr == NULL) {
            start_ptr = p;
            ptr = p;
        }

        if (*p == '\'' || *p == '"') {
            is_remove = !is_remove;
        }

        *ptr++ = *p++;
	}

    FILE *fp = _tfopen("test2.json", _T("w"));
    assert(fp != NULL);
	length = (ptr - start_ptr);
    fwrite(start_ptr, sizeof(tchar_t), length, fp);
    fclose(fp);

    _cc_free_buf(buf);
}

void test_json_from_file() {
    const char *file_name = "./test.json";
    _cc_buf_t buf;

    if (!_cc_buf_from_file(&buf, file_name)) {
        return;
    }

    read_remove_comments(&buf);
}

int main() {
	tchar_t module[1024];
	_cc_get_module_file_name(main, module, _cc_countof(module));
	printf("module: %s\n", module);
	test_json_from_file();
    return 0;
}

