#!/usr/bin/env python3
"""Sanity-check an SWM190 image before it is ever flashed. Exit non-zero on any problem."""
import struct, sys

d = open(sys.argv[1], "rb").read()
sp, reset = struct.unpack_from("<II", d, 0)
prot, key, length, memcfg = struct.unpack_from("<IIII", d, 0x1C)
errs = []
if not (0x20000000 < sp <= 0x20001000):
    errs.append(f"initial SP 0x{sp:08X} not in 4 KB data RAM")
if not (reset & 1) or reset >= len(d):
    errs.append(f"reset vector 0x{reset:08X} not a Thumb address inside the image")
if prot != 0:
    errs.append(f"protection word @0x1C = 0x{prot:08X} (must be 0!)")
if key != 0x0B11FFAC:
    errs.append(f"boot key @0x20 = 0x{key:08X} (must be 0x0B11FFAC)")
if length % 0x1000 or length < len(d) or length > 0x8000:
    errs.append(f"length @0x24 = 0x{length:X} (image is 0x{len(d):X})")
if memcfg != 2:
    errs.append(f"MEM_CONFIG @0x28 = {memcfg} (expected 2)")
if d[0xC0:0xC4] != bytes.fromhex("f8231868"):
    errs.append("QSPI switch stub missing at 0xC0 (Reset_Handler calls 0xC1)")
print(f"image {sys.argv[1]}: {len(d)} bytes, SP=0x{sp:08X} reset=0x{reset:08X} "
      f"prot=0x{prot:X} key=0x{key:08X} len=0x{length:X} memcfg={memcfg}")
if errs:
    print("IMAGE CHECK FAILED:\n  " + "\n  ".join(errs))
    sys.exit(1)
print("image check OK")
