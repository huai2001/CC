#include <stdio.h>
#include <libcc.h>

static FILE *log_file = NULL;
static bool_t log_running = false;
static int log_wday = -1;
static bool_t log_flush = false;

static void header(uint8_t level, time_t timestamp) {
    struct tm tm_now;
    static const char SYSLOG_LEVEL_CODE[_CC_LOG_LEVEL_DEBUG_ + 1] = {'G', 'A', 'C', 'E', 'W', 'N', 'I', 'D'};
    _cc_localtime(&timestamp, &tm_now);
    if (log_wday != tm_now.tm_wday || log_file == NULL) {
        tchar_t log_name[64] = {0};
    #ifdef _CC_WINDOWS_
        sprintf(log_name, "test_%04d_%02d_%02d.log", tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday);
    #else
        sprintf(log_name, "test_%04d_%02d_%02d.log", tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday);
    #endif
        if (log_file) {
            fclose(log_file);
        }
        log_file = fopen(log_name, "a+");
        log_wday = tm_now.tm_wday;
    }
    fprintf(log_file, "<%c>%04d-%02d-%02d %02d:%02d:%02d ",
                                SYSLOG_LEVEL_CODE[level], 
                                tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                                tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
    log_flush = true;
}

static void _alog(uint8_t level, time_t timestamp, const char_t *msg, int32_t length) {
    const char_t ch = *(msg + length - sizeof(char_t));
    header(level, timestamp);
    fwrite(msg, sizeof(char_t), length, log_file);
    if (ch != '\n' && ch != '\r') {
        fputwc('\n', log_file);
    }
}

static void _wlog(uint8_t level, time_t timestamp, const wchar_t *msg, int32_t length) {
    const wchar_t ch = *(msg + length - sizeof(wchar_t));
    header(level, timestamp);
    fwrite(msg, sizeof(wchar_t), length, log_file);
    if (ch != L'\n' && ch != L'\r') {
        fputwc(L'\n', log_file);
    }
}

static int _dump(void *args) {
    while (log_running) {
        _cc_loggerA_dump(_alog);
        _cc_loggerW_dump(_wlog);
        _cc_sleep(100);

        if (log_file && log_flush) {
            fflush(log_file);
            log_flush = false;
        }
    }
    return 1;
}

static void _log_init(void) {
    log_running = true;
    _cc_thread_start(_dump, "logger.dump", NULL);
}

static int thread_logger(void *args) {
    int i = 0;
    int thread_id = _cc_get_thread_id(NULL);
    //while (1) {
    for (;i < 200;) {
        _cc_logger_debug("%s id:%d, i:%03f, i:%06u, i:0x%0.7x, %s :0x%0.8xAA", "test string", thread_id, (float)i*0.002f, i, i, "i << 8", i << 8);    
        i++;
    }
    return 1;
}

int main (int argc, char * const argv[]) {
	char c = 0;
    _cc_thread_start(thread_logger, "logger", NULL);
    _log_init();

    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }

    log_running = false;
    _cc_sleep(100);

    return 0;
}