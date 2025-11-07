#include <libcc/alloc.h>
#import <Foundation/Foundation.h>

_CC_API_PUBLIC(void) _cc_get_preferred_languages(tchar_t *buf, size_t length) {
@autoreleasepool {
    NSArray *languages = NSLocale.preferredLanguages;
    size_t numlangs = 0;
    size_t i;

    numlangs = (size_t) [languages count];

    for (i = 0; i < numlangs; i++) {
        NSString *nsstr = [languages objectAtIndex:i];
        size_t rc;

        if (nsstr == nil) {
            break;
        }

        [nsstr getCString:buf maxLength:length encoding:NSASCIIStringEncoding];
        rc = _tcslen(buf);

        if (length <= rc) {
            *buf = '\0';
            break;
        }

        buf += rc;
        length -= rc;

        if (i < (numlangs - 1)) {
            if (length <= 1) {
                break;
            }
            buf[0] = ',';
            buf[1] = '\0';
            buf++;
            length--;
        }
    }
}}