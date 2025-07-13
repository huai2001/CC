/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#include <libcc/power.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

static const char *proc_apm_path = "/proc/apm";
static const char *proc_acpi_battery_path = "/proc/acpi/battery";
static const char *proc_acpi_ac_adapter_path = "/proc/acpi/ac_adapter";
static const char *sys_class_power_supply_path = "/sys/class/power_supply";

_CC_API_PRIVATE(void) check_proc_acpi_ac_adapter(const char *node, bool_t *have_ac);

_CC_API_PRIVATE(int) open_power_file(const char *base, const char *node, const char *key) {
    char path[_CC_MAX_PATH_ * 4];
    snprintf(path, _cc_countof(path), "%s/%s/%s", base, node, key);
    return open(path, O_RDONLY);
}

_CC_API_PRIVATE(bool_t) read_power_file(const char *base, const char *node, const char *key, char *buf,
                                         size_t buflen) {
    ssize_t br;
    const int fd = open_power_file(base, node, key);
    if (fd == -1) {
        return false;
    }

    br = read(fd, buf, buflen - 1);
    close(fd);

    if (br < 0) {
        return false;
    }
    /* nullptr-terminate the string. */
    buf[br] = '\0';
    return true;
}

_CC_API_PRIVATE(bool_t) make_proc_acpi_key_val(char **_ptr, char **_key, char **_val) {
    char *ptr = *_ptr;

    while (*ptr == ' ') {
        ptr++; /* skip whitespace. */
    }

    if (*ptr == '\0') {
        return false; /* EOF. */
    }

    *_key = ptr;

    while ((*ptr != ':') && (*ptr != '\0')) {
        ptr++;
    }

    if (*ptr == '\0') {
        return false; /* (unexpected) EOF. */
    }

    *(ptr++) = '\0'; /* terminate the key. */

    while ((*ptr == ' ') && (*ptr != '\0')) {
        ptr++; /* skip whitespace. */
    }

    if (*ptr == '\0') {
        return false; /* (unexpected) EOF. */
    }

    *_val = ptr;

    while ((*ptr != _CC_LF_) && (*ptr != '\0')) {
        ptr++;
    }

    if (*ptr != '\0') {
        *(ptr++) = '\0'; /* terminate the value. */
    }

    *_ptr = ptr; /* store for next time. */
    return true;
}

_CC_API_PRIVATE(void) check_proc_acpi_battery(const char *node, bool_t *have_battery, bool_t *charging,
                                               int32_t *seconds, byte_t *percent) {
    const char *base = proc_acpi_battery_path;
    char info[1024];
    char state[1024];
    char *ptr = nullptr;
    char *key = nullptr;
    char *val = nullptr;
    bool_t charge = false;
    bool_t choose = false;
    int32_t maximum = -1;
    int32_t remaining = -1;
    int32_t secs = -1;
    int32_t pct = -1;

    if (!read_power_file(base, node, "state", state, sizeof(state))) {
        return;
    } else if (!read_power_file(base, node, "info", info, sizeof(info))) {
        return;
    }

    ptr = &state[0];
    while (make_proc_acpi_key_val(&ptr, &key, &val)) {
        if (strcmp(key, "present") == 0) {
            if (strcmp(val, "yes") == 0) {
                *have_battery = true;
            }
        } else if (strcmp(key, "charging state") == 0) {
            /* !!! FIXME: what exactly _does_ charging/discharging mean? */
            if (strcmp(val, "charging/discharging") == 0) {
                charge = true;
            } else if (strcmp(val, "charging") == 0) {
                charge = true;
            }
        } else if (strcmp(key, "remaining capacity") == 0) {
            char *endptr = nullptr;
            const int cvt = (int)strtol(val, &endptr, 10);
            if (*endptr == ' ') {
                remaining = cvt;
            }
        }
    }

    ptr = &info[0];
    while (make_proc_acpi_key_val(&ptr, &key, &val)) {
        if (strcmp(key, "design capacity") == 0) {
            char *endptr = nullptr;
            const int cvt = (int)strtol(val, &endptr, 10);
            if (*endptr == ' ') {
                maximum = cvt;
            }
        }
    }

    if ((maximum >= 0) && (remaining >= 0)) {
        pct = (int32_t)((((float)remaining) / ((float)maximum)) * 100.0f);
        if (pct < 0) {
            pct = 0;
        } else if (pct > 100) {
            pct = 100;
        }
    }

    /* !!! FIXME: calculate (secs). */

    /*
     * We pick the battery that claims to have the most minutes left.
     *  (failing a report of minutes, we'll take the highest percent.)
     */
    if ((secs < 0) && (*seconds < 0)) {
        if ((pct < 0) && (*percent < 0)) {
            choose = true; /* at least we know there's a battery. */
        }
        if (pct > *percent) {
            choose = true;
        }
    } else if (secs > *seconds) {
        choose = true;
    }

    if (choose) {
        *seconds = secs;
        *percent = (byte_t)pct;
        *charging = charge;
    }
}

