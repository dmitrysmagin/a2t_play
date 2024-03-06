#ifndef _UNLZSS_H_
#define _UNLZSS_H_

#ifdef __cplusplus
extern "C" {
#endif

int LZSS_decompress(char *source, char *dest, int size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _UNLZSS_H_ */
