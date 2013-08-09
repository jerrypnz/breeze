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
others are simple test servers.

For the static file module, please run `mod_static <root_dir>`. It will server
static files on port 8000.

For a hello world http server, please run `test_http_server`. It will return
a short HTML for every request on port 8000.

## TODO

- Index file support for static file module
- HTTP chunked encoding, gzip compression
- FastCGI support
- Automatically close keep-alive connection on server side
- Implement configuration file module

## Lisence

GPLv3
