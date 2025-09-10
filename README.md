# SCells
Modular status bar for dwm written in C, based on
[dwmblocks](https://github.com/torrinfail/dwmblocks).

## Usage
First, check `config.def.h` and change it according to your needs.

Then, compile the tool using `make [-j] && make install`.

After that, you can place the program in your `.xinitrc` or some other startup
script to have it start along with dwm.

The program may also run without X11, and this will make its output go to the
standard one instead.

## Customizing Cells
The status bar is made from the text output of CLI programs. Those are then 
called "cells".

Cells and the delimiter for each of them are defined by the file `config.h`.
By default, the header file is created on the first time you run make, which
copies the default config from `config.def.h`.

This is so you can edit your status bar commands and they won't get overwritten
in some future update.

## Contributing
Feel free to open pull requests and add patches to this tool! A new file will
be created containing a list of those :)
