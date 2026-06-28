#ifndef UART_LOGS_H
#define UART_LOGS_H
#include "main.h"
typedef enum {
    UART_LOG_LEVEL_NONE = 0,
    UART_LOG_LEVEL_ERROR,
    UART_LOG_LEVEL_INFO,
    UART_LOG_LEVEL_DEBUG
} UARTLogLevel_t;
void log_debug(const char* message, ...);
void log_info(const char* message, ...);
void log_error(const char* message, ...);
#endif