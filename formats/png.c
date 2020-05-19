#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "png.h"
#include "../utils/version.h"
#include "../utils/iv_opts.h"

/*
 * Initialize an empty PNG image
 * 
 * Returns: NULL on error
 *          Initialized Formant_PNG type
 */
Format_PNG png_new() {
    Format_PNG png = (Format_PNG) malloc(sizeof(struct format_png_t));

    // TODO Initialize PNG image

    return png;
}

/* 
 * Attempt to open a file as a PNG file
 * 
 * Returns:   0 - file opened successfully
 *          < 0 - an unknown error occurred
 *          > 0 - file is not of type PNG
 */
int png_open(Format_PNG png, char *filename) {
    char buf[PNG_SIG_SZ];
    int rv;
    if (filename == NULL) {
        png->name = NULL;
        png->fin = stdin;
    } else {
        char *s = strrchr(filename, '/');
        if (s == NULL) {
            png->name = strdup(filename);
        } else {
            png->name = strdup(s+1);
        }
        png->fin = fopen(filename, "rb");
    }
    if (png->fin == NULL) return -1;

    /* Attempt to read the signature */
    rv = fread(buf, sizeof(char), PNG_SIG_SZ, png->fin);
    if (iv_opts.verbose) {
        printf("Reading signature... ");
    } else if (iv_opts.scan) {
        printf("[PNG] Read signature\n");
    }

    if (rv != PNG_SIG_SZ) return 1;

    /* Validate the signature */
    if ((*(long long*) (&buf)) - PNG_SIG != 0) return 1;
    if (iv_opts.verbose) {
        printf("matched.\n");
    }

    png->IHDR = png_chunk_next(png);

    if (png->IHDR == NULL) {
        return -1;
    }

    return 0;
}

/*
 * Close the png file and free the resources associated with it
 * 
 * Returns: void
 */
void png_close(Format_PNG png) {
    if (png->name) free(png->name);
    if (png->fin)  fclose(png->fin);
    if (png->IHDR) png_chunk_free(png->IHDR);
    free(png);
}

/*
 * Extract the next chunk from the PNG image
 * 
 * Returns: -1 on error
 *          NULL when a valid chunk cannot be read (EOF)
 *          Valid chunk from PNG image
 */
png_chunk_t *png_chunk_next(Format_PNG png) {
    if (feof(png->fin) || ferror(png->fin)) return NULL;
    int rv;

    png_chunk_t *chunk = (png_chunk_t *) malloc(sizeof(png_chunk_t));

    /* Read chunk length */
    rv = fread(&chunk->length, PNG_CHNK_LEN, 1, png->fin);
    if (feof(png->fin)) {
        free(chunk);
        return NULL;
    }
    if (rv != 1) {
        if (iv_opts.verbose) {
            printf("Chunk: Error (chunk length)\n");
        }
        free(chunk);
        return NULL;
    }

    if (iv_opts.verbose) {
        printf("Chunk: ");
        fflush(stdout);
    }

    /* Normalize length */
    chunk->length = ntohl(chunk->length);

    /* Read chunk type */
    rv = fread(chunk->type, 1, PNG_CHNK_TYPE_SZ, png->fin);
    if (rv != PNG_CHNK_TYPE_SZ) {
        if (iv_opts.verbose) {
            printf("Error (chunk type)\n");
        }
        fprintf(stderr, "[%s/png]: Unexpected EOF in reading chunk type, read %d/%d chunk type bytes\n", IV_PROGRAM_NAME, rv, PNG_CHNK_TYPE_SZ);
        free(chunk);
        return NULL;
    }

    /* Allocate memory for chunk data */
    chunk->data = malloc(chunk->length);

    /* Read chunk data */
    if (chunk->length != 0) {
        rv = fread(chunk->data, 1, chunk->length, png->fin);
        if (rv != chunk->length) {
            if (iv_opts.verbose) {
                printf("Error (chunk data)\n");
            }
            fprintf(stderr, "[%s/png]: Unexpected EOF in reading chunk '%.4s', read %d/%d chunk data bytes\n", IV_PROGRAM_NAME, chunk->type, rv, chunk->length);
            free(chunk->data);
            free(chunk);
            return NULL;
        }
    } else {
        free(chunk->data);
        chunk->data = NULL;
    }

    /* Read CRC */
    rv = fread(&chunk->CRC, sizeof(int), 1, png->fin);
    if (rv != 1) {
        if (iv_opts.verbose) {
            printf("Error (chunk CRC)\n");
        }
        fprintf(stderr, "[%s/png]: Unexpected EOF in reading CRC for chunk '%.4s', read %d/%d bytes\n", IV_PROGRAM_NAME, chunk->type, rv, 4);
        free(chunk->data);
        free(chunk);
        return NULL;
    }

    if (iv_opts.verbose) {
        printf("%.4s (%d byte%s)\n", chunk->type, chunk->length, chunk->length != 1 ? "s" : "");
    }

    return chunk;
}

