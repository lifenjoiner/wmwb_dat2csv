/*

wmwb: wang ma wu bi

jm: jian ma
qm: quan ma
zg: zeng guang? zi gen?

jp: jian pin
6j: 6 jian
jpp:
_p: pin
*/

#include <stdlib.h>
#include <stdio.h>
#include <direct.h>

/* **** */
char *g_Ver = "0.0.1 #20180302";

#define dict_item_size 0x130

#if !defined(_STDINT) && !defined(_STDINT_H) && !defined(__int8_t_defined)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#endif

/* 0x100, header
*/

/* 0x130, item
*/
/* len296 block
wchar_t, ...
x01: single word, length = 2
x10: special char, length = 2
0x2: single word, length = 4
x08: preset, length >= 2, doesn't need

uint8_t[40], wchar_t[64], char[192]
x04: phrase
*/

typedef struct _DICT_ITEM
{
    uint32_t type;
    char code[4];
    wchar_t word[296];
} DICT_ITEM, *PDICT_ITEM;

int main(int argc, char **argv)
{
    FILE *fp_r = NULL, *fp_w = NULL;
    char *filename_dat, *filename_csv;
    DICT_ITEM dict_item;
    int total_len, total_items, total_got, total_recognized, total_discard;
    int ret = 0;
    //
    if (argc != 3) {
        fprintf(stderr, "wmwb dat file extractor (GB 18030-2000) v%s @YX Hao\n", g_Ver);
        fprintf(stderr, "Usage: %s <input-dat-file> <ouput-csv-file>\n", argv[0]);
        return 1;
    }
    //
    filename_dat = argv[1];
    filename_csv = argv[2];
    fp_r = fopen(filename_dat, "rb");
    if (!fp_r) {
        fprintf(stderr, "Can't open file!\n");
        goto error;
    }
    total_len = _filelength(fileno(fp_r));
    total_len -= 0x100;
    if (total_len % dict_item_size != 0) {
        fprintf(stderr, "Wrong dat file format!\n");
        goto error;
    }
    total_items = total_len / dict_item_size;
    fprintf(stdout, "%s >>\n", filename_dat);
    fprintf(stdout, "items total: %d\n", total_items);
    //
    fp_w = fopen(filename_csv, "wb");
    if (!fp_w) {
        fprintf(stderr, "Can't open file!\n");
        goto error;
    }
    fprintf(stdout, "%s <<\n", filename_csv);
    //
    fseek(fp_r, 0x100, SEEK_SET);
    //
    total_items = 0; // as index
    total_got = 0;
    total_recognized = 0;
    total_discard = 0;
    while (fread(&dict_item, 1, dict_item_size, fp_r) == dict_item_size) {
        total_items++;
        switch (dict_item.type) {
        case 0x01:
        case 0x10:
            total_recognized++;
            if (fwrite(dict_item.code, 1, 4, fp_w) == 4
                && fputs("\t", fp_w) >= 0
                && fwrite(dict_item.word, 1, 2, fp_w) == 2
                && fputs("\n", fp_w) >= 0) {
                total_got++;
            }
            else {
                fprintf(stderr, "unknown index: %d\n", total_items);
            }
            break;
        case 0x02:
            total_recognized++;
            if (fwrite(dict_item.code, 1, 4, fp_w) == 4
                && fputs("\t", fp_w) >= 0
                && fwrite(dict_item.word, 1, 4, fp_w) == 4
                && fputs("\n", fp_w) >= 0) {
                total_got++;
            }
            else {
                fprintf(stderr, "unknown index: %d\n", total_items);
            }
            break;
        case 0x04:
            total_recognized++;
            if (fwrite(dict_item.code, 1, 4, fp_w) == 4
                && fputs("\t", fp_w) >= 0
                && fputws(dict_item.word + 20, fp_w) >= 0 // wide char
                && fputs("\n", fp_w) >= 0) {
                total_got++;
            }
            else {
                fprintf(stderr, "unknown index: %d\n", total_items);
            }
            break;
        case 0x08: // discard
            total_recognized++;
            total_discard++;
            break;
        default:
            fprintf(stderr, "unknown type, total_items: %d\n", dict_item.type, total_items);
            break;
        }
    }
    fflush(fp_w);
    //
    fprintf(stdout, "items recognized: %d\n", total_recognized);
    fprintf(stdout, "items got: %d\n", total_got);
    fprintf(stdout, "items discard: %d\n", total_discard);
    //
cleanup:
    if (fp_r) fclose(fp_r);
    if (fp_w) fclose(fp_w);
    //
    return ret;
    //
error:
    ret = 1;
    goto cleanup;
}
