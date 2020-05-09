#ifndef __IV_FORMAT_PNG
#define __IV_FORMAT_PNG
#include <stdio.h>
#include "../utils/types.h"

#define SIG_PNG 0x0A1A0A0D474E5089
#define PNG_SIG_SZ 8
#define PNG_CHNK_LEN 4
#define PNG_CHNK_TYPE_SZ 4
#define PNG_CHNK_IHDR 0x52444849

#define PNG_ISCRITICAL(chunk)  (chunk->type[0] & 0x20 != 0)
#define PNG_ISANCILLARY(chunk) (chunk->type[0] & 0x20 == 0)

typedef struct png_chunk {
    unsigned int length;
    char type[4];
    void *data;
    unsigned int CRC;

    struct png_chunk *next;
} png_chunk_t;

typedef struct format_png_t {
    char *name;

    png_chunk_t *IHDR;

    FILE *fin;
} *Format_PNG;

/* Function signatures */

Format_PNG png_new();

int  png_open(Format_PNG, char *);
void png_close(Format_PNG);

/* PNG chunks */
png_chunk_t *png_chunk_next(Format_PNG);

/* Image attributes */
int png_attr_w(Format_PNG);
int png_attr_h(Format_PNG);

void png_debug(Format_PNG);

int png_extract_meta(Format_PNG, char *, IVValue);

#endif