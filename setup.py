from setuptools import setup, find_packages

setup(
    name="stm32-hal-fuzzer",
    version="0.1.0",
    packages=find_packages(),
    install_requires=[],
    author="Engineer Abdullah Bin Zafar",
    description="Automated edge-case tester for STM32 HAL functions.",
    license="MIT",
    keywords="stm32, hal, fuzzing, embedded, security",
    url="https://github.com/EngineerAbdullahBinZafar/stm32-hal-fuzzer",
)
