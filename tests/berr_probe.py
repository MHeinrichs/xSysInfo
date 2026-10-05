#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Stefan Reinauer
"""Exercise assembled guards with injected CPU exception frames.

Requires vasmm68k_mot, amitools and machine68k. Run after changing either
probe; NDK_PATH can override the includes used by the assembler. Faults
are injected at the hardware reads, not supplied by emulated hardware.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from amitools.binfmt.BinFmt import BinFmt
from amitools.binfmt.Relocate import Relocate
from machine68k import CPUType, Machine, Register

ROOT = Path(__file__).resolve().parents[1]
CC = Path(shutil.which("m68k-amigaos-gcc")).resolve()
NDK = os.environ.get("NDK_PATH", str(CC.parent.parent / "m68k-amigaos/ndk-include"))
BASE, EXEC, END, TARGET, OUTPUT = 0x10000, 0x4000, 0x8000, 0x9000, 0xa000
USP, SSP = 0xe0000, 0xf0000
CPUS = [(CPUType.M68000, 0, [(0, 14)]),
        (CPUType.M68010, 1, [(8, 58)]),
        (CPUType.M68020, 3, [(10, 32), (11, 92)]),
        (CPUType.M68030, 7, [(10, 32), (11, 92)]),
        (CPUType.M68040, 15, [(7, 60)]),
        # The 040 core can exercise frame discard and MOVEC CACR for 060.
        # It cannot emulate 060 execution or RTE from a format-4 frame.
        (CPUType.M68040, 143, [(4, 16)])]


def assemble(source, output):
    subprocess.run(["vasmm68k_mot", "-quiet", "-Fhunkexe", "-I", NDK,
                    "-o", str(output), str(source)], check=True)
    image = BinFmt().load_image(str(output))
    assert len(image.get_segments()) == 1
    segment = image.get_segments()[0]
    symbols = {s.get_name().decode(): BASE + s.get_offset()
               for s in segment.get_symtab().get_symbols()}
    return bytes(Relocate(image).relocate([BASE])[0]), symbols


def check(code, symbols, cpu_type, flags, vbr, width, fault, initial_sr):
    machine = Machine(cpu_type, 1024)
    try:
        cpu, mem = machine.cpu, machine.mem
        mem.w_block(BASE, code)
        mem.w32(4, EXEC)
        mem.w16(EXEC + 296, flags)
        cpu.w_reg(Register.VBR, vbr)
        for base in {0, vbr}:
            mem.w32(base + 8, 0xdead0008)
            mem.w32(base + 12, 0xdead000c)
        mem.w32(TARGET, 0x87654321)
        mem.w32(OUTPUT, 0xcafebabe)
        end = machine.create_execute_end("returned")
        mem.w16(END, 0xa000 | machine.traps.alloc(lambda op, pc: end))

        def supervisor(op, pc):
            # Exec Supervisor's contract: call A5 with an RTE frame.
            sp, sr = cpu.r_sp(), cpu.r_sr()
            ret = mem.r32(sp)
            supervisor_sp = sp + 4 if sr & 0x2000 else cpu.r_isp()
            cpu.w_sr((sr | 0x2000) & 0x3fff)
            cpu.w_sp(supervisor_sp)
            if not sr & 0x2000:
                cpu.w_usp(sp + 4)
            sp = supervisor_sp - (8 if flags else 6)
            cpu.w_sp(sp)
            mem.w16(sp, sr)
            mem.w32(sp + 2, ret)
            if flags:
                mem.w16(sp + 6, 0)
            cpu.w_pc(cpu.r_reg(Register.A5))

        mem.w16(EXEC - 30, 0xa000 | machine.traps.alloc(supervisor))
        injected = []

        def inject(op, pc):
            fmt, size, vector, bpe = fault
            assert cpu.r_sr() & 0x700 == 0x700
            sp = cpu.r_sp() - size
            # Garbage in the rest of the frame detects fixed-size pops.
            mem.w_block(sp, bytes([0x5a]) * size)
            if flags:
                mem.w16(sp, cpu.r_sr())
                mem.w32(sp + 2, pc)
                mem.w16(sp + 6, (fmt << 12) | vector)
                if fmt == 4:
                    mem.w32(sp + 8, TARGET)
                    mem.w32(sp + 12, 0x01000020 | bpe)
            else:
                mem.w16(sp, 0x15)
                mem.w32(sp + 2, TARGET)
                mem.w16(sp + 6, 0x1214)
                mem.w16(sp + 8, cpu.r_sr())
                mem.w32(sp + 10, pc + 2)
            cpu.w_sp(sp)
            cpu.w_pc(mem.r32(vbr + vector))
            injected.append(pc)

        if fault:
            read = symbols["probe_byte"] if width == 1 else symbols["probe_byte"] - 4
            assert mem.r16(read) == (0x1214 if width == 1 else 0x2214)
            mem.w16(read, 0xa000 | machine.traps.alloc(inject))

        saved = {reg: 0x11220000 + int(reg) for reg in
                 [Register.D2, Register.D3, Register.D4, Register.D5,
                  Register.D6, Register.D7, Register.A2, Register.A3,
                  Register.A4, Register.A5, Register.A6]}
        for reg, value in saved.items():
            cpu.w_reg(reg, value)
        cpu.w_sr(initial_sr)
        cpu.w_sp(SSP if initial_sr & 0x2000 else USP)
        cpu.w_isp(SSP)
        cpu.w_usp(USP)
        start_sp = cpu.r_sp()
        mem.w32(start_sp, END)
        mem.w32(start_sp + 4, TARGET)
        mem.w32(start_sp + 8, OUTPUT)
        cpu.w_pc(symbols["_berr_probe_byte" if width == 1 else "_berr_probe_long"])
        result = machine.execute(100000)
        assert result.result is end, (cpu_type, flags, fault, result)
        assert cpu.r_reg(Register.D0) == (0xffffffff if fault else 0), (
            cpu_type, flags, vbr, width, fault, initial_sr,
            hex(cpu.r_reg(Register.D0)))
        expected = 0xcafebabe if fault else (0x87febabe if width == 1 else 0x87654321)
        assert mem.r32(OUTPUT) == expected
        assert cpu.r_sp() == start_sp + 4
        assert cpu.r_sr() & 0x2700 == initial_sr & 0x2700
        for reg, value in saved.items():
            assert cpu.r_reg(reg) == value, reg
        for base in {0, vbr}:
            assert mem.r32(base + 8) == 0xdead0008
            assert mem.r32(base + 12) == 0xdead000c
        assert bool(injected) == bool(fault)
    finally:
        machine.cleanup()


def check_identify(code, symbols, cpu_type, flags, vbr, signature, fault):
    machine = Machine(cpu_type, 16384)
    try:
        cpu, mem = machine.cpu, machine.mem
        mem.w_block(BASE, code)
        mem.w16(EXEC + 296, flags)
        cpu.w_reg(Register.A6, EXEC)
        cpu.w_reg(Register.VBR, vbr)
        for base in {0, vbr}:
            mem.w32(base + 8, 0xdead0008)
            mem.w32(base + 12, 0xdead000c)
        mem.w8(0xdd0044, 0x0f if signature == 770 else 0xff)
        mem.w8(0xdd0045, 0x20 if signature == 770 else 0xff)
        mem.w8(0xdd0061, 0x10 if signature == 710 else 0xff)
        end = machine.create_execute_end("returned")
        mem.w16(END, 0xa000 | machine.traps.alloc(lambda op, pc: end))
        injected = []

        def inject(op, pc):
            fmt, size, vector = fault
            assert cpu.r_sr() & 0x700 == 0x700
            sp = cpu.r_sp() - size
            mem.w_block(sp, bytes([0x5a]) * size)
            mem.w16(sp, cpu.r_sr())
            mem.w32(sp + 2, pc)
            mem.w16(sp + 6, (fmt << 12) | vector)
            if fmt == 4:
                mem.w32(sp + 12, 0x01000020)
            cpu.w_sp(sp)
            cpu.w_pc(mem.r32(vbr + vector))
            injected.append(pc)

        if fault:
            # Fault the first signature read, as an absent controller would.
            pc = BASE
            while pc < BASE + len(code):
                length, instruction = cpu.disassemble(pc)
                if instruction.startswith("move.b") and "A1" in instruction:
                    break
                pc += length
            assert pc < BASE + len(code)
            mem.w16(pc, 0xa000 | machine.traps.alloc(inject))
        cpu.w_sr(0x2000)
        cpu.w_sp(SSP - 8)
        mem.w16(SSP - 8, 0x2000)
        mem.w32(SSP - 6, END)
        mem.w16(SSP - 2, 0)
        cpu.w_pc(BASE)
        result = machine.execute(100000)
        assert result.result is end, result
        assert cpu.r_reg(Register.D0) == (0 if fault else int(signature != 0))
        assert cpu.r_sp() == SSP
        assert cpu.r_sr() & 0x2700 == 0x2000
        for base in {0, vbr}:
            assert mem.r32(base + 8) == 0xdead0008
            assert mem.r32(base + 12) == 0xdead000c
        assert bool(injected) == bool(fault)
    finally:
        machine.cleanup()


def main():
    count = 0
    with tempfile.TemporaryDirectory(prefix="xsysinfo-berr-") as tmp:
        code, symbols = assemble(ROOT / "src/berr_trap.S", Path(tmp) / "probe")
        for cpu, flags, frames in CPUS:
            for vbr in ([0, 0x6000] if flags else [0]):
                for width in (1, 4):
                    faults = [None] + [(fmt, size, vector, bpe)
                                      for fmt, size in frames
                                      for vector in (8, 12)
                                      for bpe in ([0, 4] if fmt == 4 and vector == 8 else [0])]
                    for fault in faults:
                        for sr in (0, 0x2000, 0x2700):
                            check(code, symbols, cpu, flags, vbr, width, fault, sr)
                            count += 1
        print(f"PASS: {count} guard cases (CPU frames, VBR, width, SR, output and registers)")
        # Compile the actual submodule routine, without the rest of the library.
        source = (ROOT / "3rdparty/identify/src/identify/ID_Hardware.s").read_text()
        routine = source[source.index(".getncr\t"):source.index(".getramsey\t")]
        fixture = Path(tmp) / "identify.s"
        fixture.write_text('        include "exec/execbase.i"\n'
                           '        section code,code\n'
                           '        machine 68000\nidentify_probe:\n' + routine)
        code, symbols = assemble(fixture, Path(tmp) / "identify")
        count = 0
        for cpu, flags, frames in CPUS[1:]:
            for vbr in (0, 0x6000):
                for signature in (0, 710, 770):
                    for fault in [None] + [(fmt, size, vector)
                                          for fmt, size in frames
                                          for vector in (8, 12)]:
                        check_identify(code, symbols, cpu, flags, vbr, signature, fault)
                        count += 1
        print(f"PASS: {count} identify cases (NCR signatures, faults and vector restoration)")


if __name__ == "__main__":
    main()
