#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <poll.h>
#include <string.h>

#include "utils/iv_opts.h"
#include "utils/types.h"
#include "utils/version.h"
#include "formats/png.h"

void usage(int exit_code) {
    fprintf(stderr,
        "Usage: %s [options...] [command] <file1> [file2...]\n\n"
        "Options:\n"
        "-h, --help [command]    Get help on %s (or [command])\n"
        "-v, --version           Print version information\n"
        "-s, --scan              Scan the chunks of each image\n",
        USAGE_PROGRAM_NAME, USAGE_PROGRAM_NAME);
    exit(exit_code);
}

void version() {
    printf(
        "Image-View %s, update on %s\n",
        IV_VERSION,
        IV_UPDATED);
    exit(EXIT_SUCCESS);
}

void help() {
    if (optarg) {
        printf("Help: %s\n", optarg);
    } else {
        printf("Generic help\n");
    }
    exit(EXIT_SUCCESS);
}

void handle_png(Format_PNG png) {
    png_chunk_t *chunk;
    
    while (chunk = png_chunk_next(png)) {
        printf("  Chunk: ");
        if (chunk == ((png_chunk_t *) -1)) {
            printf("error\n");
            break;
        }
        printf("%.4s (%d)\n", chunk->type, chunk->length);
        if (strncmp(chunk->type, PNG_CHUNK_TEXT_INT, 4) == 0) {
            char *text = png_iTXt_keyword(chunk);
            printf("   %s", text); free(text);
            text = png_iTXt_text(chunk);
            printf(": %s", text); free(text);

            text = png_iTXt_lang(chunk);
            if (text && *text) {
                printf(" (lang: %s)", text);
            }
            printf("\n");
            free(text);
        } else if (strncmp(chunk->type, PNG_CHUNK_TEXT, PNG_CHNK_LEN) == 0) {
            char *text = png_tEXt_keyword(chunk);
            printf("   %s", text); free(text);
            text = png_tEXt_text(chunk);
            printf(": %s\n", text); free(text);
        } else if (strncmp(chunk->type, PNG_CHUNK_TIME, PNG_CHNK_LEN) == 0) {
            printf("   Timestamp: %d-%02d-%02dT%02d:%02d:%02d\n",
                   png_tIME_year(chunk),
                   png_tIME_month(chunk),
                   png_tIME_day(chunk),
                   png_tIME_hour(chunk),
                   png_tIME_minute(chunk),
                   png_tIME_second(chunk));
        } else if (strncmp(chunk->type, PNG_CHUNK_PALETTE, PNG_CHNK_LEN) == 0) {
            int len = png_PLTE_length(chunk), i;
            char **palette = png_PLTE_get(chunk);
            printf("    Entries: %d\n", len);
            for (i = 0; i < 10 && i < len; i++) {
                printf("      #%d: 0x%02hhx%02hhx%02hhx\n", i, palette[i][0], palette[i][1], palette[i][2]);
            }
            if (i != len) printf("      ...\n");
            png_PLTE_free(palette, len);
        }

        png_chunk_free(chunk);
    }
}

int main(int argc, char **argv) {
    int i, rv, n, len = 0;
    const struct option options[] = {
        { "version", no_argument, NULL, 'v' },
        { "help", optional_argument, NULL, 'h' },
        { "verbose", optional_argument, NULL, 'V' },
        { "scan", no_argument, NULL, 's' },
        {NULL, 0, NULL, 0}, /* Last element must be all 0s */
    };
    
    /* Default options */
    iv_opts.verbose = 0;

    int option_index;
    int c;
    while (1) {
        c = getopt_long(argc, argv, "vh::V::s", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 'v':
                version();
                break;

            case 'h':
                help();
                break;

            case 'V':
                iv_opts.verbose = 1;
                printf("  ===== %s [%s] =====\n", IV_PROGRAM_NAME, IV_VERSION);
                break;

            case 's':
                iv_opts.scan = 1;
                break;

            case '?':
                exit(EXIT_FAILURE);

            default:
                usage(EXIT_FAILURE);
                break;
        }
    }

    if (optind == argc) {
        // TODO Read from stdin
        struct pollfd ps = { 0, POLLIN, 0 };

        int rv = poll(&ps, 1, 0);
        if (rv == 0) {
            if (iv_opts.verbose) {
                exit(1);
            } else {
                usage(EXIT_FAILURE);
            }
        } else {
            Format_PNG png = png_new();
            rv = png_open(png, NULL);
            if (rv != 0) {
                png_close(png);
                exit(EXIT_FAILURE);
            }
            if (iv_opts.scan) {
                handle_png(png);
            }
            png_close(png);
        }
    }

    // TODO Read files
    for (i = optind; i < argc; i++) {
        n = strlen(argv[i]);
        if (n > len) len = n;
    }
    
    n = argc - optind;
    for (i = optind; i < argc; i++) {
        Format_PNG png = png_new();
        png_chunk_t *chunk;
        if (access(argv[i], R_OK) == -1) {
            // TODO Error message for bad file
            printf("Could not open file %s\n", argv[i]);
            continue;
        }
        if (n != 1) {
            printf("%s %.*s\n", argv[i], (int) (len - strlen(argv[i]) + 7), "--------------------------------------");
        }

        rv = png_open(png, argv[i]);

        if (rv != 0) {
            png_close(png);
            continue;
        }

        if (iv_opts.scan) {
            handle_png(png);
        }
        png_close(png);
    }

    exit(EXIT_SUCCESS);
}