# Tinydhcpd

`tinydhpcd` is (going to be) a tiny DHCPv4 server, aiming for compliance with [RFC2131](https://datatracker.ietf.org/doc/html/rfc2131) and implementation of
most common DHCP options.

- [Tinydhcpd](#tinydhcpd)
  - [Getting started](#getting-started)
    - [Building](#building)
    - [Running](#running)

## Getting started
### Building
To compile and run `tinydhcpd`, you need the following:

- Linux with `epoll` support
- [libconfig](https://github.com/hyperrealm/libconfig)
  - Ubuntu: `apt install libconfig`
  - Gentoo: `emerge libconfig`  
- A compiler supporting C++20 designated initializers (GCC 8 or later, Clang 10 or later)

To compile, execute the following in the root of the repository:
```bash
$ meson <build_dir_name>
$ cd <build_dir_name>
$ meson compile
```
The executable can then be found in the specified build directory.

### Running
By default, `tinydhcpd` looks for a file called `tinydhcpd.conf` in `/etc/tinydhcpd/`. An example configuration file can be found [here](examples/example.conf).

Currently, the following commandline options are available:
```
-a, --address w.x.y.z        Listen on address w.x.y.z
-i, --interface <iface>      Bind to interface <iface>
-c, --configfile <file>      Use <file> instead of the default config file
```