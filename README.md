# Breeze

This is a simple HTTP server inspired by
[Tornado Web Server](https://github.com/facebook/tornado), for
practising my Linux programming skills. No third-party libraries are
used, only glibc.

This project is still a work-on-progress. But several features are
already implemented:

- ioloop/iostream: Async IO module inspired by Tornado
- buffer: Simple ring-buffer module
- http: Core HTTP related features like protocol parser,
  handler architecture, etc.
- mod_static: A module that could serve static files and support basic
  MIME mapping, cache-control, welcome files, directory listing,
  ranged request (partially).

A lot work needs to be done, but it won't stop you from trying the
features already implemented. There are several unit test programs
that test individual modules. A `mod_static` program is also provided
for trying static file serving. Please refer to the following sections
for instructions on how to run it.

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

### Test static file module

Run the following command in `src` directory:

```bash
./mod_static <root_dir>
```

A server on port 8000 will be started. You could visit
http://localhost:8000 to see the result.

### Hello world HTTP server

Run the following command in `src` directory:

```bash
./test_http_server
```

A server on port 8000 will be started. It will return a short HTML for
any request on port 8000.

## TODO

- Implement configuration file module
- Automatically close keep-alive connection on server side
- HTTP chunked encoding, gzip compression
- FastCGI support
- File upload module
- Master-worker multi-process architecture

## Known Bugs

- The server stops responding to any request on high load (>2000
  concurrent connections)

## Lisence

GPLv3
