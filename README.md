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
- virtual host: Simple virtual host support.
- location config: Simple location configuration mechanism based on
  URI prefix and regex.
- module architecture: An simple module architecture to easily add new
  modules
- mod_static: A module that could serve static files and support basic
  MIME mapping, cache-control, welcome files, directory listing,
  ranged request (partially).

A lot work needs to be done, but it won't stop you from trying the
features already implemented. Please refer to the following sections
for instructions on how to run it.

## Build

Run make in the `src` dir:

```bash
cd src
make
```

The target executable files will appear in the same directory. There
are several unit test cases and a main program `breeze`.

## Usage

### Run breeze based on config file

Run the following command in `src` directory:

```bash
./breeze [-c configfile] [-t]
```

If `-c` is not specified, then `/etc/breeze.conf` is used. If `-t` is
specified, the program will print the details of the config and exits.

Please refer to the following sections for details about the config
file.

### Run breeze on specified root directory

Run the following command in `src` directory:

```bash
./breeze [-r root_dir] [-p port]
```

This mode is activated with the `-r` option. When activated, `breeze`
will not read config file. Instead, breeze will setup a simple server
serving content in the `root_dir`. In this mode, the directory listing
feature is enabled. Therefor this is a perfect replacement of Python
SimpleHTTPServer. Use `-p` to specify the port, by default `8000` is
used.

### Hello world HTTP server

Run the following command in `src` directory:

```bash
./test_http_server
```

A server on port 8000 will be started. It will return a short HTML for
any request on port 8000.

## Config File

Please refer to [the sample config file](src/sample-conf.json) for an
example. Please note that some config options are not implemented yet.

## TODO

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
