# discovery
A Gameboy Advance emulator written in C++

![discovery](assets/discovery.png)

## Preamble
When I was 10 years old, I vividly remember watching people on YouTube play Gameboy games on their PC, using a mysterious program called an 'emulator'. Although I didn't have the technical knowhow at the time to install one myself, this experience sparked my curiosity about computer systems. While I learned more and more about how computers worked, the technical details of how a machine could emulate another machine remained unknown to me. I resolved very early in my programming career that I would eventually build an emulator myself. This project represents over a decade of *Discovery*.

![kirby](assets/kirby_gameplay.png)

# WARNING
This emulator is a work in progress. It probably won't work very well in its current state. There are currently many unresolved bugs. Please contact me with any questions.

## Usage
Discovery currently requires a GBA BIOS ROM to operate (I am currently working on implementing all of the BIOS calls using high level emulation to cease this requirement). I cannot provide a BIOS ROM for you. DuckDuckGo is your friend here.

To use discovery from the command line:

`./discovery path/to/rom`

Discovery will attempt to load a BIOS in the same directory as the executable called `gba_bios.bin`. To specify a BIOS of your choice:

`./discovery path/to/rom -b path/to/bios`

### Flags

| Flag | | Type | Description|
|----|----|----|----|
`-i` | `--input` | `string` | Specify the input ROM
`-b` | `--bios` | `string` | Specify the BIOS Discovery will use
`-c` | `--config` | `string` | Specify the config file Discovery will use
`-h` | `--help` | `boolean` | Print Discovery help

## Configuration

By default Discovery will load and read settings from a file named `discovery.config`. Alternatively, you can load from a different config file
using: 

`./discovery -c my_alt_config.file`

The syntax for a config file is very simple. Key-value pairs are set like so:

```
# this is a comment
set_key = values_like_this
```

Below is a table describing the keys Discovery will recognize.

### Configuration options

|Key | Description|
|----|----|
`gba_a         `  | Map the A button to the specified key
`gba_b         `  | Map the B button to the specified key
`gba_sel       `  | Map the Select button to the specified key
`gba_start     `  | Map the Start button to the specified key
`gba_dpad_right`  | Map right on the dpad to the specified key
`gba_dpad_left `  | Map left on the dpad to the specified key
`gba_dpad_up   `  | Map up on the dpad to the specified key
`gba_dpad_down `  | Map down on the dpad the specified key
`gba_r         `  | Map the R button to the specified key
`gba_l         `  | Map the L button to the specified key

#### Example

This example config file will map controls to WASD keys, using the k, l, o, and p 
keys for the A, B, L and R buttons, respectively:

```
# map buttons to WASD controls
gba_dpad_up = w 
gba_dpad_down = s
gba_dpad_left = a
gba_dpad_right = d
gba_a = k
gba_b = l
gba_r = o
gba_l = p
gba_start = cr
gba_sel = bs
```

## Building on Linux based systems
Discovery has the following dependencies:
- make
- [SDL](https://www.libsdl.org)
- [fmt](https://www.github.com/fmtlib/fmt)

If fmt is already installed on your system, simply run `make` to build.

Otherwise, follow the instructions from fmt to install it.

Coming soon:
- cmake build process to automatically install fmt
- C++20 std::format support to make fmt library obsolete 

## Building on MacOS & Windows
As the only dependencies are make & SDL, discovery should be just as easy to build on non-Linux systems. However, I do not develop for these systems, so you're on your own there ;)

## License
Discovery is licensed under the GPL (version 2). See LICENSE for full license text.

## Legal
Gameboy & Gameboy Advance are copyrights of Nintendo. No copyright infringement was intended in the creation of this project. All information and system specification herein was obtained from sources available to the public. I do not advocate for the possesion of unlicensed copyrighted files in the form of ROM.

## Acknowledgement
As with all GBA homebrew projects, the following documents were indispensible:

[GBATEK - GBA/NDS Technical Info](https://problemkaputt.de/gbatek.htm)

[Tonc's Homebrew Tutorials](https://www.coranac.com/tonc/text/toc.htm)

[ARM7TDMI Reference Sheet](https://www.dwedit.org/files/ARM7TDMI.pdf)

I also referenced other open source emulators for particularly tricky sections:

Shonumi's [GBE+](https://github.com/shonumi/gbe-plus/)

Fleroviux's [NanoBoyAdvance](https://github.com/fleroviux/NanoBoyAdvance)

Thank you for sharing the wisdom!
