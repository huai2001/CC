#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libcc.h>

int main() {
    tchar_t secret[13];
    uint32_t user_code;
    uint32_t prev_code = 0;
    bool_t valid;
    uint32_t countdown;
    time_t now;
    int iteration = 0;

    /* Generate otpauth URL with user and host info */
    _cc_generate_secret(secret, _cc_countof(secret));
    
    printf("\n========== Google Authenticator TOTP Test ==========\n\n");
    printf("1. Scan this QR Code with Google Authenticator app:\n");
    printf("otpauth://totp/Google:alice?secret=%*s&issuer=Google\n\n", (int)_cc_countof(secret) - 1, secret);
    printf("2. Or manually enter this secret: %s\n\n", secret);
    /* Verify the secret is valid */
    user_code = _cc_generate_totp(secret, 30);
    printf("Initial TOTP code: %06u\n\n", user_code);
    
    /* Test verification functions */
    valid = _cc_verify_totp(secret, user_code, 30, 0);
    printf("Strict validation (window=0): %s\n", valid ? "✓ PASS" : "✗ FAIL");
    
    valid = _cc_verify_totp(secret, user_code, 30, 1);
    printf("Window validation (window=1): %s\n\n", valid ? "✓ PASS" : "✗ FAIL");
    
    printf("========== TOTP Display (Sync with App) ==========\n");
    printf("Press Ctrl+C to exit\n\n");
    
    /* Sync display with 30-second TOTP period */
    while (1) {
        now = time(NULL);
        countdown = 30 - (now % 30);
        
        user_code = _cc_generate_totp(secret, 30);
        
        printf("\r[%03d] TOTP Code: %06u | Expires in: %2u seconds", iteration, user_code, countdown);
        /* Display with real-time countdown (clear and redraw) */
        if (user_code != prev_code || iteration == 0) {
            prev_code = user_code;
            iteration++;
        }
        fflush(stdout);
        
        /* Sleep for 1 second to update countdown */
        _cc_sleep(1000);
    }
    
    return 0;
}
