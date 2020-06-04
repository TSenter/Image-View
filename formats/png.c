#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "png.h"
#include "../utils/version.h"

/*
 * Initialize an empty PNG image
 * 
 * Returns: NULL on error
 *          Initialized Formant_PNG type
 */
Format_PNG png_new() {
    Format_PNG png = (Format_PNG) malloc(sizeof(struct format_png_t));

    if (png == NULL) return NULL;

    // TODO Initialize PNG image
    png->IHDR = NULL;
    png->fin = NULL;
    png->name = NULL;
    png->bytes = 0;

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

    if (rv != PNG_SIG_SZ) return 1;
    png->bytes += sizeof(char) * rv;

    /* Validate the signature */
    if ((*(long long*) (&buf)) - PNG_SIG != 0) return 1;

    png->IHDR = png_chunk_next(png);

    if (png->IHDR == (png_chunk_t *) -1) {
        free(png->name);
        fclose(png->fin);
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
    int read = 0;
    int rv;
    if (feof(png->fin) || ferror(png->fin)) return (png_chunk_t *) -1;

    png_chunk_t *chunk = (png_chunk_t *) malloc(sizeof(png_chunk_t));

    /* Read chunk length */
    rv = fread(&chunk->length, PNG_CHNK_LEN, 1, png->fin);
    if (feof(png->fin)) {
        free(chunk);
        return NULL;
    }
    if (rv != 1) {
        free(chunk);

        return (png_chunk_t *) -1;
    }
    read += sizeof(char) * rv;

    /* Normalize length */
    chunk->length = ntohl(chunk->length);

    /* Read chunk type */
    rv = fread(chunk->type, sizeof(char), PNG_CHNK_TYPE_SZ, png->fin);
    if (rv != PNG_CHNK_TYPE_SZ) {
        fseek(png->fin, -1 * read, SEEK_CUR);
        fprintf(stderr, "[%s/png]: Unexpected EOF in reading chunk type, read %d/%d chunk type bytes\n", IV_PROGRAM_NAME, rv, PNG_CHNK_TYPE_SZ);
        free(chunk);
        return NULL;
    }
    read += sizeof(char) * rv;

    /* Allocate memory for chunk data */
    chunk->data = malloc(chunk->length);

    /* Read chunk data */
    if (chunk->length != 0) {
        rv = fread(chunk->data, 1, chunk->length, png->fin);
        if (rv != chunk->length) {
            fseek(png->fin, -1 * read, SEEK_CUR);
            fprintf(stderr, "[%s/png]: Unexpected EOF in reading chunk '%.4s', read %d/%d chunk data bytes\n", IV_PROGRAM_NAME, chunk->type, rv, chunk->length);
            free(chunk->data);
            free(chunk);
            return NULL;
        }
        read += rv;
    } else {
        free(chunk->data);
        chunk->data = NULL;
    }

    /* Read CRC */
    rv = fread(&chunk->CRC, sizeof(int), 1, png->fin);
    if (rv != 1) {
        fseek(png->fin, -1 * read, SEEK_CUR);
        fprintf(stderr, "[%s/png]: Unexpected EOF in reading CRC for chunk '%.4s', read %d/%d bytes\n", IV_PROGRAM_NAME, chunk->type, rv, 4);
        free(chunk->data);
        free(chunk);
        return NULL;
    }
    read += sizeof(int) * rv;

    chunk->png = png;

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
 * Fetch the palette as a two-dimensional array:
 *      [n][0] - Red channel
 *      [n][1] - Green channel
 *      [n][2] - Blue channel
 * 
 * Returns: NULL on error
 *          (char **) -1 if chunk length is not divisible by 3 (required by Internation Standard section 11.2.3)
 */
char **png_PLTE_get(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_PALETTE, NULL);
    
    int n = chunk->length % 3, i;

    if (n != 0) return (char **) -1;
    
    n = chunk->length / 3;
    char **palette = (char **) malloc(sizeof(char *) * n);

    for (i = 0; i < n; i++) {
        palette[i] = malloc(sizeof(char) * 3);
        palette[i][0] = ((char *) chunk->data)[3*i];
        palette[i][1] = ((char *) chunk->data)[3*i+1];
        palette[i][2] = ((char *) chunk->data)[3*i+2];
    }

    return palette;
}

/*
 * Fetch the number of entries in a palette chunk
 * 
 * Returns: -1 on error
 *           0 if chunk does not conform to International Standard (section 11.2.3)
 * 
 */
int png_PLTE_length(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_PALETTE, -1);

    if (chunk->length % 3 != 0) return 0;

    return chunk->length / 3;
}

