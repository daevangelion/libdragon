<p align="center">
  <img src="https://github.com/user-attachments/assets/74c67b9b-ddb7-4527-bc61-86d244205c65#gh-dark-mode-only"
       id="gh-dark-mode-only"
       width="360">
  <img src="https://github.com/user-attachments/assets/02586355-e89e-4aac-a208-5ae465287bd7#gh-light-mode-only"
       id="gh-light-mode-only"
       width="360">
</p>

<p align="center">
  <strong>A modern open-source SDK for Nintendo 64 development.</strong>
</p>

<p align="center">
  <a href="https://github.com/DragonMinded/libdragon/actions/workflows/build-toolchain-library-and-roms.yml">
    <img src="https://github.com/DragonMinded/libdragon/actions/workflows/build-toolchain-library-and-roms.yml/badge.svg?branch=trunk"
         alt="Build">
  </a>
</p>

> [!TIP]
> Coming back here after a while? Check the [ChangeLog](https://github.com/DragonMinded/libdragon/wiki/Stable-branch--Changelog) of our stable branch, or the [Preview branch](https://github.com/DragonMinded/libdragon/wiki/Preview-branch).

Libdragon aims to provide a complete Nintendo 64 development experience, combining modern development tools, high-level libraries, and low-level access to the hardware.

## Features at a glance

<table>
<tr>
<td width="33%" valign="top">

<h3>🎨&nbsp; Graphics &amp; 3D</h3>

<a href="https://libdragon.dev/ref/group__rdpq.html">RDPQ</a> • <a href="https://libdragon.dev/ref/group__rdpq.html">Sprites</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Mkfont">Text & Fonts</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Mkfont">TrueType</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/OpenGL-on-N64">OpenGL 1.1</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Magma-%5BWIP%5D">Magma 3D</a> •
glTF Models

</td>
<td width="33%" valign="top">

<h3>🔊&nbsp; Audio &amp; Music</h3>

<a href="https://libdragon.dev/ref/group__mixer.html">RSP Mixer</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Audio-playback">Streaming</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Audioconv64">VADPCM</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/ULC-Audio-Codec">ULC</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Opus-decompression">Opus</a> • <a href="https://libdragon.dev/ref/xm64_8h.html">XM modules</a> • <a href="https://libdragon.dev/ref/ym64_8h.html">YM modules</a> •
MIDI + SF2

</td>
<td width="33%" valign="top">

<h3>🎬&nbsp; Video</h3>

<a href="https://github.com/DragonMinded/libdragon/wiki/MPEG1-Player">MPEG-1</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Videoconv64">H.264</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Videoconv64">YUV</a> •
Subtitles •
Seeking • <a href="https://github.com/DragonMinded/libdragon/wiki/MPEG1-Player">Render-to-texture</a>

</td>
</tr>

<tr>
<td width="33%" valign="top">

<h3>🎮&nbsp; Input &amp; Peripherals</h3>

<a href="https://libdragon.dev/ref/group__joypad.html">Joypads</a> • <a href="https://libdragon.dev/ref/group__joypad.html">GameCube controllers</a> • <a href="https://libdragon.dev/ref/group__joypad.html">Mouse</a> • <a href="https://libdragon.dev/ref/group__controllerpak.html">Controller Pak</a> • <a href="https://libdragon.dev/ref/group__joypad.html">Rumble Pak</a> • <a href="https://libdragon.dev/ref/group__joypad.html">Transfer Pak</a> • <a href="https://libdragon.dev/ref/group__joypad.html">Bio Sensor</a> • <a href="https://libdragon.dev/ref/group__rtc.html">RTC</a>

</td>
<td width="33%" valign="top">

<h3>💾&nbsp; Storage &amp; Filesystems</h3>

<a href="https://libdragon.dev/ref/group__dfs.html">DragonFS</a> • <a href="https://libdragon.dev/ref/group__dfs.html">C/POSIX API</a> •
SD cards • <a href="https://libdragon.dev/ref/eeprom_8h.html">EEPROM</a> •
SRAM •
FlashRAM •
iQue NAND/BBFS

</td>
<td width="33%" valign="top">

<h3>🚀&nbsp; Runtime &amp; Boot</h3>

64-bit ABI •
C11 Runtime • <a href="https://libdragon.dev/ref/n64sys_8h.html">iQue support</a> •
Open-source IPL3 •
Region free •
Coroutines • <a href="https://github.com/DragonMinded/libdragon/wiki/DSO-%28dynamic-libraries%29">DSO/Overlays</a>

</td>
</tr>

<tr>
<td width="33%" valign="top">

<h3>⚙️&nbsp; Low-level Hardware</h3>

<a href="https://libdragon.dev/ref/rspq_8h.html">RSP / RSPQ</a> • <a href="https://libdragon.dev/ref/group__rdpq.html">RDP / RDPQ</a> • <a href="https://libdragon.dev/ref/rsp_8h.html">Custom microcode</a> • <a href="https://libdragon.dev/ref/dma_8h.html">PI DMA queue</a> • <a href="https://libdragon.dev/ref/interrupt_8h.html">Interrupts</a> • <a href="https://libdragon.dev/ref/timer_8h.html">Timers</a> • <a href="https://libdragon.dev/ref/exception_8h.html">Exceptions</a>

</td>
<td width="33%" valign="top">

<h3>🛠️&nbsp; Toolchain &amp; Asset Pipeline</h3>

<a href="https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon">GCC 16</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon">Docker</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Mksprite">PNG</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Mkfont">TTF/OTF</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/OpenGL-on-N64">glTF</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Audioconv64">Audio conversion</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Videoconv64">Video conversion</a> • <a href="https://github.com/DragonMinded/libdragon/wiki/Compression">Compression</a>

</td>
<td width="33%" valign="top">

<h3>🐛&nbsp; Debugging &amp; Profiling</h3>

<a href="https://libdragon.dev/ref/group__debug.html">Crash screen</a> • <a href="https://libdragon.dev/ref/group__backtrace.html">Symbolized backtraces</a> • <a href="https://libdragon.dev/ref/group__debug.html">Assertions</a> • <a href="https://libdragon.dev/ref/debug_8h.html">USB logging</a> •
CPU profiler • <a href="https://libdragon.dev/ref/rdpq__debug_8h.html">RDP validation</a> • <a href="https://libdragon.dev/ref/rsp_8h.html">RSP crash diagnostics</a>

</td>
</tr>
</table>

## Getting started: how to build a ROM

To get started with libdragon, you need to [download and install the toolchain](https://github.com/DragonMinded/libdragon/releases/tag/toolchain-continuous-prerelease).

Make sure to read the [full installation instructions](https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon) which also explain the system requirements.

## Getting started: how to run a ROM

### Using emulators

libdragon targets real N64 hardware and uses many advanced corners
of the hardware not used by old commercial games, and thus requires
a modern N64 emulator which focuses on full hardware emulation.

At the moment, the only emulators that accurately emulate the hardware
(and does not just focus on playing old classics) are:
* [Ares](https://github.com/ares-emulator/ares)
* [Gopher64](https://github.com/gopher64/gopher64).

Both require a modern PC with a discrete GPU with Vulkan support. Gopher64
is more performant for gaming.

Ares has more development-oriented features so it is suggested during development.
You can develop 99% of your game using libdragon and the Ares emulator,
and be confident that the game will correctly run on hardware as well.
Make sure to turn on the "Homebrew mode" in Ares to enable developer
specific checks during emulation that will simplify the debugging experience.

### Using a development cartridge on a real N64

All cartridges that are able to load custom ROMs should be able to successfully
load libdragon ROMs via either USB/serial, or from a MMC/SD card. For instance,
the following are known to work: 64drive, EverDrive64 (all models), SC64.

If your cartridge has USB support, use one of the loaders that implement the
libdragon debugging protocol, so to be able to show logs in console. For instance,
[UNFLoader](https://github.com/buu342/N64-UNFLoader), [g64drive](https://github.com/rasky/g64drive),
[ed64](https://github.com/anacierdem/ed64). The official loaders provided by
the vendors are usually less feature-rich.

## Libdragon stable vs preview

There is a single libdragon version, but not every API in it carries the same
promise. Each API is either **stable** or **preview**:

 * A **stable** API will not break backward compatibility. We will never change
   it in a way that impedes existing applications from compiling and working
   against a newer libdragon version. We feel this is important because
   otherwise we would fragment the homebrew ecosystem too much, and we would
   leave a trail of libdragon-based applications that can't be compiled anymore.
 * A **preview** API is still being developed, evolved and battle-tested. It can
   change or be removed at any time, without notice and without a deprecation
   period (though we try to avoid *gratuitous* breakage). This is where most new
   features start their life, before they are stabilized.

Preview APIs are documented as such (look for the "Preview API" note in the
[documentation](https://libdragon.dev/ref/index.html)), and they are locked by
default: using one is a compile time error that names the API you tried to use.
To unlock them, set `LIBDRAGON_PREVIEW` in your Makefile, *before* including
`n64.mk`:

```makefile
LIBDRAGON_PREVIEW = 1          # allow preview APIs, but warn on each use
# LIBDRAGON_PREVIEW = 2        # fully unlock preview APIs (no diagnostics)
include $(N64_INST)/include/n64.mk
```

Accepted values: `0` (default, hard error), `1` (usable with a compiler
warning), `2` (fully unlocked).

The switch is a property of your project, not of your libdragon installation:
the same installed toolchain builds both stable-only and preview-using projects,
and you can flip it at any time.

There is no separate build of libdragon for preview APIs, so enabling the switch
never changes the behaviour or the memory layout of the stable APIs: it only
changes which APIs you are allowed to call.

When a preview API is considered settled, it is promoted to stable and the
marking is simply removed. Nothing needs to change in your project, and you can
drop `LIBDRAGON_PREVIEW` once you no longer use any preview API.

## Opt-in TLB code units

This fork supports explicit code residency with `N64_TLBCODE=1`. Leave the
option unset (or set it to `0`) to use the original linker script and boot path.
The same installed SDK supports both modes; use separate build directories
when switching modes, since compiler options are not make dependencies.

Set `N64_TLBCODE_UNITS=units.json` before including `n64.mk`. The JSON object
maps names to lists of linker input patterns: object patterns such as
`"*h264_decoder.o"` select text and rodata, while section patterns such as
`".text.boot_setup"` select individual explicitly annotated functions.
Unit data, small data, and BSS remain resident. See `examples/tlbcode` for
`fmv` and one-shot `init` memberships and `tests/tlbcode-units.json` for a
section-based unit.

All normal text uses KSEG2 starting at `0xC0001000`; a small KSEG0 boot path
installs wired 1 MiB-page pairs before entering it. Units occupy distinct,
fixed 1 MiB windows starting at `0xC1000000`. Unit sections are not PT_LOAD.
The installed `tlbcode` Python 3 tool generates the linker fragments, extracts
the page-aligned unit blob, and binds it to a SHA-256 resident ELF build ID.
The ROM recipe bundles that blob as `.tlbcode`; the deployment compressor
uses KSEG0 load addresses while the original ELF keeps virtual debug symbols.
The build also emits `<elf>.tlbcode.refs`, listing resident section/symbol
references into units. Review it for unintended COMDAT/extern-inline copies
and callbacks that outlive a load interval. Intentional post-load calls and
resident function-pointer tables are allowed. Constructor and destructor
tables (`.ctors`, `.dtors`, `.preinit_array`, `.init_array`, `.fini_array`)
must not target units: packing rejects such targets before a ROM is emitted.
Unit sizes above 62 pages are rejected by the linker and packer.

Call `tlbcode_init()`, find a unit with `tlbcode_find()`, and load it using
`tlbcode_load(unit, frames, unit->page_count)`. The caller supplies distinct,
4 KiB aligned cached RDRAM frames (not necessarily contiguous); both pages and
the frame-pointer array remain borrowed until `tlbcode_unload()`. No runtime
heap allocation or demand paging occurs. Each pair consumes one of the 32 TLB
slots after the wired resident entries. Oversubscribing these slots fails.
Loading uses the PI DMA queue and explicit cache maintenance.

Serialize load/unload in application code. Before unloading, return out of
the unit and quiesce every callback, thread, deferred RSP/RDP command, and
interrupt that could enter it. Do not DMA unit rodata directly to the RCP:
ordinary `PhysicalAddr` cannot translate these noncontiguous mappings; keep
RCP-visible assets resident. This feature does not page data or translate
arbitrary pointers for DMA. Debugger PC checks and symbols understand unit
ranges, and accesses to an unloaded unit report its name.
DSOs cannot safely relocate J26 calls from KSEG0 heap code to mapped code;
`dlopen` therefore returns NULL with an explicit error in mapped executables.
Ordinary ROMs retain the existing DSO behavior.

Build the example with `make -C examples/tlbcode N64_TLBCODE=1 CLIP=/path/to/clip.h264`,
where the clip is encoded with `videoconv64 --codec h264`. Its frame callback
stops after 24 frames, unloads the FMV unit, then reallocates and touches the
released memory. Emulator load timings are not hardware measurements: ares
models PI at fixed speed and has no micro-TLB.

## Upgrading libdragon

Check the [ChangeLog](https://github.com/DragonMinded/libdragon/wiki/Stable-branch--Changelog)
in the wiki to see the latest changes to the stable APIs.
Also check the wiki page for [common hurdles in upgrading libdragon](https://github.com/DragonMinded/libdragon/wiki/Upgrade-troubleshooting).

If your project enables `LIBDRAGON_PREVIEW`, instead, remember that some
breaking changes are expected. We do not keep track of those though, so you will
have to check the relevant header files yourself to see what has changed.

## Resources

 * [API reference](https://dragonminded.github.io/libdragon/ref/topics.html)
 * [Examples](https://github.com/DragonMinded/libdragon/tree/trunk/examples)
 * [ROM Metadata Viewer](https://dragonminded.github.io/libdragon/static/metadata.html)
 * [Wiki](https://github.com/DragonMinded/libdragon/wiki) (contains tutorials
   and troubleshooting guides)
 * [Discord n64brew](https://discord.gg/WqFgNWf)
