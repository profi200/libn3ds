# wf-fatfs

wf-fatfs is a fork of [ChaN's FatFs](http://elm-chan.org/fsw/ff/) with adaptations useful for projects which are part of the Wonderful toolchain. More details are available at the end of `ffconf.h`.

* The `source` directory contains source code which can be used unmodified in your environment.
    * `ff.h` - FatFs header file
* The `sample` directory contains source code which should be edited or adapted to your environment:
    * `diskio.c` - Disk device I/O
    * `ffconf.h` - FatFs configuration file
    * `ffsystem.c` - System memory/locking functions

## License

The license used by wf-fatfs is identical to that of FatFs:

    /*----------------------------------------------------------------------------/
    /  FatFs - Generic FAT Filesystem Module  Rx.xx                               /
    /-----------------------------------------------------------------------------/
    /
    / Copyright (C) 20xx, ChaN, all right reserved.
    /
    / FatFs module is an open source software. Redistribution and use of FatFs in
    / source and binary forms, with or without modification, are permitted provided
    / that the following condition is met:
    /
    / 1. Redistributions of source code must retain the above copyright notice,
    /    this condition and the following disclaimer.
    /
    / This software is provided by the copyright holder and contributors "AS IS"
    / and any warranties related to this software are DISCLAIMED.
    / The copyright owner or contributors be NOT LIABLE for any damages caused
    / by use of this software.
    /----------------------------------------------------------------------------*/

In addition, special permission is granted to ChaN, the author of FatFs, to study, use and redistribute the modifications done by wf-fatfs without limitation or any further requirement of attribution.
