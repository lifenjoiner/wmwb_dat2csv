/* wmwb dat2csv

alias
wmwbh: wang ma wu bi hun
wmwb: wang ma wu bi

jm.dat: jian ma
qm.dat: quan ma
zg.dat: zi gen

cmts: chong ma ti shi
kmts: kong ma ti shi
zgts: zi gen ti shi
pyts: zi gen ti shi
6jpjbts: 6 Jian PuJiBan TiShi

jp: jian pin
6j: 6 jian

*****
https://zh.wikipedia.org/wiki/GB_18030

*****
有些字因为很常用，它可能有一级简码，也可能同时还有二级简码和三级简码。
如“经”字，就有一、二、三级简码。
经：X（加空格）
经：XC （加空格）
经：XCG（加空格）
*/

/* change log
0.2.1 #20180307
* Optimize jm lookup
+ Output item type

0.2.0 #20180305
+ Dump "*jm.dat"

0.1.0 #20180302
  Dump "*qm.dat"
*/

#include <stdlib.h>
#include <stdio.h>
#include <direct.h>

/* **** */
char *g_Ver = "0.2.2 #20191201";

#if !defined(_STDINT) && !defined(_STDINT_H) && !defined(__int8_t_defined)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#endif

/* < 2018 */
/* 0x100, header
*/

/* qm.dat
0x130, item
uint8_t[4], char[4], len296
*/
/* len296 block
wchar_t, ...
x01: single word, length = 2
x10: special char, length <= 2
x02: single word, length = 4

x08: preset, length >= 1, doesn't need

x04: phrase
uint8_t[40], wchar_t[64], char[192]
*/

typedef struct _DICT_ITEM_QM
{
    uint32_t type;
    char code[4];
    uint8_t word[296];
} DICT_ITEM_QM, *PDICT_ITEM_QM;

/* jm.dat
x100: header
0x8: item

1: 25, index = key[0] - 97
2: 625 = 25*25, index = 30 * (key[0] - 96) + key[1] - 96
3: leftover (4440), index = 30 * (30 * (key[0] - 96) + key[1] - 96) + key[2] - 96
*/
typedef struct _DICT_ITEM_JM
{
    uint32_t idx;
    uint8_t word[4];
} DICT_ITEM_JM, *PDICT_ITEM_JM;

int dict_item_qm_size = 0x130;
int dict_item_jm_size = 0x08;

/* 2018 */
typedef struct _DICT_ITEM_QM_XA
{
    uint32_t type;
    uint32_t st;    // 1: simplified; 2: traditional; 3: 日、韩、香港地区汉字. output n-1
    char code[4];
    uint8_t word[296];
} DICT_ITEM_QM_XA, *PDICT_ITEM_QM_XA;

typedef struct _DICT_ITEM_JM_XA
{
    uint32_t idx;
    uint8_t word[20];
    uint8_t st;    // 0: simplified; 1: traditional
} DICT_ITEM_JM_XA, *PDICT_ITEM_JM_XA;

int dict_item_qm_size_xa = 0x134;
int dict_item_jm_size_xa = 0x19;

/* reform different version data */
uint32_t ver_dat;

typedef struct _DICT_ITEM
{
    uint8_t type;
    char code[5];
    uint8_t *word;
    uint8_t st;
} DICT_ITEM, *PDICT_ITEM;

