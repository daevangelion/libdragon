#!/usr/bin/env python3
"""Generate TLB unit linker fragments and pack an ELF's non-loadable units."""
import argparse
import hashlib
import json
import pathlib
import re
import struct

PAGE = 4096
NAME = re.compile(r"[a-z][a-z0-9_]{0,30}\Z")
PATTERN = re.compile(r"[a-zA-Z0-9_./*?\[\]-]+\Z")


def generate(args):
    units = json.loads(pathlib.Path(args.units).read_text())
    if not isinstance(units, dict) or not 1 <= len(units) <= 16:
        raise ValueError("units must be an object with 1..16 entries")
    sections, directory = [], []
    for i, (name, patterns) in enumerate(units.items()):
        if not NAME.fullmatch(name) or not isinstance(patterns, list) or not patterns:
            raise ValueError("invalid unit name or empty pattern list")
        body = []
        for pattern in patterns:
            if not isinstance(pattern, str) or not PATTERN.fullmatch(pattern):
                raise ValueError("invalid input pattern")
            if pattern.startswith('.') or pattern.startswith('keep.'):
                body.append(f"KEEP(*({pattern}))")
            else:
                body.append(f"KEEP({pattern}(keep.text.* keep.rodata.*))")
                body.append(f"{pattern}(.text .text.* .rodata .rodata.* .rdata .gnu.linkonce.t.* .gnu.linkonce.r.*)")
        section = f".tlbcode.{name}"
        sections.append(f"{section} 0x{0xc1000000 + i * 0x100000:08x} : {{\n" + '\n'.join(body) + f"\n. = ALIGN(4096);\n}} :units\nASSERT(SIZEOF({section}) > 0 && SIZEOF({section}) <= 62 * 4096, \"empty or oversized unit {name} (maximum 62 pages)\")\n")
        directory.extend(f"BYTE({c});" for c in name.encode().ljust(32, b'\0'))
        directory.append(f"LONG(ADDR({section})); LONG(SIZEOF({section}) / 4096);")
    pathlib.Path(args.sections).write_text('\n'.join(sections))
    pathlib.Path(args.directory).write_text('\n'.join(directory) + '\n')


class Elf:
    def __init__(self, path):
        self.data = bytearray(pathlib.Path(path).read_bytes())
        if self.data[:7] != b'\x7fELF\x01\x02\x01':
            raise ValueError("expected big-endian ELF32")
        h = struct.unpack_from('>16sHHIIIIIHHHHHH', self.data)
        if h[2] != 8 or h[9] != 32 or h[11] != 40:
            raise ValueError("invalid MIPS ELF headers")
        self.phdrs = [struct.unpack_from('>8I', self.data, h[5] + i * 32) for i in range(h[10])]
        self.sh = [struct.unpack_from('>10I', self.data, h[6] + i * 40) for i in range(h[12])]
        strings = self.span(self.sh[h[13]][4], self.sh[h[13]][5])
        self.sections = {}
        self.names = []
        for s in self.sh:
            end = strings.find(b'\0', s[0])
            if end < 0:
                raise ValueError("unterminated section name")
            self.sections[strings[s[0]:end].decode()] = s
            self.names.append(strings[s[0]:end].decode())

    def span(self, offset, size):
        if offset > len(self.data) or size > len(self.data) - offset:
            raise ValueError("ELF extent outside file")
        return bytes(self.data[offset:offset + size])

    def symbols(self, index):
        section = self.sh[index]
        if section[1] != 2 or section[9] != 16 or section[5] % 16:
            raise ValueError("invalid ELF symbol table")
        strings_section = self.sh[section[6]]
        strings = self.span(strings_section[4], strings_section[5])
        symbols = []
        for offset in range(section[4], section[4] + section[5], 16):
            name, value, size, info, other, ndx = struct.unpack_from('>IIIBBH', self.data, offset)
            end = strings.find(b'\0', name)
            if end < 0:
                raise ValueError("invalid ELF symbol name")
            symbols.append((strings[name:end].decode(), value, size, info & 15, ndx))
        return symbols

    def audit(self, units):
        def target(address):
            for name, section in units:
                if section[3] <= address < section[3] + section[5]:
                    return name
            return None

        for name, section in self.sections.items():
            if any(name == prefix or name.startswith(prefix + '.') for prefix in
                   ('.ctors', '.dtors', '.preinit_array', '.init_array', '.fini_array')):
                data = self.span(section[4], section[5])
                if len(data) % 4:
                    raise ValueError(f"unaligned constructor table {name}")
                for (address,) in struct.iter_unpack('>I', data):
                    unit = target(address)
                    if unit:
                        raise ValueError(f"{name} points into code unit {unit}: 0x{address:08x}")

        report = []
        tables = {}
        relocations = [s for s in self.sh if s[1] == 9]
        if not relocations:
            raise ValueError("link with --emit-relocs to audit resident-to-unit references")
        for rel in relocations:
            source = self.sh[rel[7]]
            if not source[2] & 2 or self.names[rel[7]].startswith('.tlbcode.'):
                continue
            if rel[9] != 8 or rel[5] % 8:
                raise ValueError("invalid ELF relocation table")
            if rel[6] not in tables:
                tables[rel[6]] = self.symbols(rel[6])
            symbols = tables[rel[6]]
            owners = sorted((s for s in symbols if s[4] == rel[7] and s[3] in (1, 2)),
                            key=lambda s: s[1])
            for address, info in struct.iter_unpack('>II', self.span(rel[4], rel[5])):
                symbol = symbols[info >> 8]
                unit = target(symbol[1])
                if not unit:
                    continue
                owner = next((s[0] for s in reversed(owners) if s[1] <= address and
                              (not s[2] or address < s[1] + s[2])), '<section>')
                report.append(f"{self.names[rel[7]]}:{owner}@0x{address:08x} -> "
                              f"{unit}:{symbol[0] or '<section>'}@0x{symbol[1]:08x}")
        return '\n'.join(sorted(set(report))) + '\n'