/*
 * Free the memory allocated for a palette
 * 
 * Returns: void
 */
void png_PLTE_free(char **palette, int len) {
    if (palette == NULL || palette[0] == NULL) return;

    if (len <= 0) return;

    while (len--) {
        free(palette[len]);
    }
    free(palette);
}

/*
 * Get the included datastream for the IDAT chunk
 * 
 * Returns: NULL on error
 */
void *png_IDAT_get(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_DATA, NULL);

    void *data = malloc(chunk->length);

    memcpy(data, chunk->data, chunk->length);

    return data;
}

/*
 * Get the length of the datastream for this IDAT chunk
 * 
 * Returns: -1 on error
 */
int png_IDAT_length(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_DATA, -1);

    return chunk->length;
}

/*
 * Validate an IEND chunk. An IEND chunk is considered valid if it has no length and no data
 * 
 * Returns: 1 if chunk is invalid
 *          0 if chunk is valid
 */
int png_IEND_valid(png_chunk_t *chunk) {
    if (strncmp(chunk->type, PNG_CHUNK_END, PNG_CHNK_LEN) != 0) return 1;

    if (chunk->length != 0 || chunk->data != NULL) return 1;

    return 0;
}

/*
 * Fetch transparency from tRNS chunk (color type 0 - grayscale)
 * 
 * Returns: -1 on error (image could be of the wrong color type)
 */
short png_tRNS_gray(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TRANSPARENCY, -1);

    if (png_attr_col_type(chunk->png) != 0) return -1;

    return ntohs(*((short *)chunk->data));
}

/*
 * Fetch transparency samples from tRNS chunk (color type 2 - truecolor)
 * 
 * Returns: NULL on error
 *          Array of 3 sample values (R, G, B); must be free'd
 */
short *png_tRNS_true(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TRANSPARENCY, NULL);

    if (png_attr_col_type(chunk->png) != 2) return NULL;

    short *array = (short *) malloc(sizeof(short) * 3);
    int i;

    for (i = 0; i < 3; i++) {
        array[i] = ntohs(*(((short *) chunk->data) + i));
    }

    return array;
}

/*
 * Fetch transparency palette from tRNS chunk (color type 3 - indexed-color)
 * 
 * Returns: NULL on error
 *          Array of alpha channel for palette indexes; must be free'd
 */
char *png_tRNS_index(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TRANSPARENCY, NULL);

    if (png_attr_col_type(chunk->png) != 3) return NULL;

    char *palette = (char *) malloc(sizeof(char) * chunk->length);
    int i;

    for (i = 0; i < chunk->length; i++) {
        palette[i] = ((char *)(chunk->data))[i];
    }

    return palette;
}

/*
 * Fetch the length of the transparency palette from tRNS chunk (color type 3 - indexed-color)
 * 
 * Returns: -1 on error
 *          Length of array returned by `png_tRNS_index()`
 */
int png_tRNS_index_length(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TRANSPARENCY, -1);

    if (png_attr_col_type(chunk->png) != 3) return -1;

    return chunk->length;
}

/*
 * Fetch the white point X from the cHRM chunk
 * 
 * Returns: -1 on error
 */
