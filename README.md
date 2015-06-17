# mmspa

Multimodal shortest path algorithms


## Dependencies

- libpq

## Installation

### Mac OS X

```bash
./configure
make && sudo make install
```

### Ubuntu

Please ensure the automake on your system is >= 1.15.

```bash
./configure CFLAGS=-I/usr/include/postgresql CC=c99
make && sudo make install
```
