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
        "-v, --version           Print version information\n",
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
        if (strncmp(chunk->type, "iTXt", 4) == 0) {
            char *text = png_iTXt_text(chunk);
            printf("   %s: %s\n", png_iTXt_keyword(chunk), text);
            free(text);
        }

        png_chunk_free(chunk);
    }

    // png_debug(png);
}

int main(int argc, char **argv) {
    int i, rv;
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
            handle_png(png);
            png_close(png);
        }
    }

    // TODO Read files
    for (i = optind; i < argc; i++) {
        Format_PNG png = png_new();
        png_chunk_t *chunk;
        if (access(argv[i], R_OK) == -1) {
            // TODO Error message for bad file
            printf("File does not exist: %s\n", argv[i]);
            continue;
        }
        rv = png_open(png, argv[i]);

        if (rv != 0) {
            png_close(png);
            continue;
        }

        handle_png(png);
        png_close(png);
    }

    exit(EXIT_SUCCESS);
}