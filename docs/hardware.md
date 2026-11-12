# Hardware

Findings marked **verified** were confirmed on the real board. Others come from reverse-engineering
the stock firmware.

## MCU

| | |
|---|---|
| Part | **Synwit SWM190CBT7-50** (marking "SWM19SCBT7-50"), Cortex-M0, LQFP48, 2.3–3.6 V |
| IDs | CPUID `0x410CC200`, SWD DP IDCODE `0x0BB11477`, chip ID `0x854012` at `0x4000008C` |
| Clock | 24 MHz crystal on PC0/PC1, PLL (÷6 ×30 ÷8) → **60 MHz** core (**verified** via UART baud) |
| Flash | Puya serial flash, JEDEC `85 40 11` (1 Mbit / 128 KB), mapped at `0x41000000` (**verified**) |
| Debug | SWD: PA2 = SWCLK, PA3 = SWDIO. On my unit the SWD pads are labelled on the silkscreen and have a header fitted. No read protection on the stock unit. |

### Boot and memory scheme (verified)

The CPU does **not** execute from flash. At power-on the boot ROM (`0x01000000`) reads a header
from the serial flash and copies the image into a 32 KB code SRAM at `0x00000000`, then runs it.

| Flash offset | Stock value | Meaning |
|---|---|---|
| `0x1C` | `0` | Protection word. `0x43211234` / `0xABCD1234` enable SWD read protection modes. **Keep 0.** |
| `0x20` | `0x0B11FFAC` | Synwit boot key; required |
| `0x24` | `0x3000` | Bytes copied flash → code SRAM (4 KB blocks) |
| `0x28` | `2` | MEM_CONFIG: 2 = cache off, 32 KB code SRAM + 4 KB data SRAM |

Consequences:

- Reading address `0x0` over SWD gives the **SRAM copy**, aliased every 32 KB. Dump the flash from `0x41000000`.
- A plain `SYSRESETREQ` does **not** reload from flash. The replacement firmware's `REBOOT` command copies
  flash → SRAM itself (from a RAM-resident routine), then resets.
- Running firmware can rewrite the whole flash safely, since it runs from SRAM.
- **PB0 is the ROM's ISP strap:** high at reset → serial bootloader instead of the app. Held low on the board.
- The stock firmware's busy-wait delays were calibrated for 24 MHz but run at 60 MHz, so its "1 ms" is ~0.415 ms.

## Pin map

### Outputs

| Pin | Function | Notes |
|---|---|---|
| PB14, PB12, PB10, PB8, PB7, PB13, PB11 | 7-segment a, b, c, d, e, f, g | Active low (common anode) |
| PB9 | Colon | Active low |
| PB15, PC2, PC4, PC5 | Digit 1–4 select | Active low, multiplexed |
| **PC3** | **Stepper STEP**, PWM1A (alt func 2) | 50 % duty, PWM clock 60 MHz / 8. Period 55 = wash speed, 35156 = cure turntable |
| **PD1** | **Stepper DIR** | **verified** |
| PA15 | Stepper driver enable | Active low |
| **PA14** | **Driver chopper mode** | **verified**: high = loud/strong (used for wash), low = quiet (cure). Likely TMC spreadCycle/stealthChop |
| PA12, PA13 | Driver config (probably microstep select) | Stock: PA12 high, PA13 low, always |
| PD0 | Driver enable/standby? | Stock: high, always |
| **PA8** | **UV LEDs** | Active high; stock only on in cure with the lid closed |
| PE4 | Buzzer | Active high; plain GPIO (no timer output), probably an active buzzer |
| PA9, PA10, PA11, PE2, PE3 | LEDs in the WASH, CURE, +, −, START buttons | Active low (**verified**) |

### Inputs (external pull-ups)

| Pin | Function |
|---|---|
| PB1 | START / pause (low = touched) |
| PB2 | − 30 s |
| PB3 | + 30 s |
| PB4 | CURE |
| PB5 | WASH |
| PB6 | Lid switch, **1 = open** (**verified**) |

### UART pads (UART1, verified)

| PCB pad label | MCU pin | Function |
|---|---|---|
| "RX" | PE7 | UART1_TX (unit transmits) |
| "TX" | PE5 | UART1_RX (unit receives) |

The labels are from the point of view of the device you connect: your TX goes to "TX".

The console is on **UART1** (UART0 on PA0/PA1 is unused). On my unit the pads are silkscreened but
**not populated**, so some soldering (a header or flying leads) is needed before the UART console or
the ESP bridge can be used.

### Free / other pins

PA0/PA1 (UART0), PA4–PA7, PD2–PD7, PE0, PE1, PE6 are not used by the stock firmware. PC6/PC7 are held
low by something on the board. PB0 is the ISP strap (keep low at reset).

## Stock firmware behaviour (summary)

- **Wash:** motor ramps the PWM period 1000 → 701, jumps to 600 → 55 (~6 s), runs, and at half time ramps
  down 54 → 499, stops, flips DIR and ramps up again. Stop = ramp to 300 then off. Lid not checked.
- **Cure:** turntable at period 35156 in quiet mode; UV on while the lid is closed. Opening the lid stops
  motor, UV and the countdown, and flashes LEDs, display and buzzer until it closes.
- **Buttons:** short press acts on release; ± auto-repeat after a 2 s hold; time 0–30 min in 30 s steps.
- **End of cycle:** three beeps with display flashes.

The annotated decompilation is in [`../reverse-engineering/`](../reverse-engineering/).
