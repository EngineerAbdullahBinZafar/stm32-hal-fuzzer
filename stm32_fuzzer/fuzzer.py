import random
import os

class STM32HALFuzzer:
    def __init__(self):
        self.functions = {
            "HAL_GPIO_WritePin": ["GPIO_TypeDef*", "uint16_t", "GPIO_PinState"],
            "HAL_UART_Transmit": ["UART_HandleTypeDef*", "uint8_t*", "uint16_t", "uint32_t"],
            "HAL_I2C_Master_Transmit": ["I2C_HandleTypeDef*", "uint16_t", "uint8_t*", "uint16_t", "uint32_t"],
            "HAL_TIM_Base_Start": ["TIM_HandleTypeDef*"]
        }
        
        self.edge_cases = {
            "uint16_t": ["0", "0xFFFF", "random.randint(0, 65535)"],
            "uint32_t": ["0", "0xFFFFFFFF", "random.randint(0, 4294967295)"],
            "GPIO_PinState": ["GPIO_PIN_RESET", "GPIO_PIN_SET", "2"], # 2 is invalid
            "uint8_t*": ["NULL", "(uint8_t*)\"test\"", "(uint8_t*)0x20000000"]
        }

    def generate_test_case(self, func_name):
        if func_name not in self.functions:
            return f"// Function {func_name} not supported"
        
        args = self.functions[func_name]
        generated_args = []
        
        for arg_type in args:
            if arg_type.endswith("*"):
                generated_args.append("NULL" if random.random() < 0.3 else f"&h{func_name.split('_')[1].lower()}")
            elif arg_type in self.edge_cases:
                choice = random.choice(self.edge_cases[arg_type])
                if "random" in choice:
                    generated_args.append(str(eval(choice)))
                else:
                    generated_args.append(choice)
            else:
                generated_args.append("0")
        
        return f"{func_name}({', '.join(generated_args)});"

    def generate_suite(self, count=10):
        suite = []
        for _ in range(count):
            func = random.choice(list(self.functions.keys()))
            suite.append(self.generate_test_case(func))
        return "\n".join(suite)

if __name__ == "__main__":
    fuzzer = STM32HALFuzzer()
    print(fuzzer.generate_suite(5))
