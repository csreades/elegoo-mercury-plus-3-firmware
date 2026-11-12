# Flashing

## Back up first

Read **`0x41000000`–`0x4101DFFF`** (the serial flash) over SWD. Do **not** dump address `0x0`: that is
the code SRAM copy. The stock image is in [`../original-firmware/`](../original-firmware/).
On my unit the SWD pads are labelled on the silkscreen and already have a header fitted, so no
soldering is needed for this.

## Image requirements

`firmware/check_image.py` enforces these:

- `0x1C` = 0 (protection word; anything else may lock SWD)
- `0x20` = `0x0B11FFAC`, `0x24` = image length rounded up to 4 KB (≤ 0x8000), `0x28` = 2
- Vector table at 0 with the "switch QSPI" stub at `0xC0` (the reset handler calls `0xC1`)

## Programming the flash with SWD

Halt the core, then drive the serial-flash controller (SFC, base `0x4004A000`) with plain memory writes.

Registers: `CFG +0x00`, `TIM +0x04`, `SR +0x08` (bit 0 busy), `IF +0x0C` (bit0 done, bit1 error, bit2
timeout, write-1-to-clear), `GO +0x14`, `ADDR +0x18`, `CMD +0x24`.

```
Once:   TIM = (TIM & ~0xFFFF) | 0xFF3C       ; WIP poll window ~0.5 s (reset value times out on erase)

Erase 4 KB sector at offset X (0-based):
        wait SR.0 == 0
        CFG = (CFG & ~0xF) | 0x1027           ; WREN | CMDWREN | CMDTYPE=7
        CMD = 0x20                            ; read back: must be 0x20 before GO
        ADDR = X                              ; 0-based, not 0x41000000+X
        GO = 1 ; wait GO == 0 ; wait SR.0 == 0
        CFG &= ~0x1027 ; check IF

Program words at offset X:
        CFG |= 0x1000
        write each word to 0x41000000 + X + 4*i (stay inside a 256-byte page)
        wait SR.0 == 0 ; CFG &= ~0x1000 ; check IF
```

Notes from testing on hardware:

- Without **CMDWREN (bit 5)** the `CMD` write is ignored and the controller re-sends whatever opcode is
  latched. The boot ROM's own IAP routine (`0x01000601`) uses `0x1007` and has this problem, so don't rely on it.
  If `CMD` holds `0x01`, the controller sends WRITE STATUS; the flash rejected it in our test.
- Burst writes through the AHB-AP stall while the flash programs (~1 ms per word). The SWD probe must
  keep retrying WAIT responses (a time budget, not a small retry count).
- The flash window returns correct data only for **aligned 32-bit reads**; byte reads are lane-shuffled.
- After programming, **power-cycle** (or use the firmware's `REBOOT`): a plain reset does not reload.

## Updating over UART / WiFi

Once the replacement firmware is installed, it can update itself through its console (`FL ...`
commands, see [`console.md`](console.md)). With the ESP8266 bridge:

```sh
python3 tools/washcure.py flash-unit firmware/wnc_fw.bin
```

## Recovery

SWD stays available whatever the image contains, because the boot ROM runs first, provided the
protection word is 0. The ROM's serial bootloader (PB0 high at reset) is a further fallback; its
protocol is undocumented.
