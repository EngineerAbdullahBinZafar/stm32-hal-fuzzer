/*
 * stm32_hal_invariants.c
 * Contract-Validated Mock STM32 HAL Implementation for Property-Based Fuzzing
 * Enforces hardware safety invariants before touching peripheral registers.
 *
 * Author: Engineer Abdullah Bin Zafar
 * License: MIT
 */

#include "stm32_hal_invariants.h"
#include <string.h>

#define HAL_MAX_TRANSFER_BYTES 4096U
#define TIM_VALID_CHANNEL_MASK 0x0FU

static HAL_StatusTypeDef record_err(hal_fuzz_telemetry_t *tel, hal_invariant_err_t err)
{
    if (tel != NULL) {
        tel->last_err = err;
        switch (err) {
            case HAL_INV_NULL_HANDLE:
                tel->rejected_null_handle++;
                break;
            case HAL_INV_NULL_BUFFER:
                tel->rejected_null_buf++;
                break;
            case HAL_INV_ZERO_SIZE:
                tel->rejected_zero_size++;
                break;
            case HAL_INV_PASS:
                break;
            default:
                tel->rejected_bounds++;
                break;
        }
    }
    return (err == HAL_INV_PASS) ? HAL_OK : HAL_ERROR;
}

void hal_mock_reset(hal_fuzz_telemetry_t *tel, FLASH_ControllerState *flash)
{
    if (tel != NULL) {
        memset(tel, 0, sizeof(*tel));
    }
    if (flash != NULL) {
        flash->locked = true;
        flash->error_flags = 0U;
    }
}

