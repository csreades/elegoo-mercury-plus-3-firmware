# Reverse engineering

- `original_decompiled.c`: Ghidra 12 decompilation of the stock firmware, with functions named
  (`main`, `TIMR2_IRQ_countdown_1s`, `motor_ramp_up_wash`, ...), literal pools resolved to SWM190
  peripheral registers, and RAM globals labelled.
- `ghidra/`: headless scripts. `Prep.java` (memory map, register labels, vector entries),
  `Rename.java` (names), `Dump.java` (export C).
- `swm190_regs.txt`: SWM190 register address → name map, generated from the Synwit SDK header by `mkregs.py`.
- `swm190_portfuncs.txt`: pin alternate-function table from the SDK.
- `subst.py`: post-processes a Ghidra C dump, replacing `DAT_xxxx` literal-pool references with
  register/RAM names.

Reproduce:

```sh
analyzeHeadless <projdir> wnc -import ../original-firmware/original_image_0x3000.bin \
  -processor "ARM:LE:32:Cortex" -loader BinaryLoader -loader-baseAddr 0 \
  -scriptPath ghidra -preScript Prep.java -postScript Rename.java -postScript Dump.java out.c
python3 subst.py ../original-firmware/original_image_0x3000.bin out.c original_decompiled.c
```

Identification: the reset handler checks chip ID `0x854012` and calls the SDK's "switch QSPI" stub at
`0xC0`. That sequence matches Synwit's `startup_SWM190.s` exactly, and the peripheral addresses match
`SWM190.h` (APB1 `0x40040000`, APB2 `0x400A0000`).
