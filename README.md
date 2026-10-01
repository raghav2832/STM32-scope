# STM32 Portable Oscilloscope

A bare-metal oscilloscope built on the STM32F446RE microcontroller.
Captures analog signals at 100 KSPS using ADC + DMA, detects edges
with a software trigger engine, streams binary packets over UART, and
displays live waveforms, FFT spectrum, and measurements on a Python PC
application.

No HAL. No CubeMX generated drivers. Every peripheral driven at the
register level.

---

## Features

- 100 KSPS ADC acquisition via TIM2-triggered DMA circular buffer
- Software trigger engine — rising edge, falling edge, auto timeout
- Pre-trigger and post-trigger capture (64 + 128 samples)
- Binary serial protocol with CRC16-CCITT error detection
- Live waveform plot centred on trigger point
- FFT spectrum with harmonic peak annotation
- Real-time measurements — Vpp, Vmax, Vmin, Vavg, Vrms, frequency,
  period, duty cycle
- Self-contained test signal — no external signal generator required

---

## Hardware

| Component | Detail |
|---|---|
| MCU | STM32F446RE (Nucleo-F446RE) |
| Analog input | PA0 — ADC1 channel 0 |
| Test signal output | PA8 — TIM1 CH1 PWM |
| UART | PA2 TX / PA3 RX — USART2 (ST-Link virtual COM) |
| Connection | One jumper wire: PA8 → PA0 |

---

## System Architecture

```
[TIM1 PWM] ──wire──► [PA0 ADC]
                          │
                    TIM2 TRGO trigger
                          │
                     ADC converts
                          │
                    DMA circular buffer
                          │
                    Trigger engine
                          │
                    CRC16 binary packet
                          │
                    USART2 TX ──USB──► Python App
                                           │
                                    Packet parser
                                           │
                              ┌────────────┴────────────┐
                         Waveform plot             FFT spectrum
                              │
                         Measurements
                     Vpp / Vrms / Freq / Duty
```

---

## Firmware Architecture

```
stm32-scope/
├── BSP/                    board support — clock init at 180MHz
├── Drivers_custom/
│   ├── gpio/               MODER/OTYPER/PUPDR/AFR register driver
│   ├── uart/               USART2 bare-metal, printf redirect
│   ├── adc/                12-bit ADC, timer-triggered, DMA-linked
│   ├── dma/                DMA2 Stream0 circular, HTIF/TCIF ISR
│   └── timer/              TIM1 PWM, TIM2 TRGO timebase
├── Middleware/
│   ├── trigger/            edge detection, pre/post capture engine
│   └── protocol/           binary framer, CRC16 packet TX
├── App/                    top-level acquisition state machine
├── Util/                   CRC16, SysTick, ring buffer
└── PC/                     Python application
    ├── main.py
    ├── packet_parser.py
    ├── plotter.py
    └── fft_analysis.py
```

---

## Signal Flow

```
PA8 PWM (1kHz, 50% duty)
  │
  └──► PA0 (ADC input)
         │
         │  TIM2 fires TRGO every 10µs (100 KSPS)
         ▼
      ADC converts 0–3.3V → 0–4095 (12-bit)
         │
         │  DMA moves result to adc_buf[1024] automatically
         ▼
      Half-transfer ISR (512 samples ready)
      Full-transfer ISR (512 samples ready)
         │
         ▼
      Trigger engine scans each sample
      Detects threshold crossing
      Freezes 192-sample frame (64 pre + 128 post)
         │
         ▼
      Protocol framer packs into binary packet
      CRC16 computed and appended
      Sent over USART2 at 115200 baud
         │
         ▼
      Python receives, parses, verifies CRC
      Matplotlib plots waveform at 20 fps
      FFT computed with Hanning window
      Measurements displayed
```

---

## Packet Protocol

