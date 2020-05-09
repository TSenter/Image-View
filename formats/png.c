#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "png.h"
#include "../utils/version.h"

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
    char buf[512];
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

    if (rv != PNG_SIG_SZ) return 1;

    /* Validate the signature */
    if ((*(long long*) (&buf)) - SIG_PNG != 0) return 1;

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
    free(png);
}

png_chunk_t *png_chunk_next(Format_PNG png) {
    if (feof(png->fin) || ferror(png->fin)) return NULL;
    int rv;

    png_chunk_t *chunk = (png_chunk_t *) malloc(sizeof(png_chunk_t));

    /* Read chunk length */
    rv = fread(&chunk->length, PNG_CHNK_LEN, 1, png->fin);
    if (rv != 1) {
        free(chunk);
        return NULL;
    }

    /* Normalize length */
    chunk->length = ntohl(chunk->length);

    /* Read chunk type */
    rv = fread(chunk->type, 1, PNG_CHNK_TYPE_SZ, png->fin);
    if (rv != PNG_CHNK_TYPE_SZ) {
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
            fprintf(stderr, "[%s/png]: Unexpected EOF in reading chunk '%.4s', read %d/%d chunk data bytes\n", IV_PROGRAM_NAME, chunk->type, rv, chunk->length);
            free(chunk->data);
            free(chunk);
            return NULL;
        }
    } else chunk->data = NULL;

    /* Read CRC */
    rv = fread(&chunk->CRC, sizeof(int), 1, png->fin);
    if (rv != 1) {
        fprintf(stderr, "[img");
        free(chunk->data);
        free(chunk);
        return NULL;
    }

    return chunk;
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