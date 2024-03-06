#ifndef _UNLZW_H_
#define _UNLZW_H_

#ifdef __cplusplus
extern "C" {
#endif

int LZW_decompress(char *source, char *dest, int size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _UNLZW_H_ */
