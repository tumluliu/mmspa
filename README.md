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

## Acknowledgements

Thanks for the support of National Natural Science Foundation of China (NSFC) project "Data model and algorithms in socially-enabled multimodal route planning service" (No. 41301431) of which I am the project leader.
