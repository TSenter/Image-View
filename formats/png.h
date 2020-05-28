#ifndef __IV_FORMAT_PNG
#define __IV_FORMAT_PNG
#include <stdio.h>
#include "../utils/types.h"

#define PNG_SIG 0x0A1A0A0D474E5089
#define PNG_SIG_SZ 8
#define PNG_CHNK_LEN 4
#define PNG_CHNK_TYPE_SZ 4
#define PNG_CHNK_IHDR 0x52444849
#define PNG_RATIO 100000.0F

#define PNG_CHUNK_HEADER          "IHDR"
#define PNG_CHUNK_PALETTE         "PLTE"
#define PNG_CHUNK_DATA            "IDAT"
#define PNG_CHUNK_END             "IEND"
#define PNG_CHUNK_TRANSPARENCY    "tRNS"
#define PNG_CHUNK_CHROMACITY      "cHRM"
#define PNG_CHUNK_GAMMA           "gAMA"
#define PNG_CHUNK_ICCP            "iCCP"
#define PNG_CHUNK_SIGBITS         "sBIT"
#define PNG_CHUNK_SRGB            "sRGB"
#define PNG_CHUNK_TEXT_INT        "iTXt"
#define PNG_CHUNK_TEXT            "tEXt"
#define PNG_CHUNK_TEXT_COMPRESSED "zTXt"
#define PNG_CHUNK_PHYS            "pHYs"
#define PNG_CHUNK_TIME            "tIME"

#define PNG_ISCRITICAL(chunk)  (chunk->type[0] & 0x20 != 0)
#define PNG_ISANCILLARY(chunk) (chunk->type[0] & 0x20 == 0)

typedef struct png_chunk {
    unsigned int length;
    char type[4];
    void *data;
    unsigned int CRC;

    struct format_png_t *png;
} png_chunk_t;

typedef struct format_png_t {
    char *name;

    png_chunk_t *IHDR;

    FILE *fin;
    unsigned long long bytes;
} *Format_PNG;

/* Function signatures */

Format_PNG png_new();

int  png_open(Format_PNG, char *);
void png_close(Format_PNG);

/* PNG chunks */
png_chunk_t *png_chunk_next(Format_PNG);
void png_chunk_free(png_chunk_t *);

char **png_PLTE_get(png_chunk_t *);
int png_PLTE_length(png_chunk_t *);
void png_PLTE_free(char **, int);

void *png_IDAT_get(png_chunk_t *);
int png_IDAT_length(png_chunk_t *);

int png_IEND_valid(png_chunk_t *);

short png_tRNS_gray(png_chunk_t *);
short *png_tRNS_true(png_chunk_t *);
char *png_tRNS_index(png_chunk_t *);
int png_tRNS_index_length(png_chunk_t *);

float png_cHRM_whiteX(png_chunk_t *);
float png_cHRM_whiteY(png_chunk_t *);
float png_cHRM_redX(png_chunk_t *);
float png_cHRM_redY(png_chunk_t *);
float png_cHRM_greenX(png_chunk_t *);
float png_cHRM_greenY(png_chunk_t *);
float png_cHRM_blueX(png_chunk_t *);
float png_cHRM_blueY(png_chunk_t *);

float png_gAMA(png_chunk_t *);
int png_gAMA_raw(png_chunk_t *);

char *png_iCCP_name(png_chunk_t *);
char png_iCCP_method(png_chunk_t *);
void *png_iCCP_profile(png_chunk_t *);
int png_iCCP_profile_len(png_chunk_t *);

char *png_sBIT_get(png_chunk_t *);
int png_sBIT_len(png_chunk_t *);

char png_sRGB_get(png_chunk_t *);

char *png_iTXt_keyword(png_chunk_t *);
char *png_iTXt_lang(png_chunk_t *);
char *png_iTXt_text(png_chunk_t *);

char *png_tEXt_keyword(png_chunk_t *);
char *png_tEXt_text(png_chunk_t *);

char *png_zTXt_keyword(png_chunk_t *);
char png_zTXt_method(png_chunk_t *);
void *png_zTXt_data(png_chunk_t *);
int png_zTXt_length(png_chunk_t *);

int png_pHYs_ppuX(png_chunk_t *);
int png_pHYs_ppuY(png_chunk_t *);
char png_pHYs_unit(png_chunk_t *);

short png_tIME_year(png_chunk_t *);
char png_tIME_month(png_chunk_t *);
char png_tIME_day(png_chunk_t *);
char png_tIME_hour(png_chunk_t *);
char png_tIME_minute(png_chunk_t *);
char png_tIME_second(png_chunk_t *);
char *png_tIME_iso8601(png_chunk_t *);

/* Image attributes */
int png_attr_width(Format_PNG);
int png_attr_height(Format_PNG);
char png_attr_bit_depth(Format_PNG);
char png_attr_col_type(Format_PNG);
char png_attr_comp_method(Format_PNG);
char png_attr_filter_method(Format_PNG);
char png_attr_il_method(Format_PNG);

#endif