/*
 * Free the memory allocated to a png chunk
 * 
 * Returns: void
 */
void png_chunk_free(png_chunk_t *chunk) {
    if (chunk->data) free(chunk->data);
    
    free(chunk);
}

/*
 * Extract the pixels per unit along the X axis
 * 
 * Returns: -1 on error
 *          pixels per unit on success
 */
int png_pHYs_ppuX(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_PHYS, PNG_CHNK_LEN) != 0) return -1;

    if (chunk->length < 4) return -1;

    return ntohl(*((int *)chunk->data));
}

/*
 * Extract the pixels per unit along the Y axis
 * 
 * Returns: -1 on error
 *          pixels per unit on success
 */
int png_pHYs_ppuY(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_PHYS, PNG_CHNK_LEN) != 0) return -1;

    if (chunk->length < 8) return -1;

    return ntohl(*((int *)(chunk->data + 4)));
}

/*
 * Extract the unit for pixels per unit
 * 
 * Returns: -1 on error
 *           0 for unknown unit
 *           1 for meter
 */
char png_pHYs_unit(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_PHYS, PNG_CHNK_LEN) != 0) return -1;

    if (chunk->length < 9) return -1;

    return *((char *) chunk->data + 9);
}

/*
 * Extract the keyword from a tEXt chunk
 * 
 * Returns: NULL on error
 *          keyword on success; must be free'd
 */
char *png_tEXt_keyword(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT, PNG_CHNK_LEN) != 0) return NULL;

    return strdup(chunk->data);
}

/*
 * Extract the text from a tEXt chunk
 * 
 * Returns: NULL on error
 *          keyword on success; must be free'd
 */
char *png_tEXt_text(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT, PNG_CHNK_LEN) != 0) return NULL;

    int c = strlen(chunk->data) + 1;
    char *text = (char *) malloc(chunk->length - c + 1);
    
    strncpy(text, chunk->data + c, chunk->length - c);
    text[chunk->length - c] = 0;

    return text;
}

/*
 * Extract the keyword from a zTXt chunk
 * 
 * Returns: NULL on error
 *          keyword on success; must be free'd
 */
char *png_zTXt_keyword(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT, PNG_CHNK_LEN) != 0) return NULL;

    if (chunk->data == NULL || chunk->length == 0) return NULL;

    return strdup(chunk->data);
}

/*
 * Extract the compression method from a zTXt chunk
 * 
 * Returns: -1 on error
 *          compression method (>= 0)
 */
char png_zTXt_method(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT_COMPRESSED, PNG_CHNK_LEN) != 0) return NULL;

    if (chunk->data == NULL || chunk->length == 0) return NULL;

    return *((char *) chunk->data + strlen(chunk->data) + 1);
}

/*
 * Extract the compressed text datastream
 * 
 * Returns: NULL on error
 *          Pointer to block containing datastream; must be free'd
 */
void *png_zTXt_data(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT_COMPRESSED, PNG_CHNK_LEN) != 0) return NULL;

    if (chunk->data == NULL || chunk->length == 0) return NULL;

    int len = strlen(chunk->data) + 2;
    int n = chunk->length - len;
    void *data = malloc(n);

    return memcpy(data, chunk->data + len, n);
}

/*
 * Extract the length of the compressed text datastream
 * 
 * Returns: -1 on error
 *          Length of datastream
 */
int png_zTXt_length(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT_COMPRESSED, PNG_CHNK_LEN) != 0) return NULL;

    if (chunk->data == NULL || chunk->length == 0) return NULL;

    return chunk->length - (strlen(chunk->data) + 2);
}

/*
 * Extract the keyword from an iTXt chunk
 * 
 * Returns: NULL on error
 *          keyword on success
 */
char *png_iTXt_keyword(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT_INT, PNG_CHNK_LEN) != 0) return NULL;

    return strdup((char *) chunk->data);
}

/*
 * Extract the language tag from an iTXt chunk
 * 
 * Returns: NULL on error
 *          Empty string if no language tag available; must be free'd
 *          string containing ISO 646 hyphen-separated words; must be free'd
 */
char *png_iTXt_lang(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT_INT, PNG_CHNK_LEN) != 0) return NULL;

    if (chunk->length <= strlen(chunk->data) + 3) return NULL;

    return strdup(chunk->data + strlen(chunk->data) + 3);
}

/*
 * Extract the text value from an iTXt chunk
 * 
 * Returns: NULL on error
 *          string containing text from an iTXt chunk on success; must be free'd
 */
char *png_iTXt_text(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT_INT, PNG_CHNK_LEN) != 0) return NULL;

    int c = strlen(chunk->data) + 3; /* Keyword + comp. flag + comp. method */
    char *p = chunk->data + c;
    char *buf;

    c += strlen(p) + 1; /* Language tag */
    p = chunk->data + c;

    c += strlen(p) + 1; /* Translated keyword */
    p = chunk->data + c;
    
    c = chunk->length - c;

    buf = (char *) malloc(sizeof(char) * (c + 1));

    strncpy(buf, p, c);
    buf[c] = 0;

    return buf;
}

