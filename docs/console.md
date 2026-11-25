# Console protocol

UART1, 115200 8N1, on the PCB "RX"/"TX" pads (silkscreened but not populated on my unit: solder a
header or leads first). Through the ESP bridge the same console is available on
TCP port 23, and single commands via `GET /api/cmd?c=<command>`.

- One command per line, case-insensitive. Numbers are decimal, except `PEEK`/`POKE`/`FL` arguments, which are hex.
- Replies start with `OK` or `ERR`. `STATUS` replies with a `STAT` line, `CFG` with a `CFG` line.
- Asynchronous events start with `EVT`.

## Commands

| Command | Description |
|---|---|
| `STATUS` / `S` | Full status line (see below) |
| `STREAM <ms>` | Send `STATUS` periodically (0 = off) |
| `MODE [WASH\|CURE]` | Get/select mode (loads that mode's default time). Not while running |
| `START` / `PAUSE` / `TOGGLE` | Run control |
| `STOP` | Pause and reload the mode's default time |
| `TIME [s]` | Get (`OK remaining total`) or set remaining time, 0–5999 s |
| `PRESS START\|PLUS\|MINUS\|WASH\|CURE` | Simulate a short button press |
| `CFG` | Show settings |
| `CFG WASH_DEFAULT\|CURE_DEFAULT <s>` | Default times |
| `CFG WASH_PERIOD\|CURE_PERIOD <n>` | Motor speed as a PWM period (smaller = faster; stock 55 / 35156) |
| `CFG CURE_DIR 0\|1` | Turntable direction |
| `CFG SPR <n>` | Steps per revolution, used only for the rpm estimate |
| `CFG KEY_BEEPS 0\|1` | Button click beeps |
| `CFG WASH_REVERSE 0\|1` | Reverse direction at half time. Default 0 (off) since 1.2: the reversal stalls the motor on this firmware (cause not found), so a wash runs one way |
| `CFG REV_DOWN\|REV_DWELL\|REV_UP <ms>` | Wash reversal: slow-down time, pause at standstill, speed-up time (defaults 3000 / 2500 / 8000) |
| `SAVE` / `FACTORY` | Save settings to flash / restore defaults (in RAM; `SAVE` to persist) |
| `BEEP <ms>` | Beep |
| `TONE <hz> <ms>` | Square wave on the buzzer (bit-banged; an active buzzer gives limited pitch control) |
| `DISP <text> [ms]` / `DISP AUTO` | Show text on the display / back to the timer |
| `LOCK` / `UNLOCK` | Ignore / accept the physical buttons (events are still reported) |
| `INTERLOCK ON` / `INTERLOCK OFF CONFIRM` | Lid interlock for UV and cure. Always ON after boot |
| `MANUAL ON\|OFF` | Take direct control of the outputs (stops any cycle) |
| `LED WASH\|CURE\|PLUS\|MINUS\|START\|ALL 0\|1` | Manual only |
| `UV [0\|1]` | Get / set UV (set: manual only; refused with the lid open unless the interlock is off) |
| `BUZ 0\|1` | Buzzer on/off (manual only) |
| `MOTOR RUN <period> [dir]` | Constant speed immediately (manual only) |
| `MOTOR RAMP <period> [dir]` | Ramp to speed (manual only) |
| `MOTOR STOP` / `MOTOR OFF` | Ramp down / stop immediately |
| `MOTOR EN 0\|1`, `MOTOR DIR 0\|1` | Driver enable and direction |
| `MOTOR PIN A12\|A13\|A14\|D0 0\|1` | Driver config pins |
| `PIN <A-E><0-15> [0\|1\|IN\|OUT]` | Read or drive any pin |
| `PEEK <addr>` / `POKE <addr> <val>` | 32-bit register/memory access |
| `FL READ <off> <len≤40h>` | Read flash bytes (hex) |
| `FL CRC <off> <len>` | CRC-32 of a flash range |
| `FL UNLOCK` | Allow writes below `0x8000` (the firmware image) for 30 s |
| `FL ERASE <off>` | Erase a 4 KB sector |
| `FL WRITE <off> <hex>` | Program up to 64 bytes inside one 256-byte page. A non-zero protection word at `0x1C` is refused |
| `REBOOT` | Reload the firmware from flash and restart (use after an update) |
| `REBOOT WARM` | Restart without reloading |
| `VER` | Version and build time |

## STATUS fields

`mode` WASH/CURE · `state` IDLE/RUNNING/LID_PAUSED/DONE · `time`/`total` seconds · `lid` OPEN/CLOSED ·
`uv` · `motor_en` · `dir` · `period` (applied PWM period, 0 = stopped) · `step_hz` and `rpm` (estimates) ·
`leds` (WASH CURE PLUS MINUS START, 1 = lit) · `buttons` (START MINUS PLUS CURE WASH, 1 = touched) ·
`buzzer` · `manual` · `interlock` · `locked` · `uptime` (s)

## Events

`EVT MODE <m>` · `EVT STATE RUNNING|PAUSED` · `EVT DONE <m>` · `EVT LID OPEN|CLOSED` ·
`EVT BTN <name> DOWN` · `EVT BTN <name> UP <held_ms>` · `EVT MOTOR REVERSE` · `EVT WARNING ...`

## Updating the firmware over the console

1. `FL UNLOCK`, then `FL ERASE` each 4 KB sector the image covers.
2. `FL WRITE` the image in 64-byte chunks, writing the first chunk (the boot header) **last**.
3. Compare `FL CRC 0 <len>` against the image, then `REBOOT`.

The running firmware is in SRAM, so an interrupted update doesn't affect the unit until it is power-cycled.
`esp_bridge` implements this flow (`POST /api/mcu/update`).
