/*
 * test_hal_invariants.c
 * Standalone C11 Regression & Property-Based Fuzz Harness for STM32 HAL
 * Compiled with -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined
 */

#include "stm32_hal_invariants.h"
#include <assert.h>
#include <stdio.h>

static void test_null_pointer_and_zero_size_traps(void)
{
    hal_fuzz_telemetry_t tel;
    FLASH_ControllerState flash;
    hal_mock_reset(&tel, &flash);

    uint8_t buf[16] = {0};
    UART_HandleTypeDef huart1 = {921600U, false, true, 0U, 0U};

    assert(hal_gpio_write_pin(&tel, NULL, 1U, GPIO_PIN_SET) == HAL_ERROR);
    assert(tel.rejected_null_handle == 1ULL);

    assert(hal_uart_transmit(&tel, &huart1, NULL, 8U, 100U) == HAL_ERROR);
    assert(tel.rejected_null_buf == 1ULL);

    assert(hal_uart_transmit_dma(&tel, &huart1, buf, 0U) == HAL_ERROR);
    assert(tel.rejected_zero_size == 1ULL);

    assert(hal_uart_transmit(&tel, &huart1, buf, 8U, 100U) == HAL_OK);
    assert(huart1.tx_count == 8U);
    printf("  [PASS] test_null_pointer_and_zero_size_traps\n");
}

static void test_rcc_and_peripheral_bounds(void)
{
    hal_fuzz_telemetry_t tel;
    FLASH_ControllerState flash;
    hal_mock_reset(&tel, &flash);

    /* Zero PLLM division trap */
    assert(hal_rcc_clock_config(&tel, 336U, 0U) == HAL_ERROR);
    assert(tel.last_err == HAL_INV_DIV_ZERO_CLOCK);

    /* Valid 168 MHz STM32F4 PLL config */
    assert(hal_rcc_clock_config(&tel, 336U, 8U) == HAL_OK);

    /* Invalid TIM PWM channel mask & out-of-range 12-bit DAC code */
    TIM_HandleTypeDef htim1 = {167U, 999U, 0U, false};
    assert(hal_tim_pwm_start(&tel, &htim1, 0x05U) == HAL_ERROR);
    assert(hal_tim_pwm_start(&tel, &htim1, 0x04U) == HAL_OK);

    DAC_HandleTypeDef hdac1 = {0U, false};
    assert(hal_dac_set_value(&tel, &hdac1, 0U, 0U, 5000U) == HAL_ERROR);
    assert(hal_dac_set_value(&tel, &hdac1, 0U, 0U, 2048U) == HAL_OK);
    printf("  [PASS] test_rcc_and_peripheral_bounds\n");
}

static void test_deterministic_fuzz_corpus_dispatch(void)
{
    hal_fuzz_telemetry_t tel;
    FLASH_ControllerState flash;
    hal_mock_reset(&tel, &flash);

    /* Generate 256 deterministic pseudo-random fuzz mutations */
    uint32_t seed = 0xDEADBEEFU;
    for (int iter = 0; iter < 256; iter++) {
        uint8_t vec[8];
        for (int b = 0; b < 8; b++) {
            seed = seed * 1664525U + 1013904223U;
            vec[b] = (uint8_t)(seed >> 24);
        }
        stm32_hal_fuzz_dispatch(vec, sizeof(vec), &tel);
    }

    assert(tel.total_calls >= 256ULL);
    printf("  [PASS] test_deterministic_fuzz_corpus_dispatch (calls=%llu, null_caught=%llu, bounds_caught=%llu)\n",
           (unsigned long long)tel.total_calls,
           (unsigned long long)tel.rejected_null_handle,
           (unsigned long long)tel.rejected_bounds);
}

int main(void)
{
    printf("Running STM32 HAL C11 Invariant & Fuzz Test Suite...\n");
    test_null_pointer_and_zero_size_traps();
    test_rcc_and_peripheral_bounds();
    test_deterministic_fuzz_corpus_dispatch();
    printf("ALL STM32 HAL C11 TESTS PASSED.\n");
    return 0;
}