float png_cHRM_whiteX(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_CHROMACITY, -1);

    int whiteX = *((int *) chunk->data);
    whiteX = ntohl(whiteX);

    return whiteX / PNG_RATIO;
}

/*
 * Fetch the white point Y from the cHRM chunk
 * 
 * Returns: -1 on error
 */
float png_cHRM_whiteY(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_CHROMACITY, -1);

    int whiteY = *(((int *)chunk->data)+1);
    whiteY = ntohl(whiteY);

    return whiteY / PNG_RATIO;
}

/*
 * Fetch the red point X from the cHRM chunk
 * 
 * Returns: -1 on error
 */
float png_cHRM_redX(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_CHROMACITY, -1);

    int redX = *(((int *)chunk->data)+2);
    redX = ntohl(redX);

    return redX / PNG_RATIO;
}

/*
 * Fetch the red point Y from the cHRM chunk
 * 
 * Returns: -1 on error
 */
float png_cHRM_redY(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_CHROMACITY, -1);

    int redY = *(((int *)chunk->data)+3);
    redY = ntohl(redY);

    return redY / PNG_RATIO;
}

/*
 * Fetch the green point X from the cHRM chunk
 * 
 * Returns: -1 on error
 */
float png_cHRM_greenX(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_CHROMACITY, -1);

    int greenX = *(((int *)chunk->data)+4);
    greenX = ntohl(greenX);

    return greenX / PNG_RATIO;
}

/*
 * Fetch the green point Y from the cHRM chunk
 * 
 * Returns: -1 on error
 */
float png_cHRM_greenY(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_CHROMACITY, -1);

    int greenY = *(((int *)chunk->data)+5);
    greenY = ntohl(greenY);

    return greenY / PNG_RATIO;
}

/*
 * Fetch the blue point X from the cHRM chunk
 * 
 * Returns: -1 on error
 */
float png_cHRM_blueX(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_CHROMACITY, -1);

    int blueX = *(((int *)chunk->data)+6);
    blueX = ntohl(blueX);

    return blueX / PNG_RATIO;
}

/*
 * Fetch the blue point Y from the cHRM chunk
 * 
 * Returns: -1 on error
 */
float png_cHRM_blueY(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_CHROMACITY, -1);

    int blueY = *(((int *)chunk->data)+7);
    blueY = ntohl(blueY);

    return blueY / PNG_RATIO;
}

/*
 * Calculate the image gamma from the gAMA chunk
 * 
 * Returns: -1 on error
 */
float png_gAMA(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_GAMMA, -1);

    int gamma = *((int *)chunk->data);
    gamma = ntohl(gamma);

    return gamma / PNG_RATIO;
}

/*
 * Fetch the raw image gamma from the gAMA chunk (gamma times 100000)
 * 
 * Returns: -1 on error
 */
int png_gAMA_raw(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_GAMMA, -1);

    int gamma = *((int *)chunk->data);
    gamma = ntohl(gamma);

    return gamma;
}

/*
 * Fetch the name of the embedded ICC (International Color Consortium) profile
 * 
 * Returns: NULL on error
 *          Profile name on success; must be free'd
 */
char *png_iCCP_name(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_ICCP, NULL);

    return strdup((char *)chunk->data);
}

/*
 * Fetch the compression method of the embedded ICC (International Color Consortium) profile
 * 
 * Returns: -1 on error
 */
char png_iCCP_method(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_ICCP, -1);

    int offset = strlen(chunk->data) + 1;

    return *((char *)chunk->data+offset);
}

/*
 * Fetch the compressed embedded ICC (International Color Consortium) profile
 * 
 * Returns: NULL on error
 *          Datastream containing profile; must be free'd
 */
void *png_iCCP_profile(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_ICCP, NULL);

    int offset = strlen(chunk->data) + 2;
    int size = chunk->length - offset;
    void *profile = malloc(size);

    memcpy(profile, chunk->data + offset, size);

    return profile;
}

