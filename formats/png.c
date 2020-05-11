#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "png.h"
#include "../utils/version.h"
#include "../utils/iv_opts.h"

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
    if ((*(long long*) (&buf)) - SIG_PNG != 0) return 1;
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
 */
void png_close(Format_PNG png) {
    if (png->name) free(png->name);
    if (png->fin)  fclose(png->fin);
    if (png->IHDR) png_chunk_free(png->IHDR);
    free(png);
}

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

void png_chunk_free(png_chunk_t *chunk) {
    if (chunk->data) free(chunk->data);
    
    free(chunk);
}

/*
 * Extract the keyword from an iTXt chunk
 */
char *png_iTXt_keyword(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT_INT, PNG_CHNK_LEN) != 0) return NULL;

    return strdup((char *) chunk->data);
}

/*
 * Extract the keyword from a tEXt chunk
 * 
 * Returns: NULL - on error
 *          keyword on success; must be free'd
 */
char *png_tEXt_keyword(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT, PNG_CHNK_LEN) != 0) return NULL;

    return strdup(chunk->data);
}

/*
 * Extract the text from a tEXt chunk
 * 
 * Returns: NULL - on error
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
 * Extract the language tag from an iTXt chunk
 * 
 * Returns: NULL - on error
 *          ""   - no language tag available; must be free'd
 *          string containing ISO 646 hyphen-separated words; must be free'd
 */
char *png_iTXt_lang(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TEXT_INT, PNG_CHNK_LEN) != 0) return NULL;

    if (chunk->length <= strlen(chunk->data) + 3) return NULL;

    return strdup(chunk->data + strlen(chunk->data) + 3);
}

/*
 * Extract the text value from an iTXt chunk
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
 *          Minute (0-60 to allow for leap seconds) on success
 */
char png_tIME_second(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_TIME, PNG_CHNK_LEN) != 0) return -1;

    return *((char *) (chunk->data + 6));
}

/*
 * Fetch the width of the given image
 */
int png_attr_w(Format_PNG png) {
    return ntohl(*((int *) png->IHDR->data));
}

/*
 * Fetch the height of the given image
 */
int png_attr_h(Format_PNG png) {
    return ntohl(*((int *) (png->IHDR->data + 4)));
}

// TODO Remove when no longer needed
void png_debug(Format_PNG png) {
    printf("%-25s | %u x %u pixels\n", png->name, png_attr_w(png), png_attr_h(png));
}

/* 
 * Extract all metadata from a PNG image
 * 
 * Returns: > 0 - total number of metadata items extracted
 *            0 - metadata already extracted
 *          < 0 - an error occurred
 */
int png_extract_meta_all(Format_PNG png) {

}

int png_extract_meta(Format_PNG png, char *key, IVValue val) {

}