_CC_API_PRIVATE(void) check_proc_acpi_ac_adapter(const char *node, bool_t *have_ac) {
    const char *base = proc_acpi_ac_adapter_path;
    char state[256];
    char *ptr = nullptr;
    char *key = nullptr;
    char *val = nullptr;

    if (!read_power_file(base, node, "state", state, sizeof(state))) {
        return;
    }

    ptr = &state[0];
    while (make_proc_acpi_key_val(&ptr, &key, &val)) {
        if (strcmp(key, "state") == 0) {
            if (strcmp(val, "on-line") == 0) {
                *have_ac = true;
            }
        }
    }
}

_CC_API_PRIVATE(bool_t) _sys_get_power_info_acpi(_CC_POWER_STATE_ENUM_ *state, int32_t *seconds, byte_t *percent) {
    struct dirent *dent = nullptr;
    DIR *dirp = nullptr;
    bool_t have_battery = false;
    bool_t have_ac = false;
    bool_t charging = false;

    *seconds = -1;
    *percent = -1;
    *state = _CC_POWERSTATE_UNKNOWN_;

    dirp = opendir(proc_acpi_battery_path);
    if (dirp == nullptr) {
        return false; /* can't use this interface. */
    } else {
        while ((dent = readdir(dirp)) != nullptr) {
            const char *node = dent->d_name;
            check_proc_acpi_battery(node, &have_battery, &charging, seconds, percent);
        }
        closedir(dirp);
    }

    dirp = opendir(proc_acpi_ac_adapter_path);
    if (dirp == nullptr) {
        return false; /* can't use this interface. */
    } else {
        while ((dent = readdir(dirp)) != nullptr) {
            const char *node = dent->d_name;
            check_proc_acpi_ac_adapter(node, &have_ac);
        }
        closedir(dirp);
    }

    if (!have_battery) {
        *state = _CC_POWERSTATE_NO_BATTERY_;
    } else if (charging) {
        *state = _CC_POWERSTATE_CHARGING_;
    } else if (have_ac) {
        *state = _CC_POWERSTATE_CHARGED_;
    } else {
        *state = _CC_POWERSTATE_ON_BATTERY_;
    }

    return true; /* definitive answer. */
}

_CC_API_PRIVATE(bool_t) next_string(char **_ptr, char **_str) {
    char *ptr = *_ptr;
    char *str = *_str;

    /* skip any spaces... */
    while (*ptr == ' ') {
        ptr++;
    }

    if (*ptr == '\0') {
        return false;
    }

    str = ptr;
    while ((*ptr != ' ') && (*ptr != _CC_LF_) && (*ptr != '\0')) {
        ptr++;
    }

    if (*ptr != '\0') {
        *(ptr++) = '\0';
    }

    *_str = str;
    *_ptr = ptr;
    return true;
}