/*
 * Fetch the length of the compressed embedded ICC (International Color Consortium) profile
 * 
 * Returns: -1 on error
 */
int png_iCCP_profile_len(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_ICCP, -1);

    return chunk->length - (strlen(chunk->data) + 2);
}

/*
 * Fetch the significant bits from the sBIT chunk
 * 
 * Returns: NULL on error
 */
char *png_sBIT_get(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_SIGBITS, NULL);

    char *bits;
    int len;

    switch (png_attr_col_type(chunk->png)) {
        case 0:
            len = 1;
            break;
        case 2:
        case 3:
            len = 3;
            break;
        case 4:
            len = 2;
            break;
        case 6:
            len = 4;
            break;
        default:
            len = -1;
    }

    if (len == -1) return NULL;

    bits = (char *) malloc(len);

    memcpy(bits, chunk->data, len);

    return bits;
}

/*
 * Fetch the length of the significant bits array from the sBIT chunk
 * 
 * Returns: -1 on error
 */
int png_sBIT_len(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_SIGBITS, -1);

    switch (png_attr_col_type(chunk->png)) {
        case 0:
            return 1;
        case 2:
        case 3:
            return 3;
        case 4:
            return 2;
        case 6:
            return 4;
        default:
            return -1;
    }
}

/*
 * Fetch the rendering intent from the sRGB chunk
 * 
 * Returns: -1 on error
 */
char png_sRGB_get(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_SRGB, -1);

    return *((char *)chunk->data);
}

/*
 * Extract the keyword from an iTXt chunk
 * 
 * Returns: NULL on error
 *          keyword on success
 */
char *png_iTXt_keyword(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TEXT_INT, NULL);

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
    PNG_VALIDATE(PNG_CHUNK_TEXT_INT, NULL);

    return strdup(chunk->data + strlen(chunk->data) + 3);
}

/*
 * Extract the text value from an iTXt chunk
 * 
 * Returns: NULL on error
 *          string containing text from an iTXt chunk on success; must be free'd
 */
char *png_iTXt_text(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TEXT_INT, NULL);

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
 * Extract the keyword from a tEXt chunk
 * 
 * Returns: NULL on error
 *          keyword on success; must be free'd
 */
char *png_tEXt_keyword(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TEXT, NULL);

    return strdup(chunk->data);
}

/*
 * Extract the text from a tEXt chunk
 * 
 * Returns: NULL on error
 *          keyword on success; must be free'd
 */
char *png_tEXt_text(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TEXT, NULL);

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
    PNG_VALIDATE(PNG_CHUNK_TEXT, NULL);

    return strdup(chunk->data);
}

/*
 * Extract the compression method from a zTXt chunk
 * 
 * Returns: -1 on error
 *          compression method (>= 0)
 */
char png_zTXt_method(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TEXT_COMPRESSED, -1);

    return *((char *) chunk->data + strlen(chunk->data) + 1);
}

/*
 * Extract the compressed text datastream
 * 
 * Returns: NULL on error
 *          Pointer to block containing datastream; must be free'd
 */
void *png_zTXt_data(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TEXT_COMPRESSED, NULL);

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
    PNG_VALIDATE(PNG_CHUNK_TEXT_COMPRESSED, -1);

    return chunk->length - (strlen(chunk->data) + 2);
}

/*
 * Fetch the background color information of the image
 * 
 * Returns: NULL on error
 *          Array of values corresponding to appropriate background color channels; must be free'd
 */
