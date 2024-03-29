# tinydhcpd

`tinydhcpd` is (going to be) a tiny DHCPv4 server, aiming for compliance with [RFC2131](https://datatracker.ietf.org/doc/html/rfc2131) and implementation of
most common DHCP options.

- [Getting started](#getting-started)
  - [Building](#building)
  - [Running](#running)
- [Why another DHCP server?](#why-another-dhcp-server)
  - [Goals](#goals)

## Getting started
### Building
To compile and run `tinydhcpd`, you need the following:

- Linux with `epoll` & `close_range` support (>5.9)
- [libconfig++](https://github.com/hyperrealm/libconfig)
- [libcap](https://sites.google.com/site/fullycapable/Home)
- [Meson](https://mesonbuild.com/) & (optionally) [Ninja](https://ninja-build.org/)
- A compiler supporting C++20 designated initializers (GCC 8 or later, Clang 10 or later)

To install the necessary dependencies, you can use the following commands:
  
  Ubuntu: 
  ```
  # apt install build-essential libconfig++-dev libcap-dev meson ninja-build 
  ```
  Gentoo: 
  ```
  # emerge libconfig libcap meson ninja
  ```
  Fedora:
  ```
  # dnf install gcc-c++ libconfig-devel libcap-devel meson ninja-build
  ```


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
-a, --address <address>      Listen on address <address>
-i, --interface <iface>      Bind to interface <iface>
-c, --configfile <file>      Use <file> instead of the default config file
-d, --debug                  Change log level to DEBUG
-f, --foreground             Don't fork to background
```
## Why another DHCP server?

Most DHCP servers these days come bundled with a DNS server of some sort (e.g. `dnsmasq` and the ISC's DHCP server implementation) to allow for tight integration between DNS and IP allocation. That unfortunately also means that they are big pieces of software, which can become a problem on small embedded systems, and their complex dependencies can lead to build failures or crashes when built against a non-standard configuration (e.g. for aarch64 with musl-libc).

The inability to properly cross-compile and/or use the above programs strictly for DHCP motivated me to write my own little DHCP server, which only does DHCP and nothing else.

### Goals

- Implement a standards-conform DHCP server
- Allow for easy cross-compilation
- Explicitly support smaller/alternative libc implementations (like musl-libc)
- Ideally minimize executable size (as of July 2022, the size-optimized stripped binary clocks in at just 80kB)
