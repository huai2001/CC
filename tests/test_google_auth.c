#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libcc.h>

int main() {
    const char *secret = "JBSWY3DPEHPK3PXP";  // Base32 密钥
    bool_t valid;

	uint32_t user_code = _cc_generate_totp(secret, 30);
    printf("Generated TOTP code: %u\n", user_code);
    
    /* 方法1: 严格验证（当前时间点）*/
    valid = _cc_verify_totp(secret, user_code, 30, 0);
    printf("Strict validation: %s\n", valid ? "PASS" : "FAIL");

	//30秒后再验证
    _cc_sleep(60000);

    /* 方法2: 宽松验证（±1个时间步，即前后30秒）*/
    valid = _cc_verify_totp(secret, user_code, 30, 1);
    printf("Window validation (±30s): %s\n", valid ? "PASS" : "FAIL");
    
    /* 方法3: 标准验证（±2个时间步，即前后60秒）*/
    valid = _cc_verify_totp(secret, user_code, 30, 2);
    printf("Standard validation (±60s): %s\n", valid ? "PASS" : "FAIL");
    
    return 0;
}