void *png_bKGD_get(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_BACKGROUND, NULL);

    int type = png_attr_col_type(chunk->png);
    void *array = NULL;

    if (type == 0 || type == 4) {
        array = malloc(sizeof(short));

        short s = *((short *)chunk->data);
        ((short *)array)[0] = ntohs(s);
    } else if (type == 2 || type == 6) {
        array = malloc(sizeof(short) * 3);

        short *s = (short *)chunk->data;
        ((short *)array)[0] = ntohs(s[0]);
        ((short *)array)[1] = ntohs(s[1]);
        ((short *)array)[2] = ntohs(s[2]);
    } else if (type == 3) {
        array == malloc(sizeof(char));

        ((char *)array)[0] = *((char *)chunk->data);
    }

    return array;
}

/*
 * Fetch the length of the array returned by `png_bKGD_get()`; this is the length of the array in bytes, not elements
 * 
 * Returns: -1 on error
 */
int png_bKGD_len(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_BACKGROUND, -1);

    switch (png_attr_col_type(chunk->png)) {
        case 0:
        case 4:
            return 2;
        case 2:
        case 6:
            return 6;
        case 3:
            return 1;
        default:
            return -1;
    }
}

/*
 * Fetch the histogram of the image
 * 
 * Returns: NULL on error
 *          Array of values representing the frequencies of the colors contained in the PLTE chunk; must be free'd
 */
unsigned short *png_hIST_get(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_HISTOGRAM, NULL);

    switch (png_attr_col_type(chunk->png)) {
        case 2:
        case 3:
        case 6:
            break;
        default:
            return NULL;
    }

    unsigned short *array = (unsigned short *) malloc(chunk->length);

    memcpy(array, chunk->data, chunk->length);

    return array;
}

/*
 * Fetch the length of the histogram of the image
 * 
 * Returns: -1 on error
 */
int png_hIST_len(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_HISTOGRAM, -1);

    switch (png_attr_col_type(chunk->png)) {
        case 2:
        case 3:
        case 6:
            break;
        default:
            return -1;
    }

    return chunk->length / sizeof(unsigned short);
}

/*
 * Extract the pixels per unit along the X axis
 * 
 * Returns: -1 on error
 *          pixels per unit on success
 */
int png_pHYs_ppuX(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_PHYS, -1);

    return ntohl(*((int *)chunk->data));
}

/*
 * Extract the pixels per unit along the Y axis
 * 
 * Returns: -1 on error
 *          pixels per unit on success
 */
int png_pHYs_ppuY(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_PHYS, -1);

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
    PNG_VALIDATE(PNG_CHUNK_PHYS, -1);

    return *((char *) chunk->data + 9);
}

/*
 * Fetch the name of the suggested palette
 * 
 * Returns: NULL on error
 *          Name of the palette; must be free'd
 */
char *png_sPLT_name(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_SPALETTE, NULL);

    return strdup(chunk->data);
}

/*
 * Fetch the sample depth of the suggested palette
 * 
 * Returns: -1 on error
 */
char png_sPLT_sample_depth(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_SPALETTE, -1);

    int offset = strlen(chunk->data) + 1;

    return *((char *)chunk->data + offset);
}

/*
 * Fetch an array of entries in the suggested palette
 * 
 * Returns: NULL on error
 *          Array of entries on success; must be free'd
 */
palette_entry_t *png_sPLT_entries(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_SPALETTE, NULL);

    int offset = strlen(chunk->data) + 2, i;
    int size = chunk->length - offset, entry_size = 10;
    int num_entries;
    void *data = chunk->data + offset;

    if (size % 6 == 0) {
        num_entries = size / 6;
        size = 6;
    } else if (size % 10 == 0) {
        num_entries = size / 10;
        size = 10;
    } else {
        return NULL;
    }

    palette_entry_t *entries = (palette_entry_t *) malloc(sizeof(palette_entry_t) * num_entries);

    if (size == 6) {
        for (i = 0; i < num_entries; i++) {
            entries[i].red       = *((char *)(data + (i * size + 0)));
            entries[i].green     = *((char *)(data + (i * size + 1)));
            entries[i].blue      = *((char *)(data + (i * size + 2)));
            entries[i].alpha     = *((char *)(data + (i * size + 3)));
            entries[i].frequency = ntohs(*((short *)(data + (i * size + 4))));
        }
    } else {
        for (i = 0; i < num_entries; i++) {
            entries[i].red       = ntohs(*((short *)(data + (i * size + 0))));
            entries[i].green     = ntohs(*((short *)(data + (i * size + 2))));
            entries[i].blue      = ntohs(*((short *)(data + (i * size + 4))));
            entries[i].alpha     = ntohs(*((short *)(data + (i * size + 6))));
            entries[i].frequency = ntohs(*((short *)(data + (i * size + 8))));
        }
    }

    return entries;
}

