#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libcc.h>

int main (int argc, char* argv[]) {
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
            T = 0;
            p1++;
        } else {
            p2++;
        }
    }
    printf("P1:%d,P2:%d\n", p1, p2);
   
    return 0;
}

