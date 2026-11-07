#!/usr/bin/env python3

# Copyright (c) 2025-2026 EDGEMTech SA

"""Replace /images/ramdisk-1/data in a FIT and refresh its sha256 hash."""
import hashlib
import sys
import libfdt

if len(sys.argv) != 4:
    sys.exit(f"usage: {sys.argv[0]} <orig.itb> <new-ramdisk> <out.itb>")

orig_fit, new_ramdisk, out_fit = sys.argv[1:]

with open(orig_fit, "rb") as f:
    blob = bytearray(f.read())
with open(new_ramdisk, "rb") as f:
    new_data = f.read()

fdt = libfdt.Fdt(blob)
node = fdt.path_offset("/images/ramdisk-1")
old_data = fdt.getprop(node, "data")
print(f"orig ramdisk-1 data: {len(old_data)} bytes")
print(f"new  ramdisk-1 data: {len(new_data)} bytes")

delta = len(new_data) - len(old_data)
if delta > 0:
    fdt.resize(len(blob) + delta + 4096)

fdt.setprop(node, "data", new_data)

hash_node = fdt.subnode_offset(node, "hash-1")
algo = bytes(fdt.getprop(hash_node, "algo")).rstrip(b"\0").decode()
if algo != "sha256":
    sys.exit(f"unexpected hash algo: {algo}")
new_hash = hashlib.sha256(new_data).digest()
fdt.setprop(hash_node, "value", new_hash)
print(f"hash-1 sha256 -> {new_hash.hex()}")

fdt.pack()
out = bytes(fdt.as_bytearray())
with open(out_fit, "wb") as f:
    f.write(out)
print(f"wrote {out_fit} ({len(out)} bytes)")