_CC_API_PRIVATE(bool_t) int_string(char *str, int *val) {
    char *endptr = nullptr;
    *val = (int)strtol(str, &endptr, 0);
    return ((*str != '\0') && (*endptr == '\0'));
}

/* http://lxr.linux.no/linux+v2.6.29/delegates/char/apm-emulation.c */
_CC_API_PRIVATE(bool_t) _sys_get_power_info_apm(_CC_POWER_STATE_ENUM_ *state, int32_t *seconds, byte_t *percent) {
    bool_t need_details = false;
    int ac_status = 0;
    int battery_status = 0;
    int battery_flag = 0;
    int battery_percent = 0;
    int battery_time = 0;
    const int fd = open(proc_apm_path, O_RDONLY);
    char buf[128];
    char *ptr = &buf[0];
    char *str = nullptr;
    ssize_t br;

    if (fd == -1) {
        return false; /* can't use this interface. */
    }

    br = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (br < 0) {
        return false;
    }

    buf[br] = '\0';                 /* nullptr-terminate the string. */
    if (!next_string(&ptr, &str)) { /* delegate version */
        return false;
    }
    if (!next_string(&ptr, &str)) { /* BIOS version */
        return false;
    }
    if (!next_string(&ptr, &str)) { /* APM flags */
        return false;
    }

    if (!next_string(&ptr, &str)) { /* AC line status */
        return false;
    } else if (!int_string(str, &ac_status)) {
        return false;
    }

    if (!next_string(&ptr, &str)) { /* battery status */
        return false;
    } else if (!int_string(str, &battery_status)) {
        return false;
    }
    if (!next_string(&ptr, &str)) { /* battery flag */
        return false;
    } else if (!int_string(str, &battery_flag)) {
        return false;
    }
    if (!next_string(&ptr, &str)) { /* remaining battery life percent */
        return false;
    }
    if (str[strlen(str) - 1] == '%') {
        str[strlen(str) - 1] = '\0';
    }
    if (!int_string(str, &battery_percent)) {
        return false;
    }

    if (!next_string(&ptr, &str)) { /* remaining battery life time */
        return false;
    } else if (!int_string(str, &battery_time)) {
        return false;
    }

    if (!next_string(&ptr, &str)) { /* remaining battery life time units */
        return false;
    } else if (strcmp(str, "min") == 0) {
        battery_time *= 60;
    }

    if (battery_flag == 0xFF) { /* unknown state */
        *state = _CC_POWERSTATE_UNKNOWN_;
    } else if (battery_flag & (1 << 7)) { /* no battery */
        *state = _CC_POWERSTATE_NO_BATTERY_;
    } else if (battery_flag & (1 << 3)) { /* charging */
        *state = _CC_POWERSTATE_CHARGING_;
        need_details = true;
    } else if (ac_status == 1) {
        *state = _CC_POWERSTATE_CHARGED_; /* on AC, not charging. */
        need_details = true;
    } else {
        *state = _CC_POWERSTATE_ON_BATTERY_;
        need_details = true;
    }

    *percent = -1;
    *seconds = -1;
    if (need_details) {
        if (battery_percent >= 0) {                                             /* -1 == unknown */
            *percent = (byte_t)(battery_percent > 100) ? 100 : battery_percent; /* clamp between 0%, 100% */
        }
        if (battery_time >= 0) { /* -1 == unknown */
            *seconds = battery_time;
        }
    }

    return true;
}