/*
 * Extract the year from the last modified date
 * 
 * Returns: -1 on error
 *          Complete year (eg. 1995, not 95) on success
 */
short png_tIME_year(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TIME, PNG_CHNK_LEN) != 0) return -1;

    return ntohs(*((short *) (chunk->data)));
}

/*
 * Extract the month from the last modified date
 * 
 * Returns: -1 on error
 *          Month (1-12) on success
 */
char png_tIME_month(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TIME, PNG_CHNK_LEN) != 0) return -1;

    return *((char *) (chunk->data + 2));
}

/*
 * Extract the day from the last modified date
 * 
 * Returns: -1 on error
 *          Day (1-31) on success
 */
char png_tIME_day(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TIME, PNG_CHNK_LEN) != 0) return -1;

    return *((char *) (chunk->data + 3));
}

/*
 * Extract the hour from the last modified date
 * 
 * Returns: -1 on error
 *          Hour (0-23) on success
 */
char png_tIME_hour(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TIME, PNG_CHNK_LEN) != 0) return -1;

    return *((char *) (chunk->data + 4));
}

/*
 * Extract the minute from the last modified date
 * 
 * Returns: -1 on error
 *          Minute (0-59) on success
 */
char png_tIME_minute(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TIME, PNG_CHNK_LEN) != 0) return -1;

    return *((char *) (chunk->data + 5));
}

/*
 * Extract the second from the last modified date
 * 
 * Returns: -1 on error
 *          Second (0-60 to allow for leap seconds) on success
 */
char png_tIME_second(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TIME, PNG_CHNK_LEN) != 0) return -1;

    return *((char *) (chunk->data + 6));
}

/*
 * Extract the last modified date in ISO 8601 format
 * 
 * Returns: NULL on error
 *          Last modified date in ISO 8601 format: YYYY-MM-DDTHH:MM:SS+00:00
 */
char *png_tIME_iso8601(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TIME, PNG_CHNK_LEN) != 0) return NULL;

    char *time = (char *) malloc(sizeof(char) * 26); // Prefer not to hardcode the length in

    sprintf(time, "%04hd-%02hhd-%02hhdT%02hhd:%02hhd:%02hhd+00:00",
            ntohs(*((short *) (chunk->data))),
            *((char *) (chunk->data + 2)),
            *((char *) (chunk->data + 3)),
            *((char *) (chunk->data + 4)),
            *((char *) (chunk->data + 5)),
            *((char *) (chunk->data + 6)));
}

/*
 * Fetch the width of the given image
 * 
 * Returns: -1 on error
 *          width of image (in pixels) on success
 */
int png_attr_width(Format_PNG png) {
    return ntohl(*((int *) png->IHDR->data));
}

/*
 * Fetch the height of the given image
 * 
 * Returns: -1 on error
 *          height of image (in pixels) on success
 */
int png_attr_height(Format_PNG png) {
    return ntohl(*((int *) (png->IHDR->data + 4)));
}

/*
 * Fetch the bit depth of the given image
 * 
 * Returns: -1 on error
 *          bit depth on success
 */
char png_attr_bit_depth(Format_PNG png) {
    if (png == NULL) return -1;

    if (png->IHDR == NULL || png->IHDR->data == NULL) return -1;

    if (png->IHDR->length < 9) return -1;

    return *((char *) png->IHDR->data + 8);
}

/*
 * Fetch the color type of the given image
 * 
 * Returns: -1 on error
 *          color type on success
 */
char png_attr_col_type(Format_PNG png) {
    if (png == NULL) return -1;

    if (png->IHDR == NULL || png->IHDR->data == NULL) return -1;

    if (png->IHDR->length < 10) return -1;

    return *((char *) png->IHDR->data + 9);
}

/*
 * Fetch the compression method of the given image
 * 
 * Returns: -1 on error
 *          compression method on success
 */
char png_attr_comp_method(Format_PNG png) {
    if (png == NULL) return -1;

    if (png->IHDR == NULL || png->IHDR->data == NULL) return -1;

    if (png->IHDR->length < 11) return -1;
    
    return *((char *) png->IHDR->data + 10);
}

/*
 * Fetch the filter method of the given image
 * 
 * Returns: -1 on error
 *          filter method on success
 */
char png_attr_filter_method(Format_PNG png) {
    if (png == NULL) return -1;

    if (png->IHDR == NULL || png->IHDR->data == NULL) return -1;

    if (png->IHDR->length < 12) return -1;
    
    return *((char *) png->IHDR->data + 11);
}

/*
 * Fetch the interlace method of the given image
 * 
 * Returns: -1 on error
 *          0 on success (no interlace)
 *          1 on success (Adam7 interlace)
 */
char png_attr_il_method(Format_PNG png) {
    if (png == NULL) return -1;

    if (png->IHDR == NULL || png->IHDR->data == NULL) return -1;

    if (png->IHDR->length < 13) return -1;
    
    return *((char *) png->IHDR->data + 12);
}