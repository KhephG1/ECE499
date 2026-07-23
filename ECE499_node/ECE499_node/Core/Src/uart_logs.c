#include "uart_logs.h"
#include <stdarg.h>
#include <stdio.h>

#define MAX_LOG_MSG (256)
#define LOG_LEVEL_NONE

#ifdef LOG_LEVEL_NONE
UARTLogLevel_t log_level = UART_LOG_LEVEL_NONE;
#endif

#ifdef LOG_LEVEL_INFO
UARTLogLevel_t log_level = UART_LOG_LEVEL_INFO;
#endif

#ifdef LOG_LEVEL_DEBUG
UARTLogLevel_t log_level = UART_LOG_LEVEL_DEBUG;
#endif

#ifdef LOG_LEVEL_ERROR
UARTLogLevel_t log_level = UART_LOG_LEVEL_ERROR; // Fixed typo
#endif

void reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

int intToStr(int x, char str[], int d) {
    int i = 0;
    if (x == 0) {
        str[i++] = '0';
    } else {
        while (x) {
            str[i++] = (x % 10) + '0';
            x = x / 10;
        }
    }
    while (i < d) {
        str[i++] = '0';
    }
 
    reverse(str, i);
    str[i] = '\0';
    return i;
}

void my_ftoa(float n, char* res, int afterpoint) {
    int i = 0;
    if (n < 0) {
        res[i++] = '-';
        n = -n;
    }
    if (afterpoint > 0) {
        float round_offset = 0.5f;
        for (int j = 0; j < afterpoint; j++) {
            round_offset /= 10.0f;
        }
        n += round_offset;
    } else {
        n += 0.5f; 
    }
    int ipart = (int)n;
    float fpart = n - (float)ipart;
    i += intToStr(ipart, res + i, 0);
    if (afterpoint > 0) {
        res[i++] = '.'; 
        
        for (int j = 0; j < afterpoint; j++) {
            fpart *= 10.0f;
            int digit = (int)fpart;
            if (digit > 9) digit = 9;
            if (digit < 0) digit = 0;

            res[i++] = digit + '0';
            fpart -= digit;
        }
    }
    res[i] = '\0';
}
void vlog_debug(const char* message, va_list args) {
    if (log_level >= UART_LOG_LEVEL_DEBUG) { 
        char buf[MAX_LOG_MSG] = {0}; 
        
        int bytes_written = vsnprintf(buf, MAX_LOG_MSG, message, args);
        
        if (bytes_written >= MAX_LOG_MSG) {
            bytes_written = MAX_LOG_MSG - 1;
        }
        //HAL_UART_Transmit(&hlpuart1, (uint8_t*)buf, bytes_written, HAL_MAX_DELAY);
    }
}
void log_debug(const char* message, ...) {
    if(log_level >= UART_LOG_LEVEL_DEBUG){ // Fixed logic checking hierarchy
        char buf[MAX_LOG_MSG] = {0}; 
        va_list args;
        va_start(args, message);
        int bytes_written = vsnprintf(buf, MAX_LOG_MSG, message, args);
        va_end(args);
        
        // Fixed: Protect against string truncation buffer overruns
        if (bytes_written >= MAX_LOG_MSG) bytes_written = MAX_LOG_MSG - 1;
        //HAL_UART_Transmit(&hlpuart1, (uint8_t*)buf, bytes_written, HAL_MAX_DELAY);
    }
}

void log_info(const char* message, ...) {
    if(log_level >= UART_LOG_LEVEL_INFO){ 
        char buf[MAX_LOG_MSG] = {0}; // Added structural initialisation
        va_list args;
        va_start(args, message);
        int bytes_written = vsnprintf(buf, MAX_LOG_MSG, message, args);
        va_end(args);
        
        if (bytes_written >= MAX_LOG_MSG) bytes_written = MAX_LOG_MSG - 1;
        //HAL_UART_Transmit(&hlpuart1, (uint8_t*)buf, bytes_written, HAL_MAX_DELAY);
    }
}

void log_error(const char* message, ...) {
    if(log_level >= UART_LOG_LEVEL_ERROR){ 
        char buf[MAX_LOG_MSG] = {0}; // Added structural initialisation
        va_list args;
        va_start(args, message);
        int bytes_written = vsnprintf(buf, MAX_LOG_MSG, message, args);
        va_end(args);
        
        if (bytes_written >= MAX_LOG_MSG) bytes_written = MAX_LOG_MSG - 1;
        //HAL_UART_Transmit(&hlpuart1, (uint8_t*)buf, bytes_written, HAL_MAX_DELAY);
    }
}