_CC_API_PRIVATE(bool_t) _sys_get_sys_class_power_supply(_CC_POWER_STATE_ENUM_ *state, int32_t *seconds, byte_t *percent) {
    const char *base = sys_class_power_supply_path;
    struct dirent *dent;
    DIR *dirp;

    dirp = opendir(base);
    if (!dirp) {
        return false;
    }
    /* assume we're just plugged in. */
    *state = _CC_POWERSTATE_NO_BATTERY_;
    *seconds = -1;
    *percent = -1;

    while ((dent = readdir(dirp)) != nullptr) {
        const char *name = dent->d_name;
        bool_t choose = false;
        char str[64];
        _CC_POWER_STATE_ENUM_ st;
        int secs;
        int pct;
        int energy;
        int power;

        if ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0)) {
            /* skip these, of course. */
            continue;
        } else if (!read_power_file(base, name, "type", str, sizeof(str))) {
            /* Don't know _what_ we're looking at. Give up on it. */
            continue;
        } else if (strcmp(str, "Battery\n") != 0) {
            /* we don't care about UPS and such. */
            continue;
        }

        /* if the scope is "device," it might be something like a PS4
           controller reporting its own battery, and not something that powers
           the system. Most system batteries don't list a scope at all; we
           assume it's a system battery if not specified. */
        if (read_power_file(base, name, "scope", str, sizeof(str))) {
            if (strcmp(str, "device\n") == 0) {
                /* skip external devices with their own batteries. */
                continue;
            }
        }

        /* some delegates don't offer this, so if it's not explicitly reported
         * assume it's present. */
        if (read_power_file(base, name, "present", str, sizeof(str)) && (strcmp(str, "0\n") == 0)) {
            st = _CC_POWERSTATE_NO_BATTERY_;
        } else if (!read_power_file(base, name, "status", str, sizeof(str))) {
            st = _CC_POWERSTATE_UNKNOWN_;
        } else if (strcmp(str, "Charging\n") == 0) {
            st = _CC_POWERSTATE_CHARGING_;
        } else if (strcmp(str, "Discharging\n") == 0) {
            st = _CC_POWERSTATE_ON_BATTERY_;
        } else if ((strcmp(str, "Full\n") == 0) || (strcmp(str, "Not charging\n") == 0)) {
            st = _CC_POWERSTATE_CHARGED_;
        } else {
            st = _CC_POWERSTATE_UNKNOWN_;
        }

        if (!read_power_file(base, name, "capacity", str, sizeof(str))) {
            pct = -1;
        } else {
            pct = atoi(str);
            pct = (pct > 100) ? 100 : pct; /* clamp between 0%, 100% */
        }

        if (read_power_file(base, name, "time_to_empty_now", str, sizeof(str))) {
            secs = atoi(str);
            /* 0 == unknown */
            secs = (secs <= 0) ? -1 : secs;
        } else if (st == _CC_POWERSTATE_ON_BATTERY_) {
            /* energy is Watt*hours and power is Watts */
            energy = (read_power_file(base, name, "energy_now", str, sizeof(str))) ? atoi(str) : -1;
            power = (read_power_file(base, name, "power_now", str, sizeof(str))) ? atoi(str) : -1;
            secs = (energy >= 0 && power > 0) ? (3600LL * energy) / power : -1;
        } else {
            secs = -1;
        }

        /*
         * We pick the battery that claims to have the most minutes left.
         *  (failing a report of minutes, we'll take the highest percent.)
         */
        if ((secs < 0) && (*seconds < 0)) {
            if ((pct < 0) && (*percent < 0)) {
                choose = true; /* at least we know there's a battery. */
            } else if (pct > *percent) {
                choose = true;
            }
        } else if (secs > *seconds) {
            choose = true;
        }

        if (choose) {
            *seconds = secs;
            *percent = pct;
            *state = st;
        }
    }

    closedir(dirp);
    /* don't look any further. */
    return true;
}

_CC_API_PUBLIC(bool_t) _cc_get_sys_power_info(_CC_POWER_STATE_ENUM_ *state, int32_t *seconds, byte_t *percent) {
    if (_sys_get_sys_class_power_supply(state, seconds, percent)) {
        return true;
    }

    if (_sys_get_power_info_acpi(state, seconds, percent)) {
        return true;
    }

    return _sys_get_power_info_apm(state, seconds, percent);
}
