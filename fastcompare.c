#include "fastcompare.h"



/*
given a pointer to arrays of unsigned chars by Python, gets lazy hamming label. 1 for dup 0 for non-dup.
*/
void find_hammings_and_mark(char *in[], int outs_labels[], int len_rows, int maxlen) {  
    uint8_t (*nested_array)[len_rows][maxlen] = (uint8_t (*)[len_rows][maxlen])in; 
    
    int i = -1;
    int j = -1;
    int m = -1;
    int cmp;
    char *s1;
    char *s2;
    size_t strlen1;
    size_t strlen2;

    while (i++ < len_rows) {
        if (outs_labels[i] != 0) continue;

        j = i + 1;
        while (j++ < len_rows) {
            if (outs_labels[j] != 0) continue;

            s1 = (char *)(*nested_array)[i];
            s2 = (char *)(*nested_array)[j];
            strlen1 = strlen(s1);
            strlen2 = strlen(s2);
            
            if (strlen1 != strlen2) continue;

            m = -1;
            cmp = 0;
            while ((m++ < (int)strlen1) && (cmp <= 2)) {
                cmp += s1[m] ^ s2[m];
            };

            outs_labels[j] = (cmp <= 1) ? i : -1; 
        }
    }
}

