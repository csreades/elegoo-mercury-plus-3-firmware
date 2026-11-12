# Stock firmware dump

Read over SWD from the SWM190's serial flash (`0x41000000`), with the core halted. Dumped twice and
compared byte for byte.

| File | Contents |
|---|---|
| `flash_0x41000000_120k.bin` | Full 120 KB user flash. Only `0x0000–0x2DCF` holds data; the rest is erased (`0xFF`) |
| `original_image_0x3000.bin` | The first 12 KB: the image the boot ROM loads (header length `0x3000`) |
| `SHA256SUMS` | Checksums |

Built with Keil/ARMCC and the Synwit standard peripheral library. To restore it, program
`original_image_0x3000.bin` at flash offset 0 (see [`../docs/flashing.md`](../docs/flashing.md)) and power-cycle.

This is the manufacturer's copyrighted firmware, provided for interoperability, backup and research.
