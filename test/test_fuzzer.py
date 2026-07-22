from stm32_fuzzer.fuzzer import STM32HALFuzzer

def test_fuzzer_initialization():
    fuzzer = STM32HALFuzzer(seed=42)
    assert len(fuzzer.functions) >= 25

def test_single_test_case_generation():
    fuzzer = STM32HALFuzzer(seed=100)
    code_line = fuzzer.generate_test_case("HAL_GPIO_WritePin")
    assert "HAL_GPIO_WritePin" in code_line
    assert code_line.endswith(";")

def test_c_harness_export():
    fuzzer = STM32HALFuzzer(seed=7)
    c_harness = fuzzer.generate_c_harness(test_count=5)
    assert "#include <stdio.h>" in c_harness
    assert "int main(void)" in c_harness
    assert "HAL_GPIO_WritePin" in c_harness or "HAL_UART_Transmit" in c_harness or "HAL_SPI_Transmit" in c_harness
