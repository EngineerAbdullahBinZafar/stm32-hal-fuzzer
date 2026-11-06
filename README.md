<div align="center">

# ⚡ STM32 HAL Fuzzer
### **Automated Coverage-Guided Edge-Case Fuzzer & Stress Tester for STM32 HAL Firmware**
*AFL Edge Hashing · ARM Cortex-M NVIC Latency Bounds · 25+ HAL Peripheral Drivers · Automated C Harness Generation*

[![CI Build & Test](https://img.shields.io/badge/CI-Passing%20(3%2F3%20Tests)-00d4ff?style=for-the-badge&logo=githubactions&logoColor=white)](https://github.com/EngineerAbdullahBinZafar/stm32-hal-fuzzer)
[![MCU Target](https://img.shields.io/badge/MCU-STM32F4%20%7C%20F7%20%7C%20H7%20%7C%20G4-03234B?style=for-the-badge&logo=stmicroelectronics&logoColor=white)](https://st.com)
[![Fuzzing Engine](https://img.shields.io/badge/Fuzzing-AFL--Style%20Coverage-7b61ff?style=for-the-badge&logo=security&logoColor=white)](https://github.com/EngineerAbdullahBinZafar/stm32-hal-fuzzer)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge)](LICENSE)

---

```
"Your firmware works under nominal inputs. Does it survive adversarial boundary edge cases?"
```

```bash
# ⚡ 1-Command Live Terminal Demonstration (Zero Dependencies)
python demo.py
```

[Executive Overview](#-executive-overview) • [Mathematical Formulations](#-mathematical-formulations--coverage-mechanics) • [HAL Peripheral Catalog](#-hal-peripheral-driver-catalog) • [Mutation Engine](#-algebraic-mutation-engine) • [C Harness Generation](#%EF%B8%8F-automated-c-test-harness-export) • [Verification](#-verification--empirical-test-suite)

</div>

<br/>

## 🎯 Executive Overview

**STM32 HAL Fuzzer** is an automated, coverage-guided edge-case fuzzer and verification engine engineered to stress-test **STMicroelectronics STM32 Hardware Abstraction Layer (HAL)** peripheral firmware drivers.

Bare-metal and RTOS firmware in robotics and embedded systems often encounter catastrophic runtime failures not from logic bugs during nominal operation, but from unhandled hardware boundary states:
- Unaligned memory pointers (`0x20000001`) causing ARM Cortex-M `UsageFault` traps.
- Null pointer dereferencing on uninitialized peripheral handles (`huart1`, `hi2c1`, `hspi1`).
- Buffer length integer underflows and size boundary overflows (`Size = 0`, `Size = 0xFFFF`).
- Hardware bus deadlocks and infinite blocking timeouts (`HAL_MAX_DELAY`).

`stm32-hal-fuzzer` systematically exercises 25+ STM32 HAL peripheral APIs (GPIO, UART, I2C, SPI, Timers, ADC, DAC, Flash, Power, and RCC) by generating adversarial corpora and generating standalone runnable C test harnesses compatible with GCC, Clang, QEMU ARM emulation, and physical STM32 development boards.

---

## 📐 Mathematical Formulations & Coverage Mechanics

### 1. AFL-Style Directional Edge Coverage Hashing

Coverage-guided fuzzing tracks branch transitions between basic blocks $A$ and $B$. Assigning deterministic 16-bit pseudo-random identifiers $L_A, L_B \in [0, 2^{16}-1]$:

$$\text{cur\_location} = \text{Hash}(B)$$

$$\text{edge\_id} = (\text{cur\_location} \oplus \text{prev\_location}) \pmod K$$

$$\text{prev\_location} \leftarrow \text{cur\_location} \gg 1$$

where $K = 65{,}536 = 2^{16}$ represents the capacity of the shared coverage bitmap array.

The 1-bit right shift ($\gg 1$) preserves branch directionality, guaranteeing asymmetry:

$$(L_A \oplus (L_B \gg 1)) \ne (L_B \oplus (L_A \gg 1))$$

To prevent combinatorial path explosion from tight hardware polling loops, execution hit counts are quantized into 8 non-linear logarithmic buckets:

$$C_{\text{bucket}}(c) = \begin{cases}
1, & c = 1 \\
2, & c = 2 \\
3, & c = 3 \\
4, & 4 \le c \le 7 \\
8, & 8 \le c \le 15 \\
16, & 16 \le c \le 31 \\
32, & 32 \le c \le 127 \\
128, & c \ge 128
\end{cases}$$

### 2. ARM Cortex-M NVIC Interrupt Latency Bounds

For ARM Cortex-M3/M4/M7 microcontrollers executing real-time HAL drivers, total interrupt response latency $\Delta t_{\text{latency}}$ is bounded by:

$$\Delta t_{\text{latency}} \le t_{\text{stack}} + t_{\text{tail-chain}} + t_{\text{jitter}} + t_{\text{mem}}$$

1. **Hardware Context Stacking Cycles ($t_{\text{stack}}$):**
   The hardware automatically pushes 8 basic core registers onto the current stack pointer:
   $$R_0, R_1, R_2, R_3, R_{12}, LR, PC, xPSR$$
   $$t_{\text{stack}} = 12\,\text{clock cycles}$$
   When the Floating Point Unit (FPU / FPv4-SP) is active with lazy stacking enabled (`FPCCR.LSPEN = 1`, default), space for 18 floating-point registers ($S_0\text{--}S_{15}, FPSCR, \text{reserved}$) is reserved in stack memory without stalling the CPU:
   $$t_{\text{stack,lazy}} = 12\,\text{clock cycles}$$

2. **Tail-Chaining Cycles ($t_{\text{tail-chain}}$):**
   When an interrupt is pending upon exit from a preceding ISR, context restore and re-stacking are skipped:
   $$t_{\text{tail-chain}} = 6\,\text{clock cycles}$$

3. **Absolute Response Latency Formulation:**
   $$T_{\text{latency}} = \frac{N_{\text{cycles}}}{f_{\text{CPU}}}$$
   For an STM32F407 core operating at $f_{\text{CPU}} = 168\,\text{MHz}$ ($T_{\text{clk}} = \frac{1}{168 \times 10^6\,\text{s}^{-1}} \approx 5.952\,\text{ns}$):
   $$T_{\text{latency,nominal}} = \frac{12\,\text{cycles}}{168 \times 10^6\,\text{s}^{-1}} \approx 71.43\,\text{ns}$$
   Under worst-case Flash memory bus contention with $W_{\text{wait}} = 5$ wait states:
   $$T_{\text{latency,worst}} \le \frac{12 + 2 \times W_{\text{wait}}}{168 \times 10^6\,\text{s}^{-1}} = \frac{22}{168 \times 10^6\,\text{s}^{-1}} \approx 130.95\,\text{ns}$$

### 3. Formal Algebraic Mutation Operators

The fuzzer engine generates targeted input variations via formal algebraic mutation operators:
1. **Single-Bit Flip Operator ($\mathcal{M}_{\text{bit}}$):**
   $$\mathcal{M}_{\text{bit}}(x, k) = x \oplus 2^k, \quad k \in \{0, \dots, 31\}$$
2. **Boundary Value Substitution ($\mathcal{M}_{\text{int}}$):**
   $$x \leftarrow v^*, \quad v^* \in \{0, 1, 0\text{x}7F, 0\text{x}80, 0\text{x}FF, 0\text{xFFFF}, 0\text{x}7FFF, 0\text{x}8000, 0\text{xFFFFFFFF}\}$$
3. **Arithmetic Delta Injection ($\mathcal{M}_{\text{arith}}$):**
   $$\mathcal{M}_{\text{arith}}(x, \delta) = (x + \delta) \pmod{2^{32}}, \quad \delta \in \{-35, \dots, +35\} \setminus \{0\}$$
4. **Memory Address Alignment & Fault Injection ($\mathcal{M}_{\text{ptr}}$):**
   $$p \in \{\text{NULL}, \text{RAM\_UNALIGNED} = 0\text{x}20000001, \text{FLASH\_BASE} = 0\text{x}08000000, \text{PERIPH\_BASE} = 0\text{x}40000000\}$$
5. **Peripheral Reserved Bit-Field Masking ($\mathcal{M}_{\text{mask}}$):**
   $$R_{\text{target}} = (R_{\text{fuzzed}} \ \& \ \mathbf{M}_{\text{writable}}) \ | \ (\mathbf{M}_{\text{reserved\_state}})$$

---

## 🎛️ HAL Peripheral Driver Catalog

The fuzzer provides automated coverage harnesses across 25+ STM32 HAL peripheral APIs:

| Peripheral Group | APIs Covered | Transmission Modes | Stress Vectors Tested |
|:---|:---|:---:|:---|
| **GPIO** | `HAL_GPIO_WritePin`, `ReadPin`, `TogglePin` | Direct Register | Invalid Port pointers, Pin masks $> 0\text{xFFFF}$ |
| **UART** | `HAL_UART_Transmit`, `Receive`, `Transmit_IT`, `Receive_IT`, `Transmit_DMA` | Polling, Interrupt, DMA | Size $= 0$, Size $= 0\text{xFFFF}$, Unaligned RX buffer, `HAL_MAX_DELAY` timeout |
| **I2C** | `HAL_I2C_Master_Transmit`, `Master_Receive`, `Mem_Write`, `Mem_Read` | Master Polling / Memory | 7-bit vs 10-bit address shifts, MemAddressSize boundary, NACK recovery |
| **SPI** | `HAL_SPI_Transmit`, `Receive`, `TransmitReceive` | Full-Duplex Polling | 8-bit vs 16-bit frame formats, simultaneous RX/TX pointer overlap |
| **Timers (TIM)** | `HAL_TIM_Base_Start`, `Base_Stop`, `PWM_Start`, `PWM_Stop` | Hardware PWM | Uninitialized timer handle, invalid PWM channel ($> 4$), period overflow |
| **ADC / DAC** | `HAL_ADC_Start`, `PollForConversion`, `GetValue`, `HAL_DAC_Start`, `SetValue` | Analog Mixed-Signal | Over-sampling triggers, DAC alignment flags, zero-wait conversion timeout |
| **System / RCC** | `HAL_FLASH_Unlock`, `Lock`, `HAL_PWR_EnterSLEEPMode`, `HAL_RCC_DeInit` | System Control | Flash lock state transitions, low-power sleep modes, clock tree reset |

---

## 🛠️ Automated C Test Harness Export

Generate a standalone, self-contained C source test harness complete with boundary parameters, unaligned pointers, and mocked HAL headers:

```bash
# Export runnable C harness with 20 randomized edge cases
python -m stm32_fuzzer.fuzzer --export-c
```

### Python SDK Usage

```python
from stm32_fuzzer.fuzzer import STM32HALFuzzer

# 1. Initialize fuzzer with optional deterministic seed
fuzzer = STM32HALFuzzer(seed=42)

# 2. Generate a single adversarial test case for a target API
test_stmt = fuzzer.generate_test_case("HAL_UART_Transmit")
print(test_stmt)
# Output: HAL_UART_Transmit(&huart1, (uint8_t*)0x20000001, 0, 1000);

# 3. Generate a full suite across randomized HAL peripherals
suite = fuzzer.generate_suite(count=15)

# 4. Export complete standalone C file
c_code = fuzzer.generate_c_harness(test_count=25)
with open("test_harness.c", "w") as f:
    f.write(c_code)
```

---

## 🧪 Verification & Empirical Test Suite

Run the automated test suite verifying 25+ function signatures, single test case generation, and valid C harness generation:

```bash
uv run --with pytest pytest -o pythonpath=. test/ -v
```

```
============================= test session starts =============================
platform win32 -- Python 3.11.15, pytest-9.2.0, pluggy-1.7.0
rootdir: C:\Users\star\Downloads\freeapps\stm32-hal-fuzzer
collecting ... collected 3 items

test/test_fuzzer.py::test_fuzzer_initialization PASSED                   [ 33%]
test/test_fuzzer.py::test_single_test_case_generation PASSED             [ 66%]
test/test_fuzzer.py::test_c_harness_export PASSED                        [100%]

============================== 3 passed in 0.06s ==============================
```

---

<details>
<summary>📋 Machine-Readable Metadata (Schema.org JSON-LD)</summary>

```json
{
  "@context": "https://schema.org",
  "@type": "SoftwareSourceCode",
  "name": "stm32-hal-fuzzer",
  "version": "1.0.0",
  "description": "Automated coverage-guided edge-case fuzzer and stress-testing framework for STM32 Hardware Abstraction Layer (HAL) peripheral drivers with AFL edge hashing, NVIC interrupt latency models, and C test harness generation.",
  "author": {
    "@type": "Person",
    "name": "Engineer Abdullah Bin Zafar",
    "email": "abz.king.1.9.2003@gmail.com",
    "affiliation": {
      "@type": "EducationalOrganization",
      "name": "University of Engineering and Technology (UET) Lahore"
    },
    "sameAs": "https://github.com/EngineerAbdullahBinZafar"
  },
  "license": "https://opensource.org/licenses/MIT",
  "codeRepository": "https://github.com/EngineerAbdullahBinZafar/stm32-hal-fuzzer",
  "programmingLanguage": [
    "Python",
    "C"
  ],
  "applicationCategory": "EmbeddedSecurityFuzzing",
  "operatingSystem": "Cross-Platform (Linux, Windows, Bare-Metal Microcontrollers)",
  "keywords": [
    "stm32",
    "hal",
    "fuzzing",
    "embedded-security",
    "afl-coverage",
    "nvic-latency",
    "firmware-testing",
    "hard-fault-detection",
    "cortex-m4",
    "automated-testing"
  ]
}
```

</details>

---

## 👨‍💻 Author & Engineering Credits
**Engineer Abdullah Bin Zafar**  
*Mechatronics & Control Systems Engineer | UET Lahore*  
- **GitHub:** [@EngineerAbdullahBinZafar](https://github.com/EngineerAbdullahBinZafar)  
- **Email:** abz.king.1.9.2003@gmail.com  
- **Flagship R&D:** [Automotive Chassis CBF](https://github.com/EngineerAbdullahBinZafar/automotive-chassis-cbf) · [Principia Robotica](https://github.com/EngineerAbdullahBinZafar/principia-robotica)
