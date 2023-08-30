#include <stdio.h>

#include <cc/alloc.h>
#include <cc/power.h>

int main (int argc, char * const argv[]) {
    char c;
    int32_t seconds; 
    byte_t percent;
    _CC_POWER_STATE_ENUM_ power;

    power = _cc_get_power_info(&seconds, &percent);
    if (seconds == 0xffffffff)
        seconds = 0;
    else
        seconds = seconds / 3600;

    if (percent == 0xff) percent = 0;

    switch(power) {
    case _CC_POWERSTATE_ON_BATTERY_:
        _tprintf(_T("Not plugged in, running on the battery. Info: %d Hour : %d %%.\n"), seconds, percent);
        break;
    case _CC_POWERSTATE_NO_BATTERY_:
        _tprintf(_T("Plugged in, no battery available. Info: %d Hour : %d %%.\n"), seconds, percent);
        break;
    case _CC_POWERSTATE_CHARGING_:
        _tprintf(_T("Plugged in, charging battery. Info: %d Hour : %d %%.\n"), seconds, percent);
        break;
    case _CC_POWERSTATE_CHARGED_:
        _tprintf(_T("Plugged in, battery charged. Info: %d Hour : %d %%.\n"), seconds, percent);
        break;
    default:
        _tprintf(_T("power Unknown.\n"));
        break;
    }

    while ((c = getchar()) != 'q');

    return 0;
}