def pack(args):
    elf = Elf(args.elf)
    ident = elf.sections['.tlbcode_id']
    if ident[5] != 32:
        raise ValueError("invalid resident build ID")
    elf.data[ident[4]:ident[4] + 32] = bytes(32)
    digest = hashlib.sha256()
    for ph in elf.phdrs:
        if ph[0] == 1:
            digest.update(struct.pack('>III', ph[2], ph[3], ph[5]))
            digest.update(elf.span(ph[1], ph[4]))
    units = [(n[9:], s) for n, s in elf.sections.items() if n.startswith('.tlbcode.')]
    units.sort(key=lambda item: item[1][3])
    if not 1 <= len(units) <= 16:
        raise ValueError("expected 1..16 code units")
    report = elf.audit(units)
    offset = (48 + len(units) * 48 + PAGE - 1) & -PAGE
    records, payload = bytearray(), bytearray(offset)
    for name, s in units:
        if (not NAME.fullmatch(name) or s[3] % 8192 or not s[5] or s[5] % PAGE or
                s[5] > 62 * PAGE or not 0xc1000000 <= s[3] < 0xc2000000):
            raise ValueError("invalid unit extent")
        for ph in elf.phdrs:
            if ph[0] == 1 and s[3] < ph[2] + ph[5] and ph[2] < s[3] + s[5]:
                raise ValueError("unit is in a PT_LOAD segment")
        records.extend(struct.pack('>32sIIII', name.encode(), s[3], s[5] // PAGE, offset, s[5]))
        payload.extend(elf.span(s[4], s[5]))
        digest.update(struct.pack('>II', s[3], s[5]))
        digest.update(elf.span(s[4], s[5]))
        offset += s[5]
        print(f"tlbcode {name}: VA {s[3]:08x}, {s[5] // PAGE} pages, {(s[5] // PAGE + 1) // 2} TLB entries")
    build_id = digest.digest()
    elf.data[ident[4]:ident[4] + 32] = build_id
    payload[:48] = struct.pack('>8s32sII', b'TLBCODE1', build_id, len(units), len(payload))
    payload[48:48 + len(records)] = records
    pathlib.Path(args.output).write_bytes(payload)
    pathlib.Path(args.elf).write_bytes(elf.data)
    pathlib.Path(args.output + '.refs').write_text(report)
    print(f"tlbcode resident build: {build_id.hex()}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    gen = sub.add_parser('generate')
    gen.add_argument('units')
    gen.add_argument('sections')
    gen.add_argument('directory')
    gen.set_defaults(run=generate)
    extract = sub.add_parser('pack')
    extract.add_argument('elf')
    extract.add_argument('output')
    extract.set_defaults(run=pack)
    args = parser.parse_args()
    try:
        args.run(args)
    except (ValueError, KeyError, IndexError, struct.error, OSError) as error:
        parser.exit(1, f'tlbcode: {error}\n')


if __name__ == '__main__':
    main()
