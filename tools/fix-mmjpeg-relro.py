#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Make the mako ARM IDCT literal loads position-independent.

The two known blobs have ten RELATIVE relocations in the IDCT literal pool.
Replace the LDRs with branches to PC-relative address loads in unused RX
padding, then remove the obsolete relocations. No load segment becomes
writable and executable. Reject any layout or instruction that differs.
"""

import argparse
import hashlib
from pathlib import Path
import struct

LAYOUTS = {
    "05550c453d1e34b3b080cb48824619f6a298ed8c1ad8783c6f3655081ad27509",
    "695764e9bc1669d4897e50c57c3aa0ce657d2c50d5c2d9ce71f24b7b1278f0a5",
}
ELF = struct.Struct("<16sHHIIIIIHHHHHH")
PHDR = struct.Struct("<8I")
SHDR = struct.Struct("<10I")
WORD = struct.Struct("<I")
REL = struct.Struct("<II")
DYN = struct.Struct("<II")
PT_LOAD, PT_DYNAMIC = 1, 2
PF_X, PF_W = 1, 2
DT_NULL, DT_REL, DT_RELSZ, DT_RELENT, DT_TEXTREL, DT_FLAGS = 0, 17, 18, 19, 22, 30
DT_RELCOUNT, DF_TEXTREL, R_ARM_RELATIVE = 0x6FFFFFFA, 4, 23


def require(condition, message):
    if not condition:
        raise ValueError(message)


def branch(address, target):
    delta = target - (address + 8)
    require(delta % 4 == 0 and -(1 << 25) <= delta < (1 << 25),
            "ARM branch out of range")
    return 0xEA000000 | ((delta // 4) & 0xFFFFFF)


def patch(blob):
    h = ELF.unpack_from(blob)
    require(h[0][:7] == b"\x7fELF\x01\x01\x01" and h[1:3] == (3, 40),
            "expected ELF32 ARM shared library")
    phoff, shoff, phnum, shnum, shstr = h[5], h[6], h[10], h[12], h[13]
    require(h[9] == PHDR.size and h[11] == SHDR.size, "unknown ELF header layout")
    ph = [PHDR.unpack_from(blob, phoff + i * PHDR.size) for i in range(phnum)]
    sh = [SHDR.unpack_from(blob, shoff + i * SHDR.size) for i in range(shnum)]
    names = blob[sh[shstr][4]:sh[shstr][4] + sh[shstr][5]]
    sections = {names[s[0]:].split(b"\0", 1)[0]: (i, s)
                for i, s in enumerate(sh)}
    dynamic, = [p for p in ph if p[0] == PT_DYNAMIC]
    entries = []
    for pos in range(dynamic[1], dynamic[1] + dynamic[4], DYN.size):
        tag, value = DYN.unpack_from(blob, pos)
        if tag == DT_NULL:
            break
        entries.append((tag, value))
    tags = dict(entries)
    rx_index, rx = [(i, p) for i, p in enumerate(ph)
                    if p[0] == PT_LOAD and p[6] & PF_X][0]
    require(all(not (p[6] & PF_W and p[6] & PF_X)
                for p in ph if p[0] == PT_LOAD), "writable executable segment")
    require(rx[1:4] == (0, 0, 0) and rx[4] == rx[5], "unexpected RX layout")
    _, relsec = sections[b".rel.dyn"]
    require(relsec[3] == tags[DT_REL] and relsec[5] == tags[DT_RELSZ]
            and tags[DT_RELENT] == REL.size, "unexpected relocation table")
    rel_offset, rel_size = relsec[4:6]
    relocs = list(REL.iter_unpack(blob[rel_offset:rel_offset + rel_size]))
    moved = [(offset, info) for offset, info in relocs if offset < rx[5]]
    if not moved and DT_TEXTREL not in tags and not tags.get(DT_FLAGS, 0) & DF_TEXTREL:
        return blob
    require(hashlib.sha256(blob).hexdigest() in LAYOUTS, "unknown original libmmjpeg")
    require(len(moved) == 10 and relocs[:10] == moved and
            all(info == R_ARM_RELATIVE for _, info in moved), "unexpected text relocations")
    _, idct = sections[b"idct_func"]
    _, constants = sections[b"RODataArea"]
    offsets = [offset for offset, _ in moved]
    require(offsets == list(range(offsets[0], offsets[0] + 40, 4)) and
            offsets[-1] + 4 == idct[3] + idct[5] and idct[3] == idct[4],
            "IDCT literal pool layout differs")
    refs = {}
    for pc in range(idct[3], offsets[0], 4):
        instr = WORD.unpack_from(blob, pc)[0]
        if instr & 0xFFFF0000 != 0xE59F0000:
            continue
        literal = pc + 8 + (instr & 0xFFF)
        if literal in offsets:
            require(literal not in refs, "literal referenced twice")
            refs[literal] = (pc, (instr >> 12) & 15)
    require(set(refs) == set(offsets), "IDCT references differ")
    stub_start = (rx[4] + 3) & ~3
    stub_end = stub_start + len(offsets) * 16
    next_load = min(p[1] for p in ph if p[0] == PT_LOAD and p[1] != 0)
    require(stub_end <= next_load and not any(blob[rx[4]:stub_end]),
            "RX padding is occupied")
    out = bytearray(blob)
    for n, literal in enumerate(offsets):
        pc, reg = refs[literal]
        target, = WORD.unpack_from(blob, literal)
        require(reg not in (13, 15) and constants[3] <= target < constants[3] + constants[5],
                "unexpected IDCT operand")
        stub = stub_start + n * 16
        WORD.pack_into(out, pc, branch(pc, stub))
        struct.pack_into("<4I", out, stub,
                         0xE59F0004 | reg << 12,         # ldr Rd, [pc, #4]
                         0xE08F0000 | reg << 12 | reg,   # add Rd, pc, Rd
                         branch(stub + 8, pc + 4),
                         (target - (stub + 12)) & 0xFFFFFFFF)
    updated_rx = list(rx)
    updated_rx[4] = updated_rx[5] = stub_end
    PHDR.pack_into(out, phoff + rx_index * PHDR.size, *updated_rx)

    new_rel = b"".join(REL.pack(*r) for r in relocs[10:])
    out[rel_offset:rel_offset + rel_size] = new_rel.ljust(rel_size, b"\0")
    rel_index, rel_header = sections[b".rel.dyn"]
    rel_header = list(rel_header)
    rel_header[5] = len(new_rel)
    SHDR.pack_into(out, shoff + rel_index * SHDR.size, *rel_header)

    replacements = {DT_RELSZ: len(new_rel), DT_FLAGS: tags.get(DT_FLAGS, 0) & ~DF_TEXTREL}
    if DT_RELCOUNT in tags:
        require(tags[DT_RELCOUNT] >= 10, "invalid relative relocation count")
        replacements[DT_RELCOUNT] = tags[DT_RELCOUNT] - 10
    new_dyn = b"".join(DYN.pack(tag, replacements.get(tag, value))
                       for tag, value in entries if tag != DT_TEXTREL)
    out[dynamic[1]:dynamic[1] + dynamic[4]] = new_dyn.ljust(dynamic[4], b"\0")
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    options = parser.parse_args()
    options.output.write_bytes(patch(options.source.read_bytes()))


if __name__ == "__main__":
    main()