/*****/
int convert_qm(FILE *fp_r, FILE *fp_w)
{
    uint8_t *pdict_item_qm = NULL;
    DICT_ITEM dict_item_qm = {0};
    int total_len, counter, total_got, total_recognized, total_discard;
    //
    if (ver_dat >= 0x0A) {
        dict_item_qm_size = dict_item_qm_size_xa;
    }
    pdict_item_qm = malloc(dict_item_qm_size);
    //
    total_got = 0;
    total_len = _filelength(fileno(fp_r));
    total_len -= 0x100;
    if (total_len % dict_item_qm_size != 0) {
        fprintf(stderr, "Wrong dat file format!\n");
        goto cleanup;
    }
    counter = total_len / dict_item_qm_size;
    fprintf(stdout, "items total: %d\n", counter);
    //
    fseek(fp_r, 0x100, SEEK_SET);
    //
    counter = 0; // as index
    total_recognized = 0;
    total_discard = 0;
    while (fread(pdict_item_qm, 1, dict_item_qm_size, fp_r) == dict_item_qm_size) {
        int offset_word = 0;
        counter++;
        if (ver_dat < 0x0A) {
            dict_item_qm.type = ((PDICT_ITEM_QM)pdict_item_qm)->type;
            *(uint32_t*)dict_item_qm.code = *(uint32_t*)(((PDICT_ITEM_QM)pdict_item_qm)->code);
            dict_item_qm.word = ((PDICT_ITEM_QM)pdict_item_qm)->word;
        }
        else {
            dict_item_qm.type = ((PDICT_ITEM_QM_XA)pdict_item_qm)->type;
            *(uint32_t*)dict_item_qm.code = *(uint32_t*)(((PDICT_ITEM_QM_XA)pdict_item_qm)->code);
            dict_item_qm.word = ((PDICT_ITEM_QM_XA)pdict_item_qm)->word;
            dict_item_qm.st = ((PDICT_ITEM_QM_XA)pdict_item_qm)->st - 1;
        }
        //
        switch (dict_item_qm.type) {
        case 0x01:
        case 0x10:
        case 0x02:
            dict_item_qm.word[4] = 0; // drop following
            break;
        case 0x04:
            offset_word = 40;
        case 0x08:
            break;
        default:
            fprintf(stderr, "unknown type, counter: %d\n", dict_item_qm.type, counter);
            continue;
        }
        //
        total_recognized++;
        dict_item_qm.word += offset_word;
        if (fprintf(fp_w, "%d\t%s\t%s", dict_item_qm.type, dict_item_qm.code, dict_item_qm.word) > 7
            && (ver_dat >= 0x0A ? fprintf(fp_w, "\t%d", dict_item_qm.st) > 1 : 1)
            && fputs("\n", fp_w) >= 0
        ) {
            total_got++;
        }
        else {
            fprintf(stderr, "unknown with index: %d\n", counter);
        }
    }
    fflush(fp_w);
    //
    fprintf(stdout, "items recognized: %d\n", total_recognized);
    fprintf(stdout, "items got: %d\n", total_got);
    fprintf(stdout, "items discard: %d\n", total_discard);
    //
cleanup:
    free(pdict_item_qm);
    return total_got;
}

int lookup_jm_item(FILE *fp_r, int idx, uint8_t * pdict_item_jm)
{   // l < b
    while (fread(pdict_item_jm, 1, dict_item_jm_size, fp_r) == dict_item_jm_size) {
        if (*(uint32_t*)pdict_item_jm == idx) {
            return 1;
        }
        else if (*(uint32_t*)pdict_item_jm > idx) {
            fseek(fp_r, -dict_item_jm_size, SEEK_CUR);
            break;
        }
    }
    return 0;
}

int fwrite_jm_item(uint32_t code, uint8_t *pdict_item_jm, FILE *fp_w)
{
    if (ver_dat < 0x0A) {
        uint8_t CHWORD[5] = {0}; // strip next index
        *(uint32_t*)CHWORD = *(uint32_t*)(((PDICT_ITEM_JM)pdict_item_jm)->word);
        fprintf(fp_w, "1\t%s\t%s\n", &code, CHWORD);
    }
    else {
        fprintf(fp_w, "1\t%s\t%s\t%d\n", &code,
            ((PDICT_ITEM_JM_XA)pdict_item_jm)->word,
            ((PDICT_ITEM_JM_XA)pdict_item_jm)->st
        );
    }
}

