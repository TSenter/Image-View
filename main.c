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
        "-s, --scan              View the chunks of each image\n"
        "-d, --scan-deep         View details of the chunks of each image\n",
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
        if (iv_opts.deep) {
            if (strncmp(chunk->type, PNG_CHUNK_TEXT_INT, 4) == 0) {
                char *text = png_iTXt_keyword(chunk);
                printf("    %s", text); free(text);
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
                printf("    %s", text); free(text);
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
            } else if (strncmp(chunk->type, PNG_CHUNK_TRANSPARENCY, PNG_CHNK_LEN) == 0) {
                int type = png_attr_col_type(png);

                if (type == 0) {
                    printf("    Grayscale sample: %hx\n", png_tRNS_gray(chunk));
                } else if (type == 2) {
                    short *rgb = png_tRNS_true(chunk);
                    printf("    Truecolor sample:\n      R = %hx\n      G = %hx\n      B = %hx\n", rgb[0], rgb[1], rgb[2]);
                    free(rgb);
                } else if (type == 3) {
                    int len = png_tRNS_index_length(chunk), i;
                    char *palette = png_tRNS_index(chunk);
                    printf("    Entries: %d\n", len);
                    for (i = 0; i < 10 && i < len; i++) {
                        printf("      #%d: %hhu\n", i, palette[i]);
                    }
                    if (i != len) printf("      ...\n");
                    free(palette);
                } else {
                    printf("    [Error]: Invalid color type %d\n", type);
                }
            } else if (strncmp(chunk->type, PNG_CHUNK_CHROMACITY, PNG_CHNK_LEN) == 0) {
                printf("    Chromacities:\n");
                printf("      White point (x): %.5f\n"
                       "      White point (y): %.5f\n"
                       "      Red (x):         %.5f\n"
                       "      Red (y):         %.5f\n"
                       "      Green (x):       %.5f\n"
                       "      Green (y):       %.5f\n"
                       "      Blue (x):        %.5f\n"
                       "      Blue (y):        %.5f\n",
                       png_cHRM_whiteX(chunk),
                       png_cHRM_whiteX(chunk),
                       png_cHRM_redX(chunk),
                       png_cHRM_redY(chunk),
                       png_cHRM_greenX(chunk),
                       png_cHRM_greenY(chunk),
                       png_cHRM_blueX(chunk),
                       png_cHRM_blueY(chunk));
            } else if (strncmp(chunk->type, PNG_CHUNK_GAMMA, PNG_CHNK_LEN) == 0) {
                printf("    Gamma: %.5f (0x%08x)\n", png_gAMA(chunk), png_gAMA_raw(chunk));
            } else if (strncmp(chunk->type, PNG_CHUNK_ICCP, PNG_CHNK_LEN) == 0) {
                char *profile = png_iCCP_name(chunk);
                
                printf("    Profile: '%s' (%d bytes, %s compression)\n", profile, png_iCCP_profile_len(chunk), png_iCCP_method(chunk) ? "unknown" : "zlib");

                free(profile);
            } else if (strncmp(chunk->type, PNG_CHUNK_SIGBITS, PNG_CHNK_LEN) == 0) {
                int type = png_attr_col_type(png);
                char *bits = png_sBIT_get(chunk);

                if (type == 0 || type == 4) {
                    printf("    Grayscale: 0x%hhx\n", bits[0]);
                } else if (type == 2 || type == 3 || type == 6) {
                    printf("    Red:       0x%hhx\n"
                           "    Green:     0x%hhx\n"
                           "    Blue:      0x%hhx\n",
                           bits[0], bits[1], bits[2]);
                }
                if (type == 4 || type == 6) {
                    printf("    Alpha:     0x%hhx\n", bits[type == 4 ? 2 : 3]);
                }

                free(bits);
            } else if (strncmp(chunk->type, PNG_CHUNK_SRGB, PNG_CHNK_LEN) == 0) {
                char intent = png_sRGB_get(chunk);
                char *s = "error";

                switch (intent) {
                    case 0:
                        s = "perceptual";
                        break;
                    case 1:
                        s = "relative colorimetric";
                        break;
                    case 2:
                        s = "saturation";
                        break;
                    case 3:
                        s = "absolute colorimetric";
                        break;
                }

                printf("    Rendering intent: %s\n", s);

            } else if (strncmp(chunk->type, PNG_CHUNK_BACKGROUND, PNG_CHNK_LEN) == 0) {
                int size = png_bKGD_len(chunk);
                void *array = png_bKGD_get(chunk);
                short *s = (short *) array;

                if (size == 2) {
                    printf("    Grayscale: 0x%hx\n", s[0]);
                } else if (size == 6) {
                    printf("    HTML Code: 0x%hx%hx%hx\n", s[0], s[1], s[2]);
                } else if (size == 1) {
                    printf("    Palette index: %hhd\n", *((char *)array));
                }

                free(array);
            }
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
        { "scan-deep", no_argument, NULL, 'd' },
        {NULL, 0, NULL, 0}, /* Last element must be all 0s */
    };
    
    /* Default options */
    iv_opts.verbose = 0;

    int option_index;
    int c;
    while (1) {
        c = getopt_long(argc, argv, "vh::V::sd", options, &option_index);

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

            case 'd':
                iv_opts.scan = 1;
                iv_opts.deep = 1;
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