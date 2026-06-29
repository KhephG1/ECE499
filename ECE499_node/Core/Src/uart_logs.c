#include "uart_logs.h"
#include <stdarg.h>
#include <stdio.h>
#include "usart.h"
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
UARTUARTLogLevel_t log_level = UATY_LOG_LEVEL_ERROR;
#endif
void log_debug(const char* message, ...) {
    if(log_level == UART_LOG_LEVEL_DEBUG){ 
        char buf[MAX_LOG_MSG]; 
        va_list args;
        va_start(args, message);
        int bytes_written = vsnprintf(buf, MAX_LOG_MSG, message, args);
        va_end(args);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, bytes_written,HAL_MAX_DELAY);
    }
}

void log_info(const char* message, ...) {
    if(log_level >= UART_LOG_LEVEL_INFO){ 
        char buf[MAX_LOG_MSG]; 
        va_list args;
        va_start(args, message);
        int bytes_written = vsnprintf(buf, MAX_LOG_MSG, message, args);
        va_end(args);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, bytes_written,HAL_MAX_DELAY);
    }
}

void log_error(const char* message, ...) {
    if(log_level >= UART_LOG_LEVEL_ERROR){ 
        char buf[MAX_LOG_MSG]; 
        va_list args;
        va_start(args, message);
        int bytes_written = vsnprintf(buf, MAX_LOG_MSG, message, args);
        va_end(args);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, bytes_written,HAL_MAX_DELAY);
    }
}


