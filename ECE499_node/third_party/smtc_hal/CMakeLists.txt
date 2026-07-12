# SPDX-License-Identifier: BSD-3-Clause-Clear

# NOTE: this project provides its own STM32U5 HAL build via
# cmake/stm32cubemx (targets `stm32cubemx` + `STM32_Drivers`), generated
# from the CubeMX .ioc file. smtc_hal reuses that instead of building a
# second copy of the HAL/CMSIS/ThreadX sources from a `../mcu_drivers`
# tree, which doesn't exist in this repo (this project doesn't use ThreadX).

add_library(smtc_hal STATIC
    smtc_hal_flash.c
    smtc_hal_gpio.c
    smtc_hal_lp_timer.c
    smtc_hal_libc_stub.c
    smtc_hal_mcu.c
    smtc_hal_rng.c
    smtc_hal_rtc.c
    smtc_hal_spi.c
    smtc_hal_trace.c
    smtc_hal_uart.c
    smtc_hal_watchdog.c
)

target_include_directories(smtc_hal PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(smtc_hal PUBLIC stm32cubemx STM32_Drivers)

# RADIO_FAMILY (e.g. "sx126x") is set by third_party/lbm/lbm_lib's
# CMakeLists.txt; smtc_hal_mcu.c uses the uppercased form (e.g. SX126X) to
# select the right radio-specific GPIO setup (antenna/RF-switch control pin).
string(TOUPPER ${RADIO_FAMILY} RADIO_FAMILY_UPPER)
target_compile_definitions(smtc_hal PUBLIC ${RADIO_FAMILY_UPPER})
