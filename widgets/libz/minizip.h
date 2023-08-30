
#ifndef _C_CC_LIBZ_H_INCLUDED_
#define _C_CC_LIBZ_H_INCLUDED_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

_CC_API(int) CompressZipFile(const char *source, const char *dest, uint64_t *resultSize);
_CC_API(int) DecompressZipFile(const char *source, const char *dest, uint64_t *resultSize);
_CC_API(int) MiniUnzip(const char *zipFile, const char *savePath, const char *pass);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif
