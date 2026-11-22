# Wash & Cure: reverse engineering and replacement firmware

Open replacement firmware and WiFi control for the **Elegoo Mercury Plus 3.0** resin-printer **wash & cure station**
built around a **Synwit SWM190CBT7-50** (Arm Cortex-M0) microcontroller.

> **Device:** Elegoo **Mercury Plus 3.0** wash & cure station

This repo contains:

| Folder | What |
|---|---|
| [`docs/`](docs/) | Hardware findings (pin map, boot/flash scheme), console protocol, flashing guide |
| [`original-firmware/`](original-firmware/) | Dump of the stock firmware, read from the MCU's serial flash over SWD |
| [`reverse-engineering/`](reverse-engineering/) | Annotated Ghidra decompilation of the stock firmware + scripts |
| [`firmware/`](firmware/) | Replacement firmware for the SWM190 (C, GCC, Synwit SDK) |
| [`esp_bridge/`](esp_bridge/) | ESP8266 (NodeMCU) WiFi bridge: web UI, HTTP API, TCP console, OTA for both chips |
| [`tools/`](tools/) | `washcure.py`: control and update the unit over WiFi |

## What the replacement firmware does

It reproduces the stock behaviour (same motor PWM settings, start/stop ramp shapes, wash direction reversal at
half time, cure lid interlock and alarm, button behaviour, LEDs, beeps) and adds:

- **Power-on in CURE mode** at 10:00. Selecting WASH loads 20:00, CURE loads 10:00 (both configurable, saved to flash).
- After a cycle finishes, the mode stays selected and its default time is reloaded.
- The display blinks **`LId`** while a cure is paused because the lid is open.
- **Gentler wash reversal** (1.1): the stock sequence cut the motor at about 11 % speed and ramped straight
  back up, which could stall against liquid still swirling the old way. Now it slows evenly to a crawl,
  pauses, then speeds up with constant acceleration (`CFG REV_DOWN|REV_DWELL|REV_UP`).
- Non-blocking design (1 ms tick) instead of busy-wait loops.
- A **UART console** (115200 8N1 on the PCB's RX/TX pads) giving full control and read-back:
  mode/time/start/pause/stop, lid state, motor speed and direction, LEDs, buttons (events and
  simulated presses), buzzer (beeps and tones), display text, settings, raw pins/registers,
  and **flash read/erase/write + reboot**, so the firmware can be updated over UART (and therefore WiFi).
- Safety: UV only with the lid closed unless the interlock is explicitly overridden over the console
  (`INTERLOCK OFF CONFIRM`, reset on every boot); the boot-header protection word can never be written
  non-zero (that would lock out SWD).

See [`docs/console.md`](docs/console.md) for the full command list.

## Hardware summary

- MCU: Synwit **SWM190CBT7-50**, Cortex-M0, LQFP48, 24 MHz crystal, PLL to 60 MHz.
- Program storage: 128 KB Puya serial flash (JEDEC `85 40 11`), memory-mapped at `0x41000000`.
  At power-on the boot ROM copies the image into 32 KB of code SRAM at `0x0` and runs it from there.
- Stepper driver (STEP/DIR/EN + mode pins), UV LED driver, 4-digit 7-segment display, 5 capacitive
  buttons with LEDs, lid switch, buzzer.
- Power: external 24 V DC brick.
- SWD on PA2 (SWCLK) / PA3 (SWDIO). On my unit the SWD pads are labelled on the silkscreen and have a
  header fitted, so no soldering is needed to flash or debug.
- UART1 pads: pad "RX" = PE7 (UART1 TX), pad "TX" = PE5 (UART1 RX). The pads are silkscreened but
  **not populated** on my unit, so some soldering (a header or flying leads) is needed to use the UART.

Details, including which findings were verified on hardware: [`docs/hardware.md`](docs/hardware.md).

## Building

**Unit firmware** (needs `arm-none-eabi-gcc`, e.g. Arm GNU Toolchain 14.x):

```sh
cd firmware
make                      # -> wnc_fw.bin, header checked by check_image.py
```

**ESP8266 bridge** (Arduino core for ESP8266 3.x, board "NodeMCU 1.0", flash layout 4MB/FS:2MB):

```sh
cp esp_bridge/secrets.example.h esp_bridge/secrets.h   # add your 2.4 GHz WiFi details
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2:eesz=4M2M esp_bridge
```

## Flashing

- **First time:** over SWD. Program the image at flash offset 0 (`0x41000000`), see
  [`docs/flashing.md`](docs/flashing.md) for the flash-controller recipe, and **back up the stock
  firmware first** (or use the dump in `original-firmware/`).
- **After that:** over UART or WiFi. With the ESP bridge:
  `python3 tools/washcure.py flash-unit firmware/wnc_fw.bin` (about 17 s, CRC-verified).
- ESP8266 updates: `python3 tools/washcure.py flash-esp <esp_bridge.ino.bin>` or the `/update` web page.

## Wiring the ESP8266 bridge (NodeMCU)

The unit's UART pads are silkscreened but not populated, so solder a header or leads to them first.

| NodeMCU | Unit |
|---|---|
| D8 (GPIO15), TX | pad "TX" (PE5, unit RX) |
| D7 (GPIO13), RX | pad "RX" (PE7, unit TX) |
| GND | GND |

## License

Original code in this repository is MIT licensed (see [`LICENSE`](LICENSE)). The Synwit SDK in
`firmware/sdk/` is Apache-2.0 and the CMSIS headers are under Arm's licence (notices kept in the files).
The stock firmware dump and its decompilation are the manufacturer's work, included for
interoperability and research; they will be removed on request.

Not affiliated with or endorsed by Elegoo or Synwit.

## Disclaimer

This involves UV LEDs, moving parts and a 24 V DC supply (the unit runs from an external 24 V brick).
The interlock override exists for testing and is off by default; UV light is harmful to eyes and skin.
Use at your own risk.
