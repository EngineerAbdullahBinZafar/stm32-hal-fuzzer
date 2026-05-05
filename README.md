# ⚡ STM32 HAL Fuzzer: Automated Edge-Case Tester

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Target: STM32](https://img.shields.io/badge/Target-STM32--HAL-blue.svg)](https://www.st.com/en/embedded-software/stm32cube-mcu-mpu-packages.html)
[![Security: Fuzzing](https://img.shields.io/badge/Security-Fuzzing-red.svg)](#)

**`stm32-hal-fuzzer`** is a Python-based tool designed to automatically generate boundary-condition test cases for the STM32 Hardware Abstraction Layer (HAL). It helps embedded developers identify crashes, hangs, and undefined behavior caused by invalid inputs (NULL pointers, buffer overflows, or out-of-range parameters).

---

## 🔍 Why Fuzz the HAL?

The STM32 HAL is the foundation of thousands of safety-critical and IoT devices. However, many HAL functions do not perform exhaustive input validation. Passing a `NULL` pointer or an out-of-range `uint16_t` to a UART or I2C function can lead to:
- 🛑 **HardFault** exceptions
- 🔄 **Infinite loops** in status polling
- 🔒 **Security vulnerabilities** in edge devices

---

## 🚀 How It Works

The tool generates C code snippets that call standard HAL functions with randomized "toxic" inputs, such as:
- `0x0000` and `0xFFFF` for 16-bit integers.
- `NULL` pointers for handle structures.
- Invalid enumeration states (e.g., passing `2` to a binary `GPIO_PinState`).

---

## 🛠️ Usage

### 1. Installation
```bash
git clone https://github.com/EngineerAbdullahBinZafar/stm32-hal-fuzzer.git
cd stm32-hal-fuzzer
pip install .
```

### 2. Generate a Test Suite
```python
from stm32_fuzzer import STM32HALFuzzer

fuzzer = STM32HALFuzzer()

# Generate 20 random edge-case HAL calls
test_suite = fuzzer.generate_suite(count=20)

with open("fuzz_test.c", "w") as f:
    f.write(test_suite)
```

### 3. Integration
Copy the generated `fuzz_test.c` into your STM32CubeIDE project and call the generated functions within a `while(1)` loop or a dedicated test task.

---

## 🤝 Contributing

We want to support more HAL modules (ADC, CAN, Ethernet, etc.)! If you'd like to contribute:
1.  Fork the repo.
2.  Add a new module to the `self.functions` dictionary in `fuzzer.py`.
3.  Submit a Pull Request.

---

## 📄 License
Distributed under the MIT License. See `LICENSE` for more information.

## 👤 Author
**Engineer Abdullah Bin Zafar**
- GitHub: [@EngineerAbdullahBinZafar](https://github.com/EngineerAbdullahBinZafar)
- LinkedIn: [Abdullah Bin Zafar](https://www.linkedin.com/in/abdullah-bin-zafar/)
