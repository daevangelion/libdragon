#!/usr/bin/env python3
"""Run with a linked N64_TLBCODE ELF; all mutations stay in TMPDIR."""
import contextlib
import io
import pathlib
import runpy
import struct
import sys
import tempfile
import types
import unittest

TOOL = runpy.run_path(str(pathlib.Path(__file__).with_name('tlbcode.py')))
ELF_PATH = pathlib.Path(sys.argv.pop(1)) if len(sys.argv) > 1 else None


class TlbcodeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name)

    def test_generate_rejects_injection_and_oversized_directory(self):
        for text in ('{"Bad": [".text.x"]}', '{"ok": ["*(.text)"]}',
                     '{' + ','.join(f'"u{i}":[".text.x"]' for i in range(17)) + '}'):
            source = self.root / 'units.json'
            source.write_text(text)
            with self.assertRaises(ValueError):
                TOOL['generate'](types.SimpleNamespace(units=source,
                    sections=self.root / 'sections.ld', directory=self.root / 'directory.ld'))

    def test_generate_rejects_data_selectors(self):
        for name in ('.data.x', '.sdata.x', '.bss.x', '.sbss.x'):
            with self.subTest(section=name):
                source = self.root / 'units.json'
                source.write_text('{"bad":["' + name + '"]}')
                with self.assertRaisesRegex(ValueError, 'must name text or rodata'):
                    TOOL['generate'](types.SimpleNamespace(units=source,
                        sections=self.root / 'sections.ld', directory=self.root / 'directory.ld'))

    def test_object_keep_and_link_limit(self):
        source = self.root / 'units.json'
        source.write_text('{"fmv":["*h264_decoder.o"]}')
        section = self.root / 'sections.ld'
        TOOL['generate'](types.SimpleNamespace(units=source, sections=section,
            directory=self.root / 'directory.ld'))
        self.assertIn('KEEP(*h264_decoder.o(keep.text.* keep.rodata.*))', section.read_text())
        self.assertIn('62 * 4096', section.read_text())

    def elf_copy(self, data=None):
        self.assertIsNotNone(ELF_PATH, 'supply a linked N64_TLBCODE ELF')
        path = self.root / 'test.elf'
        path.write_bytes(ELF_PATH.read_bytes() if data is None else data)
        return path

    def pack(self, path):
        output = str(self.root / 'test.tlbcode')
        with contextlib.redirect_stdout(io.StringIO()):
            TOOL['pack'](types.SimpleNamespace(elf=path, output=output))
        return pathlib.Path(output)

    def test_pack_hash_binding_and_idempotence(self):
        path = self.elf_copy()
        blob = self.pack(path)
        before = blob.read_bytes()
        elf = TOOL['Elf'](path)
        ident = elf.sections['.tlbcode_id']
        self.assertEqual(before[8:40], elf.span(ident[4], 32))
        self.pack(path)
        self.assertEqual(before, blob.read_bytes())
        original = path.read_bytes()
        resident = next(p for p in elf.phdrs if p[0] == 1 and p[4] and p[6] & 1)
        data = bytearray(original)
        data[resident[1]] ^= 1
        self.pack(self.elf_copy(data))
        self.assertNotEqual(before[8:40], blob.read_bytes()[8:40])
        unit = next(s for n, s in elf.sections.items() if n.startswith('.tlbcode.'))
        data = bytearray(original)
        data[unit[4]] ^= 1
        path = self.elf_copy(data)
        self.pack(path)
        self.assertNotEqual(before[8:40], blob.read_bytes()[8:40])
        report = pathlib.Path(str(blob) + '.refs').read_text()
        self.assertIn('.text:test_tlbcode@', report)
        self.assertIn('-> test:tlbcode_test_function@', report)

    def test_constructor_into_unit_is_rejected(self):
        path = self.elf_copy()
        elf = TOOL['Elf'](path)
        ctor = elf.sections['.ctors']
        self.assertGreaterEqual(ctor[5], 8)
        unit = next(s for n, s in elf.sections.items() if n.startswith('.tlbcode.'))
        data = bytearray(path.read_bytes())
        struct.pack_into('>I', data, ctor[4] + 4, unit[3])
        path = self.elf_copy(data)
        with self.assertRaisesRegex(ValueError, 'ctors.*points into code unit'):
            self.pack(path)
        self.assertFalse((self.root / 'test.tlbcode').exists())

    def test_pack_rejects_writable_and_nobits_units(self):
        path = self.elf_copy()
        elf = TOOL['Elf'](path)
        index = next(i for i, name in enumerate(elf.names) if name.startswith('.tlbcode.'))
        section = elf.sh[index]
        table = struct.unpack_from('>I', elf.data, 32)[0]
        for field, value in ((8, section[2] | 1), (4, 8)):
            with self.subTest(field=field):
                data = bytearray(ELF_PATH.read_bytes())
                struct.pack_into('>I', data, table + index * 40 + field, value)
                with self.assertRaisesRegex(ValueError, 'read-only PROGBITS'):
                    self.pack(self.elf_copy(data))


if __name__ == '__main__':
    unittest.main()