HAL_StatusTypeDef hal_gpio_write_pin(hal_fuzz_telemetry_t *tel, GPIO_TypeDef *gpiox, uint16_t pin, GPIO_PinState state)
{
    if (tel != NULL) tel->total_calls++;
    if (gpiox == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (pin == 0U || (pin & (pin - 1U)) != 0U) return record_err(tel, HAL_INV_INVALID_PIN_MASK);

    if (state == GPIO_PIN_SET) {
        gpiox->ODR |= pin;
    } else {
        gpiox->ODR &= (uint32_t)(~((uint32_t)pin));
    }
    return record_err(tel, HAL_INV_PASS);
}

GPIO_PinState hal_gpio_read_pin(hal_fuzz_telemetry_t *tel, const GPIO_TypeDef *gpiox, uint16_t pin)
{
    if (tel != NULL) tel->total_calls++;
    if (gpiox == NULL) {
        record_err(tel, HAL_INV_NULL_HANDLE);
        return GPIO_PIN_RESET;
    }
    if (pin == 0U || (pin & (pin - 1U)) != 0U) {
        record_err(tel, HAL_INV_INVALID_PIN_MASK);
        return GPIO_PIN_RESET;
    }
    record_err(tel, HAL_INV_PASS);
    return ((gpiox->IDR & pin) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

HAL_StatusTypeDef hal_gpio_toggle_pin(hal_fuzz_telemetry_t *tel, GPIO_TypeDef *gpiox, uint16_t pin)
{
    if (tel != NULL) tel->total_calls++;
    if (gpiox == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (pin == 0U) return record_err(tel, HAL_INV_INVALID_PIN_MASK);
    gpiox->ODR ^= pin;
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_uart_transmit(hal_fuzz_telemetry_t *tel, UART_HandleTypeDef *huart, const uint8_t *pdata, uint16_t size, uint32_t timeout)
{
    (void)timeout;
    if (tel != NULL) tel->total_calls++;
    if (huart == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (pdata == NULL) return record_err(tel, HAL_INV_NULL_BUFFER);
    if (size == 0U) return record_err(tel, HAL_INV_ZERO_SIZE);
    if (size > HAL_MAX_TRANSFER_BYTES) return record_err(tel, HAL_INV_SIZE_OVERFLOW);
    if (huart->busy) return HAL_BUSY;

    huart->tx_count += size;
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_uart_receive(hal_fuzz_telemetry_t *tel, UART_HandleTypeDef *huart, uint8_t *pdata, uint16_t size, uint32_t timeout)
{
    (void)timeout;
    if (tel != NULL) tel->total_calls++;
    if (huart == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (pdata == NULL) return record_err(tel, HAL_INV_NULL_BUFFER);
    if (size == 0U) return record_err(tel, HAL_INV_ZERO_SIZE);
    if (size > HAL_MAX_TRANSFER_BYTES) return record_err(tel, HAL_INV_SIZE_OVERFLOW);
    if (huart->busy) return HAL_BUSY;

    huart->rx_count += size;
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_uart_transmit_dma(hal_fuzz_telemetry_t *tel, UART_HandleTypeDef *huart, const uint8_t *pdata, uint16_t size)
{
    if (tel != NULL) tel->total_calls++;
    if (huart == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (!huart->dma_enabled) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (pdata == NULL) return record_err(tel, HAL_INV_NULL_BUFFER);
    if (size == 0U) return record_err(tel, HAL_INV_ZERO_SIZE);
    if (size > HAL_MAX_TRANSFER_BYTES) return record_err(tel, HAL_INV_SIZE_OVERFLOW);

    huart->tx_count += size;
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_i2c_master_transmit(hal_fuzz_telemetry_t *tel, I2C_HandleTypeDef *hi2c, uint16_t dev_addr, const uint8_t *pdata, uint16_t size, uint32_t timeout)
{
    (void)timeout;
    if (tel != NULL) tel->total_calls++;
    if (hi2c == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (pdata == NULL) return record_err(tel, HAL_INV_NULL_BUFFER);
    if (size == 0U) return record_err(tel, HAL_INV_ZERO_SIZE);
    if (dev_addr > 0x3FFU) return record_err(tel, HAL_INV_INVALID_I2C_ADDR);
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_i2c_mem_write(hal_fuzz_telemetry_t *tel, I2C_HandleTypeDef *hi2c, uint16_t dev_addr, uint16_t mem_addr, uint16_t mem_add_size, const uint8_t *pdata, uint16_t size, uint32_t timeout)
{
    (void)mem_addr;
    (void)timeout;
    if (tel != NULL) tel->total_calls++;
    if (hi2c == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (pdata == NULL) return record_err(tel, HAL_INV_NULL_BUFFER);
    if (size == 0U) return record_err(tel, HAL_INV_ZERO_SIZE);
    if (dev_addr > 0x3FFU || (mem_add_size != 1U && mem_add_size != 2U)) {
        return record_err(tel, HAL_INV_INVALID_I2C_ADDR);
    }
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_spi_transmit_receive(hal_fuzz_telemetry_t *tel, SPI_HandleTypeDef *hspi, const uint8_t *ptx, uint8_t *prx, uint16_t size, uint32_t timeout)
{
    (void)timeout;
    if (tel != NULL) tel->total_calls++;
    if (hspi == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (ptx == NULL || prx == NULL) return record_err(tel, HAL_INV_NULL_BUFFER);
    if (size == 0U) return record_err(tel, HAL_INV_ZERO_SIZE);
    if (size > HAL_MAX_TRANSFER_BYTES) return record_err(tel, HAL_INV_SIZE_OVERFLOW);
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_tim_pwm_start(hal_fuzz_telemetry_t *tel, TIM_HandleTypeDef *htim, uint32_t channel)
{
    if (tel != NULL) tel->total_calls++;
    if (htim == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (channel > TIM_VALID_CHANNEL_MASK || (channel & 0x03U) != 0U) {
        return record_err(tel, HAL_INV_INVALID_CHANNEL);
    }
    htim->running = true;
    htim->active_channels_mask |= (1U << (channel >> 2));
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_tim_pwm_stop(hal_fuzz_telemetry_t *tel, TIM_HandleTypeDef *htim, uint32_t channel)
{
    if (tel != NULL) tel->total_calls++;
    if (htim == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (channel > TIM_VALID_CHANNEL_MASK || (channel & 0x03U) != 0U) {
        return record_err(tel, HAL_INV_INVALID_CHANNEL);
    }
    htim->active_channels_mask &= (uint32_t)(~(1U << (channel >> 2)));
    if (htim->active_channels_mask == 0U) htim->running = false;
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_adc_start(hal_fuzz_telemetry_t *tel, ADC_HandleTypeDef *hadc)
{
    if (tel != NULL) tel->total_calls++;
    if (hadc == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (hadc->resolution_bits != 12U && hadc->resolution_bits != 10U && hadc->resolution_bits != 8U) {
        return record_err(tel, HAL_INV_INVALID_CHANNEL);
    }
    hadc->started = true;
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_dac_set_value(hal_fuzz_telemetry_t *tel, DAC_HandleTypeDef *hdac, uint32_t channel, uint32_t alignment, uint32_t data)
{
    (void)alignment;
    if (tel != NULL) tel->total_calls++;
    if (hdac == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    if (channel > 1U || data > 0x0FFFU) return record_err(tel, HAL_INV_INVALID_CHANNEL);
    hdac->last_code = data;
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_flash_unlock(hal_fuzz_telemetry_t *tel, FLASH_ControllerState *flash)
{
    if (tel != NULL) tel->total_calls++;
    if (flash == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    flash->locked = false;
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_flash_lock(hal_fuzz_telemetry_t *tel, FLASH_ControllerState *flash)
{
    if (tel != NULL) tel->total_calls++;
    if (flash == NULL) return record_err(tel, HAL_INV_NULL_HANDLE);
    flash->locked = true;
    return record_err(tel, HAL_INV_PASS);
}

HAL_StatusTypeDef hal_rcc_clock_config(hal_fuzz_telemetry_t *tel, uint32_t plln, uint32_t pllm)
{
    if (tel != NULL) tel->total_calls++;
    if (pllm == 0U || pllm > 63U || plln < 50U || plln > 432U) {
        return record_err(tel, HAL_INV_DIV_ZERO_CLOCK);
    }
    return record_err(tel, HAL_INV_PASS);
}

void stm32_hal_fuzz_dispatch(const uint8_t *data, size_t len, hal_fuzz_telemetry_t *tel)
{
    if (data == NULL || len < 6U || tel == NULL) {
        return;
    }

    GPIO_TypeDef gpioa = {0};
    UART_HandleTypeDef huart1 = {115200U, false, true, 0U, 0U};
    I2C_HandleTypeDef hi2c1 = {400000U, false};
    SPI_HandleTypeDef hspi1 = {16U, false};
    TIM_HandleTypeDef htim1 = {167U, 999U, 0U, false};
    ADC_HandleTypeDef hadc1 = {12U, 0U, false};
    DAC_HandleTypeDef hdac1 = {0U, false};

    uint8_t dummy_tx[128];
    uint8_t dummy_rx[128];
    memset(dummy_tx, 0xA5, sizeof(dummy_tx));
    memset(dummy_rx, 0x00, sizeof(dummy_rx));

    uint8_t op = (uint8_t)(data[0] % 8U);
    uint16_t arg16 = (uint16_t)(((uint16_t)data[1] << 8) | data[2]);
    uint32_t arg32 = ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5];

    const bool pass_null = (data[1] == 0x00U);
    const uint8_t *tx_ptr = (data[2] == 0xFFU) ? NULL : dummy_tx;

    switch (op) {
        case 0:
            hal_gpio_write_pin(tel, pass_null ? NULL : &gpioa, arg16, (data[3] & 1U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
            break;
        case 1:
            hal_uart_transmit(tel, pass_null ? NULL : &huart1, tx_ptr, (uint16_t)(arg16 % 256U), arg32);
            break;
        case 2:
            hal_uart_transmit_dma(tel, pass_null ? NULL : &huart1, tx_ptr, (uint16_t)(arg16 % 256U));
            break;
        case 3:
            hal_i2c_master_transmit(tel, pass_null ? NULL : &hi2c1, arg16, tx_ptr, (uint16_t)(data[4]), arg32);
            break;
        case 4:
            hal_spi_transmit_receive(tel, pass_null ? NULL : &hspi1, tx_ptr, dummy_rx, (uint16_t)(arg16 % 128U), arg32);
            break;
        case 5:
            hal_tim_pwm_start(tel, pass_null ? NULL : &htim1, arg32 & 0x3FU);
            break;
        case 6:
            hal_dac_set_value(tel, pass_null ? NULL : &hdac1, data[3] & 3U, 0U, arg16);
            hal_adc_start(tel, &hadc1);
            break;
        default:
            hal_rcc_clock_config(tel, arg16, data[4]);
            break;
    }
}
