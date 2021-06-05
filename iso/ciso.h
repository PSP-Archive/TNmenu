#ifndef _CISO_H_
#define _CISO_H_

typedef struct ciso_header
{
    unsigned char magic[4];         /* +00 : 'C','I','S','O'                 */
    unsigned long header_size;      /* +04 : header size (==0x18)            */
    unsigned long long total_bytes; /* +08 : number of original data size    */
    unsigned long block_size;       /* +10 : number of compressed block size */
    unsigned char ver;              /* +14 : version 01                      */
    unsigned char align;            /* +15 : align of index value            */
    unsigned char rsv_06[2];        /* +16 : reserved                        */
} CISO_H;

int cso_read(char *buf, const char *path, int pos, int size);

#endif