```
Byte 0-1   AA 55         sync
Byte 2     01            packet type (waveform)
Byte 3     seq           sequence number 0-255
Byte 4-5   length        payload length, little-endian
Byte 6-N   payload       metadata + uint16 samples
Byte N+1-2 CRC           CRC16-CCITT, little-endian
```

Waveform payload:

```
Byte 0-1   sample_rate   Hz, uint16
Byte 2-3   trigger_idx   index of trigger in frame
Byte 4     trigger_mode  0=rising 1=falling 2=auto
Byte 5-6   vref_mv       reference voltage in mV
Byte 7..   samples       uint16 LE, one per ADC result
```

---

## Clock Configuration

```
HSI 16MHz → PLL → SYSCLK 180MHz
AHB  /1  → 180MHz   (CPU, DMA, GPIO)
APB1 /4  →  45MHz   (TIM2, USART2)
APB2 /2  →  90MHz   (TIM1, ADC1)
TIM2 clock = APB1 × 2 = 90MHz
TIM1 clock = APB2 × 2 = 180MHz
```

---

## Sample Rate Derivation

```
TIM2 clock = 90MHz
PSC = 89  →  tick = 1MHz
ARR = 9   →  update = 1MHz / 10 = 100,000 Hz

ADC conversion time = (56 + 12.5) / 22.5MHz = 3.04µs
Sample period       = 10µs
Margin              = 6.96µs (229% headroom)
```

---

## Getting Started

### Requirements

**Hardware**
- Nucleo-F446RE
- One jumper wire (PA8 to PA0)

**Software**
- STM32CubeIDE
- Python 3.8+
- `pip install pyserial matplotlib numpy`

### Flash Firmware

```
1. Open STM32CubeIDE
2. File → Open Projects from File System → select stm32-scope/
3. Ctrl+B to build
4. F11 to flash and debug
5. F8 to run
```

### Run PC Application

```bash
cd PC
python main.py
```

Change the COM port in `main.py` to match your system:

```python
PORT = 'COM8'     # Windows
PORT = '/dev/ttyACM0'   # Linux/Mac
```

### Physical Setup

```
Connect one jumper wire:
  PA8  →  PA0

Nucleo pin locations:
  PA8 = CN10 pin 21
  PA0 = CN8  pin 1 (A0)
```

---

## Measurements Explained

| Measurement | Formula |
|---|---|
| Vpp | Vmax − Vmin |
| Vavg | mean of all samples |
| Vrms | √( mean of squares ) |
| Frequency | sample\_rate / mean\_samples\_per\_cycle |
| Period | 1 / frequency |
| Duty cycle | samples above midpoint / total samples × 100 |

---

## FFT Implementation

Hanning window applied before FFT to reduce spectral leakage.
Single-sided magnitude spectrum scaled to Volts.
Peaks annotated with frequency labels.

A 1 kHz square wave produces odd harmonics only — 1 kHz, 3 kHz,
5 kHz, 7 kHz — with amplitudes following 1/n decay. This is the
Fourier series of a square wave and is visible directly in the
spectrum plot.

---

## Drivers Written From Scratch

| Driver | Registers Used |
|---|---|
| GPIO | MODER, OTYPER, OSPEEDR, PUPDR, AFR, BSRR, IDR |
| UART | BRR, CR1, CR2, SR, DR |
| ADC | CR1, CR2, SMPR, SQR, SR, DR, CCR |
| DMA | SxCR, SxNDTR, SxPAR, SxM0AR, LISR, LIFCR |
| TIM1 | PSC, ARR, CCR1, CCMR1, CCER, BDTR, CR1, EGR |
| TIM2 | PSC, ARR, CR1, CR2, EGR |

---

## Tech Stack

| Layer | Technology |
|---|---|
| MCU | STM32F446RE, Cortex-M4, 180MHz |
| Firmware | Bare-metal C, register-level |
| Toolchain | STM32CubeIDE, ARM GCC |
| PC App | Python, PySerial, matplotlib, numpy |
| Protocol | Custom binary, CRC16-CCITT |

---