int convert_jm(FILE *fp_r, FILE *fp_w)
{
    uint8_t *pdict_item_jm;
    int total_len, total_items, total_got;
    int k1, k2, k3, idx;
    int sets;
    //
    if (ver_dat >= 0x0A) {
        dict_item_jm_size = dict_item_jm_size_xa;
    }
    pdict_item_jm = malloc(dict_item_jm_size);
    //
    total_got = 0;
    total_len = _filelength(fileno(fp_r));
    total_len -= 0x100;
    if (total_len % dict_item_jm_size != 0) {
        fprintf(stderr, "Wrong dat file format!\n");
        goto cleanup;
    }
    total_items = total_len / dict_item_jm_size;
    fprintf(stdout, "items total: %d\n", total_items);
    //
    fseek(fp_r, 0x100, SEEK_SET);

    sets = 0;
dump_jm:
    // 1, 25
    for (k1 = 'a'; k1 < 'z'; k1++) {
        idx = k1 - 97;
        if (lookup_jm_item(fp_r, idx, pdict_item_jm) > 0) { // >=1
            fwrite_jm_item(k1, pdict_item_jm, fp_w);
            total_got++;
        }
    }
    // 2, 25 * 25
    for (k1 = 'a'; k1 < 'z'; k1++) {
        for (k2 = 'a'; k2 < 'z'; k2++) {
            idx = 30 * (k1 - 96) + k2 - 96;
            while (lookup_jm_item(fp_r, idx, pdict_item_jm) > 0) {
                fwrite_jm_item(k1 | k2<<8, pdict_item_jm, fp_w);
                total_got++;
            }
        }
    }
    // 3, 25 * 25 * 25
    for (k1 = 'a'; k1 < 'z'; k1++) {
        for (k2 = 'a'; k2 < 'z'; k2++) {
            for (k3 = 'a'; k3 < 'z'; k3++) {
                idx = 30 * (30 * (k1 - 96) + k2 - 96) + k3 - 96;
                while (lookup_jm_item(fp_r, idx, pdict_item_jm) > 0) {
                    if (ver_dat >= 0x0A && ((PDICT_ITEM_JM_XA)pdict_item_jm)->st != sets) continue;
                    fwrite_jm_item(k1 | k2<<8 | k3<<16, pdict_item_jm, fp_w);
                    total_got++;
                }
            }
        }
    }
    sets++;
    // traditional
    if (ver_dat >= 0x0A && sets < 2) {
        fseek(fp_r, 0x100 + total_got * dict_item_jm_size, SEEK_SET);
        goto dump_jm;
    }

    fflush(fp_w);
    //
    fprintf(stdout, "items got: %d\n", total_got);
    //
cleanup:
    free(pdict_item_jm);
    return total_got;
}

/*****/
int main(int argc, char **argv)
{
    FILE *fp_r = NULL, *fp_w = NULL;
    char *filename_dat, *filename_csv;
    int ret = 0;
    //
    if (argc != 4) {
        fprintf(stderr, "wmwb dat file extractor (GB 18030-2000) v%s @YX Hao\n", g_Ver);
        fprintf(stderr, "Usage: %s <q|j> <input-dat-file> <ouput-csv-file>\n", argv[0]);
        fprintf(stderr, "   q   qm dat file\n");
        fprintf(stderr, "   j   jm dat file\n");
        fprintf(stderr, "sets: 0 = simplified; 1 = traditional; 2 = jp, kr, hk, vi, etc.\n");
        return 1;
    }
    //
    filename_dat = argv[2];
    filename_csv = argv[3];
    fp_r = fopen(filename_dat, "rb");
    if (!fp_r) {
        fprintf(stderr, "Can't open file!\n");
        goto error;
    }
    fprintf(stdout, "%s >>\n", filename_dat);
    //
    fseek(fp_r, 0x80, SEEK_SET);
    if (fread(&ver_dat, 1, 4, fp_r) != 4) {
        fprintf(stderr, "too small file size!\n");
        goto error;
    }
    //
    fp_w = fopen(filename_csv, "wb");
    if (!fp_w) {
        fprintf(stderr, "Can't open file!\n");
        goto error;
    }
    fprintf(stdout, "%s <<\n", filename_csv);
    //
    if (ver_dat < 0x0A) {
        fprintf(fp_w, "%type\tkeys\twords\n");
    }
    else {
        fprintf(fp_w, "%type\tkeys\twords\tsets\n");
    }
    switch (argv[1][0]) {
    case 'q':
        ret = convert_qm(fp_r, fp_w) > 0;
        break;
    case 'j':
        ret = convert_jm(fp_r, fp_w) > 0;
        break;
    default: break;
    }
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
