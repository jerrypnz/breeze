# Breeze

This is a simple HTTP server inspired by [Tornado Web Server](https://github.com/facebook/tornado),
for practising my Linux programming kills. No third-party libraries are used, only glibc.

This project is still a work-on-progress. It is not working now, but feel free to read any code you
are interested in.

Currently you can run the `test_http_server.c` program to see a hello-world server running on
port 8000.

## Build

Run make in the `src` dir:

```bash
cd src
make
```

The target executable files will appear in the same directory.

## Usage

Currently only some test executables are provided. Some are unit tests,
others are simple test servers. For a hello world http server, please
run `test_http_server`.

## TODO

TODO

## Lisence

GPLv3
