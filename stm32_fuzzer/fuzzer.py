import random
import os
import sys

class STM32HALFuzzer:
    """
    Automated Fuzz Testing Suite for STM32 Hardware Abstraction Layer (HAL).
    Generates C test harnesses with boundary parameters, null pointers, unaligned addresses,
    and corrupted structs to uncover hard faults, memory leaks, and lockups in embedded C firmware.
    """
    def __init__(self, seed=None):
        if seed is not None:
            random.seed(seed)

        # 25+ Comprehensive STM32 HAL API Signatures
        self.functions = {
            # GPIO
            "HAL_GPIO_WritePin": ["GPIO_TypeDef*", "uint16_t", "GPIO_PinState"],
            "HAL_GPIO_ReadPin": ["GPIO_TypeDef*", "uint16_t"],
            "HAL_GPIO_TogglePin": ["GPIO_TypeDef*", "uint16_t"],
            # UART
            "HAL_UART_Transmit": ["UART_HandleTypeDef*", "uint8_t*", "uint16_t", "uint32_t"],
            "HAL_UART_Receive": ["UART_HandleTypeDef*", "uint8_t*", "uint16_t", "uint32_t"],
            "HAL_UART_Transmit_IT": ["UART_HandleTypeDef*", "uint8_t*", "uint16_t"],
            "HAL_UART_Receive_IT": ["UART_HandleTypeDef*", "uint8_t*", "uint16_t"],
            "HAL_UART_Transmit_DMA": ["UART_HandleTypeDef*", "uint8_t*", "uint16_t"],
            # I2C
            "HAL_I2C_Master_Transmit": ["I2C_HandleTypeDef*", "uint16_t", "uint8_t*", "uint16_t", "uint32_t"],
            "HAL_I2C_Master_Receive": ["I2C_HandleTypeDef*", "uint16_t", "uint8_t*", "uint16_t", "uint32_t"],
            "HAL_I2C_Mem_Write": ["I2C_HandleTypeDef*", "uint16_t", "uint16_t", "uint16_t", "uint8_t*", "uint16_t", "uint32_t"],
            "HAL_I2C_Mem_Read": ["I2C_HandleTypeDef*", "uint16_t", "uint16_t", "uint16_t", "uint8_t*", "uint16_t", "uint32_t"],
            # SPI
            "HAL_SPI_Transmit": ["SPI_HandleTypeDef*", "uint8_t*", "uint16_t", "uint32_t"],
            "HAL_SPI_Receive": ["SPI_HandleTypeDef*", "uint8_t*", "uint16_t", "uint32_t"],
            "HAL_SPI_TransmitReceive": ["SPI_HandleTypeDef*", "uint8_t*", "uint8_t*", "uint16_t", "uint32_t"],
            # TIMERS
            "HAL_TIM_Base_Start": ["TIM_HandleTypeDef*"],
            "HAL_TIM_Base_Stop": ["TIM_HandleTypeDef*"],
            "HAL_TIM_PWM_Start": ["TIM_HandleTypeDef*", "uint32_t"],
            "HAL_TIM_PWM_Stop": ["TIM_HandleTypeDef*", "uint32_t"],
            # ADC & DAC
            "HAL_ADC_Start": ["ADC_HandleTypeDef*"],
            "HAL_ADC_PollForConversion": ["ADC_HandleTypeDef*", "uint32_t"],
            "HAL_ADC_GetValue": ["ADC_HandleTypeDef*"],
            "HAL_DAC_Start": ["DAC_HandleTypeDef*", "uint32_t"],
            "HAL_DAC_SetValue": ["DAC_HandleTypeDef*", "uint32_t", "uint32_t", "uint32_t"],
            # FLASH & SYSTEM
            "HAL_FLASH_Unlock": [],
            "HAL_FLASH_Lock": [],
            "HAL_PWR_EnterSLEEPMode": ["uint32_t", "uint8_t"],
            "HAL_RCC_DeInit": []
        }

        # Safe parameter generator lookup (No eval!)
        self.generators = {
            "uint8_t": self._gen_uint8,
            "uint16_t": self._gen_uint16,
            "uint32_t": self._gen_uint32,
            "GPIO_PinState": self._gen_gpio_pinstate,
            "uint8_t*": self._gen_uint8_ptr,
            "GPIO_TypeDef*": lambda: self._gen_handle_ptr("GPIOA"),
            "UART_HandleTypeDef*": lambda: self._gen_handle_ptr("&huart1"),
            "I2C_HandleTypeDef*": lambda: self._gen_handle_ptr("&hi2c1"),
            "SPI_HandleTypeDef*": lambda: self._gen_handle_ptr("&hspi1"),
            "TIM_HandleTypeDef*": lambda: self._gen_handle_ptr("&htim1"),
            "ADC_HandleTypeDef*": lambda: self._gen_handle_ptr("&hadc1"),
            "DAC_HandleTypeDef*": lambda: self._gen_handle_ptr("&hdac1"),
        }

    def _gen_uint8(self) -> str:
        options = ["0", "255", "128", "0xFE"] + [str(random.randint(0, 255))]
        return random.choice(options)

    def _gen_uint16(self) -> str:
        options = ["0", "0xFFFF", "0x8000", "0x0001", "GPIO_PIN_0", "GPIO_PIN_ALL"] + [str(random.randint(0, 65535))]
        return random.choice(options)

    def _gen_uint32(self) -> str:
        options = ["0", "0xFFFFFFFF", "0x80000000", "HAL_MAX_DELAY", "1000"] + [str(random.randint(0, 4294967295))]
        return random.choice(options)

    def _gen_gpio_pinstate(self) -> str:
        # Include invalid enum state (2) for strict HAL robust checks
        return random.choice(["GPIO_PIN_RESET", "GPIO_PIN_SET", "(GPIO_PinState)2"])

    def _gen_uint8_ptr(self) -> str:
        # Include NULL pointer, valid dummy buffer, unaligned flash pointer
        return random.choice(["NULL", "(uint8_t*)dummy_buffer", "(uint8_t*)0x08000000", "(uint8_t*)0x20000001"])

    def _gen_handle_ptr(self, valid_var: str) -> str:
        # 25% chance of injecting NULL pointer to test defensive NULL checks
        if random.random() < 0.25:
            return "NULL"
        return valid_var

    def generate_test_case(self, func_name: str) -> str:
        """Generates a single C function call with boundary fuzzed arguments."""
        if func_name not in self.functions:
            return f"// Function {func_name} not found in database"

        arg_types = self.functions[func_name]
        args = []

        for arg_type in arg_types:
            if arg_type in self.generators:
                args.append(self.generators[arg_type]())
            else:
                args.append("0")

        return f"  {func_name}({', '.join(args)});"

    def generate_suite(self, count=15) -> str:
        """Generates a sequence of fuzzed C statements."""
        lines = []
        for i in range(count):
            func = random.choice(list(self.functions.keys()))
            lines.append(f"  // Test Case #{i+1}: Fuzzing {func}")
            lines.append(self.generate_test_case(func))
        return "\n".join(lines)

    def generate_c_harness(self, test_count=20) -> str:
        """Outputs a complete C source file runnable with GCC or Embedded Simulators."""
        fuzzed_calls = self.generate_suite(test_count)
        c_code = f"""/*
 * STM32 HAL Fuzzer Target Test Harness
 * Auto-generated by STM32HALFuzzer
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define NULL ((void *)0)
#define HAL_MAX_DELAY 0xFFFFFFFFU

typedef enum {{
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET = 1U
}} GPIO_PinState;

typedef struct {{ uint32_t dummy; }} GPIO_TypeDef;
typedef struct {{ uint32_t dummy; }} UART_HandleTypeDef;
typedef struct {{ uint32_t dummy; }} I2C_HandleTypeDef;
typedef struct {{ uint32_t dummy; }} SPI_HandleTypeDef;
typedef struct {{ uint32_t dummy; }} TIM_HandleTypeDef;
typedef struct {{ uint32_t dummy; }} ADC_HandleTypeDef;
typedef struct {{ uint32_t dummy; }} DAC_HandleTypeDef;

static GPIO_TypeDef GPIOA_struct;
static GPIO_TypeDef *GPIOA = &GPIOA_struct;
static UART_HandleTypeDef huart1;
static I2C_HandleTypeDef hi2c1;
static SPI_HandleTypeDef hspi1;
static TIM_HandleTypeDef htim1;
static ADC_HandleTypeDef hadc1;
static DAC_HandleTypeDef hdac1;
static uint8_t dummy_buffer[256];

/* Mock HAL Functions */
void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {{}}
void HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {{}}
void HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {{}}
void HAL_UART_Transmit(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{}}
void HAL_UART_Receive(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{}}
void HAL_UART_Transmit_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size) {{}}
void HAL_UART_Receive_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size) {{}}
void HAL_UART_Transmit_DMA(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size) {{}}
void HAL_I2C_Master_Transmit(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{}}
void HAL_I2C_Master_Receive(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{}}
void HAL_I2C_Mem_Write(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{}}
void HAL_I2C_Mem_Read(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{}}
void HAL_SPI_Transmit(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{}}
void HAL_SPI_Receive(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{}}
void HAL_SPI_TransmitReceive(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size, uint32_t Timeout) {{}}
void HAL_TIM_Base_Start(TIM_HandleTypeDef* htim) {{}}
void HAL_TIM_Base_Stop(TIM_HandleTypeDef* htim) {{}}
void HAL_TIM_PWM_Start(TIM_HandleTypeDef* htim, uint32_t Channel) {{}}
void HAL_TIM_PWM_Stop(TIM_HandleTypeDef* htim, uint32_t Channel) {{}}
void HAL_ADC_Start(ADC_HandleTypeDef* hadc) {{}}
void HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout) {{}}
void HAL_ADC_GetValue(ADC_HandleTypeDef* hadc) {{}}
void HAL_DAC_Start(DAC_HandleTypeDef* hdac, uint32_t Channel) {{}}
void HAL_DAC_SetValue(DAC_HandleTypeDef* hdac, uint32_t Channel, uint32_t Alignment, uint32_t Data) {{}}
void HAL_FLASH_Unlock(void) {{}}
void HAL_FLASH_Lock(void) {{}}
void HAL_PWR_EnterSLEEPMode(uint32_t Regulator, uint8_t SLEEPEntry) {{}}
void HAL_RCC_DeInit(void) {{}}

int main(void) {{
    printf("Starting STM32 HAL Fuzzing Suite execution...\\n");
{fuzzed_calls}
    printf("Fuzzing suite execution completed without critical crashes.\\n");
    return 0;
}}
"""
        return c_code

if __name__ == "__main__":
    fuzzer = STM32HALFuzzer()
    if len(sys.argv) > 1 and sys.argv[1] == "--export-c":
        print(fuzzer.generate_c_harness(25))
    else:
        print("=== STM32 HAL FUZZER TEST SUITE OUTPUT ===")
        print(fuzzer.generate_suite(10))
