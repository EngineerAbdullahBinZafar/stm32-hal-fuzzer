/*
 * stm32_hal_invariants.h
 * Formal Hardware Abstraction Layer (HAL) Contract & Invariant Checker
 * Covers 28 STM32CubeF4/G4/H7 HAL Peripheral APIs under Fault & Fuzz Inputs
 *
 * Author: Engineer Abdullah Bin Zafar
 * License: MIT
 */

#ifndef STM32_HAL_INVARIANTS_H
#define STM32_HAL_INVARIANTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef enum {
    HAL_INV_PASS = 0,
    HAL_INV_NULL_HANDLE,
    HAL_INV_NULL_BUFFER,
    HAL_INV_ZERO_SIZE,
    HAL_INV_SIZE_OVERFLOW,
    HAL_INV_INVALID_PIN_MASK,
    HAL_INV_INVALID_CHANNEL,
    HAL_INV_INVALID_I2C_ADDR,
    HAL_INV_FLASH_LOCKED,
    HAL_INV_DIV_ZERO_CLOCK
} hal_invariant_err_t;

typedef enum {
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET   = 1U
} GPIO_PinState;

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t ODR;
    volatile uint32_t IDR;
    uint32_t          pin_mask_valid;
} GPIO_TypeDef;

typedef struct {
    uint32_t baud_rate;
    bool     busy;
    bool     dma_enabled;
    uint32_t tx_count;
    uint32_t rx_count;
} UART_HandleTypeDef;

typedef struct {
    uint32_t clock_speed;
    bool     busy;
} I2C_HandleTypeDef;

typedef struct {
    uint32_t baud_prescaler;
    bool     busy;
} SPI_HandleTypeDef;

typedef struct {
    uint32_t prescaler;
    uint32_t period;
    uint32_t active_channels_mask;
    bool     running;
} TIM_HandleTypeDef;

typedef struct {
    uint32_t resolution_bits;
    uint32_t last_sample;
    bool     started;
} ADC_HandleTypeDef;

typedef struct {
    uint32_t last_code;
    bool     started;
} DAC_HandleTypeDef;

typedef struct {
    bool locked;
    uint32_t error_flags;
} FLASH_ControllerState;

typedef struct {
    uint64_t total_calls;
    uint64_t rejected_null_handle;
    uint64_t rejected_null_buf;
    uint64_t rejected_zero_size;
    uint64_t rejected_bounds;
    hal_invariant_err_t last_err;
} hal_fuzz_telemetry_t;

void hal_mock_reset(hal_fuzz_telemetry_t *tel, FLASH_ControllerState *flash);

/* Audited HAL API Equivalents */
HAL_StatusTypeDef hal_gpio_write_pin(hal_fuzz_telemetry_t *tel, GPIO_TypeDef *gpiox, uint16_t pin, GPIO_PinState state);
GPIO_PinState     hal_gpio_read_pin(hal_fuzz_telemetry_t *tel, const GPIO_TypeDef *gpiox, uint16_t pin);
HAL_StatusTypeDef hal_gpio_toggle_pin(hal_fuzz_telemetry_t *tel, GPIO_TypeDef *gpiox, uint16_t pin);

HAL_StatusTypeDef hal_uart_transmit(hal_fuzz_telemetry_t *tel, UART_HandleTypeDef *huart, const uint8_t *pdata, uint16_t size, uint32_t timeout);
HAL_StatusTypeDef hal_uart_receive(hal_fuzz_telemetry_t *tel, UART_HandleTypeDef *huart, uint8_t *pdata, uint16_t size, uint32_t timeout);
HAL_StatusTypeDef hal_uart_transmit_dma(hal_fuzz_telemetry_t *tel, UART_HandleTypeDef *huart, const uint8_t *pdata, uint16_t size);

HAL_StatusTypeDef hal_i2c_master_transmit(hal_fuzz_telemetry_t *tel, I2C_HandleTypeDef *hi2c, uint16_t dev_addr, const uint8_t *pdata, uint16_t size, uint32_t timeout);
HAL_StatusTypeDef hal_i2c_mem_write(hal_fuzz_telemetry_t *tel, I2C_HandleTypeDef *hi2c, uint16_t dev_addr, uint16_t mem_addr, uint16_t mem_add_size, const uint8_t *pdata, uint16_t size, uint32_t timeout);

HAL_StatusTypeDef hal_spi_transmit_receive(hal_fuzz_telemetry_t *tel, SPI_HandleTypeDef *hspi, const uint8_t *ptx, uint8_t *prx, uint16_t size, uint32_t timeout);

HAL_StatusTypeDef hal_tim_pwm_start(hal_fuzz_telemetry_t *tel, TIM_HandleTypeDef *htim, uint32_t channel);
HAL_StatusTypeDef hal_tim_pwm_stop(hal_fuzz_telemetry_t *tel, TIM_HandleTypeDef *htim, uint32_t channel);

HAL_StatusTypeDef hal_adc_start(hal_fuzz_telemetry_t *tel, ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef hal_dac_set_value(hal_fuzz_telemetry_t *tel, DAC_HandleTypeDef *hdac, uint32_t channel, uint32_t alignment, uint32_t data);

HAL_StatusTypeDef hal_flash_unlock(hal_fuzz_telemetry_t *tel, FLASH_ControllerState *flash);
HAL_StatusTypeDef hal_flash_lock(hal_fuzz_telemetry_t *tel, FLASH_ControllerState *flash);
HAL_StatusTypeDef hal_rcc_clock_config(hal_fuzz_telemetry_t *tel, uint32_t plln, uint32_t pllm);

/**
 * @brief Dispatch raw byte-stream fuzz vector across 28 STM32 HAL preconditions.
 */
void stm32_hal_fuzz_dispatch(const uint8_t *data, size_t len, hal_fuzz_telemetry_t *tel);

#ifdef __cplusplus
}
#endif

#endif /* STM32_HAL_INVARIANTS_H */
