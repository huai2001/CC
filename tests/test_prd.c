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
        _cc_prd(&prd, i);
        printf("P:%d, C:%lf, N:%d\n", i, prd.c, prd.nmax);
    }
    _cc_prd(&prd, 1.0/650.0*100.0);
    while(w--) {
        if (_cc_get_probability(&prd,++T)) {
            T = 0;
            p1++;
        } else {
            p2++;
        }
    }
    printf("P1:%d,P2:%d\n", p1, p2);

    // 1. 设置目标概率为50%
    float64_t target_probability = 1.0/650.0*100.0;
    _cc_prd(&prd, target_probability);
    
    // 2. 使用概率分布进行随机判断
    int success_count = 0;
    int total_tests = 1000;
    
    for (int i = 0; i < total_tests; i++) {
        // 测试不同时间点T的概率
        int T = 1;// + (i % 10); // 模拟不同时间点
        if (_cc_get_probability(&prd, T)) {
            success_count++;
        }
    }
    
    printf("在%d次测试中成功%d次，成功率: %.2f%%\n", 
           total_tests, success_count, (float)success_count/total_tests*100);
    return 0;
}

