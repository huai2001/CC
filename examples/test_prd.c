#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libcc.h>

int main (int argc, char* argv[]) {
    /*
    int T = 0;
    int p1 = 0;
    int p2 = 0;
    int w = 1500;
    int i;
    _cc_prd_t prd;
    srand(time(NULL));

    for (i = 1; i < 100; i++) {
        _cc_calculate_prd(&prd, i);
        printf("P:%d, C:%lf, N:%d\n", i, prd.C, prd.NMax);
    }
    _cc_calculate_prd(&prd, 1.0/650.0*100.0);
    while(w--) {
        if (_cc_get_probability(&prd,++T)) {
            //printf("%d.±©»÷\n", T);
            T = 0;
            p1++;
        } else {
            p2++;
            //printf("%d.¹¥»÷\n", T);
        }
    }
    printf("P1:%d,P2:%d\n", p1, p2);
    */
    ssize_t read, i;
    char line[1024];
    char line2[1024];
    int len = 0;
    FILE *wfp = _tfopen("sensitive_words.js", "w");
    FILE *fp = _tfopen("sensitive_words_lines.txt", "r");
    fputs("var sensitive_words = [", wfp);
    while (!feof(fp) && fgets(line, 1024, fp) != NULL) {
        read = strlen(line);
        line[read - 1] = 0;
        len = 0;
        for (i = 0; i < read; i++) {
            if (line[i] == '"') {
                line2[len++] = '\\';
                line2[len++] = '"';
            } else if (line[i] == '\\') {
            } else {
                line2[len++] = line[i];
            }
        }
        line2[len++] = 0;
        fprintf(wfp, "\"%s\",", line2);
    }
    fputs("];", wfp);
    return 0;
}