/*
 * Fetch the number of entries in the suggested palette
 * 
 * Returns: -1 on error
 */
int png_sPLT_entries_len(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_SPALETTE, -1);

    int size = chunk->length - (strlen(chunk->data) + 2);

    if (size % 6 == 0) {
        return size / 6;
    } else if (size % 10 == 0) {
        return size / 10;
    } else {
        return -1;
    }
}

/*
 * Fetch the minimum amount of space (in bytes) required to store one entry in the suggested palette
 * 
 * Returns: -1 on error
 *           6 if the rgba channels can be stored in one byte
 *          10 if the rgba channels can be stored in two bytes
 */
int png_sPLT_entry_size(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_SPALETTE, -1);

    int sample_depth  = *((int *) (chunk->data + strlen(chunk->data) + 1));

    if (sample_depth == 8) {
        return 6;
    } else if (sample_depth == 16) {
        return 10;
    } else {
        return -1;
    }
}

/*
 * Extract the year from the last modified date
 * 
 * Returns: -1 on error
 *          Complete year (eg. 1995, not 95) on success
 */
short png_tIME_year(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TIME, -1);

    return ntohs(*((short *) (chunk->data)));
}

/*
 * Extract the month from the last modified date
 * 
 * Returns: -1 on error
 *          Month (1-12) on success
 */
char png_tIME_month(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TIME, -1);

    return *((char *) (chunk->data + 2));
}

/*
 * Extract the day from the last modified date
 * 
 * Returns: -1 on error
 *          Day (1-31) on success
 */
char png_tIME_day(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TIME, -1);

    return *((char *) (chunk->data + 3));
}

/*
 * Extract the hour from the last modified date
 * 
 * Returns: -1 on error
 *          Hour (0-23) on success
 */
char png_tIME_hour(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TIME, -1);

    return *((char *) (chunk->data + 4));
}

/*
 * Extract the minute from the last modified date
 * 
 * Returns: -1 on error
 *          Minute (0-59) on success
 */
char png_tIME_minute(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TIME, -1);

    return *((char *) (chunk->data + 5));
}

/*
 * Extract the second from the last modified date
 * 
 * Returns: -1 on error
 *          Second (0-60 to allow for leap seconds) on success
 */
char png_tIME_second(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TIME, -1);

    return *((char *) (chunk->data + 6));
}

/*
 * Extract the last modified date in ISO 8601 format
 * 
 * Returns: NULL on error
 *          Last modified date in ISO 8601 format: YYYY-MM-DDTHH:MM:SS+00:00
 */
char *png_tIME_iso8601(png_chunk_t *chunk) {
    PNG_VALIDATE(PNG_CHUNK_TIME, NULL);

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
    if (png == NULL) return -1;

    if (png->IHDR == NULL || png->IHDR->data == NULL) return -1;

    if (png->IHDR->length < 4) return -1;

    return ntohl(*((int *) png->IHDR->data));
}

/*
 * Fetch the height of the given image
 * 
 * Returns: -1 on error
 *          height of image (in pixels) on success
 */
int png_attr_height(Format_PNG png) {
    if (png == NULL) return -1;

    if (png->IHDR == NULL || png->IHDR->data == NULL) return -1;
    
    if (png->IHDR->length < 8) return -1;

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