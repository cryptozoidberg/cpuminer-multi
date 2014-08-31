CPUMiner-Multi
==============

This is a multi-threaded CPU miner,
fork of [pooler](//github.com/pooler)'s cpuminer.

#### Table of contents

* [Algorithms](#algorithms)
* [Dependencies](#dependencies)
* [Download](#download)
* [Build](#build)
* [Usage instructions](#usage-instructions)
* [Donations](#donations)
* [Credits](#credits)
* [License](#license)

Algorithms
==========
#### Currently supported
 * ✓ __wildkeccak__ (Boolberry [BBR])

Dependencies
============
* libcurl			http://curl.haxx.se/libcurl/
* jansson			http://www.digip.org/jansson/ (jansson is included in-tree)

Download
========
* Binary releases: http://boolberry.com/downloads.html
* Git tree:   https://github.com/cryptozoidberg/cpuminer-multi
  * Clone with `git clone https://github.com/cryptozoidberg/cpuminer-multi`

Build
=====

#### Basic *nix build instructions:
```sh
./autogen.sh	# only needed if building from git repo
./nomacro.pl	# only needed if building on Mac OS X or with Clang
./configure CFLAGS="-O3 -march=native"
# Use -march=native if building for a single machine
make
```
* Static linking is not supported.

#### Notes for AIX users:
 * To build a 64-bit binary, `export OBJECT_MODE=64`
 * GNU-style long options are not supported, but are accessible via configuration file

#### Basic Windows build instructions, using MinGW:
 * Install MinGW and the MSYS Developer Tool Kit (http://www.mingw.org/)
   * Make sure you have mstcpip.h in MinGW\include
 * If using MinGW-w64, install pthreads-w64
 * Install libcurl devel (http://curl.haxx.se/download.html)
   * Make sure you have libcurl.m4 in MinGW\share\aclocal
   * Make sure you have curl-config in MinGW\bin
 * Install openssl devel (https://www.openssl.org/related/binaries.html)
 * In the MSYS shell, run:
```sh
./autogen.sh	# only needed if building from git repo
LIBCURL="-lcurldll" ./configure CFLAGS="-O3 *-march=native*"
# Use -march=native if building for a single machine
make
```

#### Basic cross-compile instructions, compiling for win64 on Linux Fedora:
```sh
yum install mingw\*
./autogen.sh    # only needed if building from git repo
./configure CC=x86_64-w64-mingw32-gcc RANLIB=x86_64-w64-mingw32-ranlib --target x86_64-w64-mingw32 
make
```

#### Architecture-specific notes:
 * ARM:
   * No runtime CPU detection. The miner can take advantage of some instructions specific to ARMv5E and later processors, but the decision whether to use them is made at compile time, based on compiler-defined macros.
   * To use NEON instructions, add "-mfpu=neon" to CFLAGS.
 * x86-64:	
   * The miner can take advantage of SSE2 and AVX2 instructions, but only if both the CPU and the operating system support them.
   * There is no runtime check for the features, so minerd compiled on CPU with
   * AVX2 support causes Invalid opcode trap when run on CPU without AVX2 support (patches accepted).

Usage instructions
==================
```sh
./minerd -o stratum+tcp://url_to_server:7778 -u 1L1ZPC9XodC6g5BX8j8m3vcdkXPiZrVF7RcERWE879coQDWiztUbkkVZ86o43P27Udb3qxL4B41gbaGpvj3nS7DgFZauAZE  -p x -P -D -t 1 -k http://url_to_server/download/scratchpad.bin
```
Visit url_to_server to verify scratchpad.bin download location.
Run "minerd --help" to see more options.



### Connecting through a proxy



Use the --proxy option.

To use a SOCKS proxy, add a socks4:// or socks5:// prefix to the proxy host  
Protocols socks4a and socks5h, allowing remote name resolving, are also available since libcurl 7.18.0.

If no protocol is specified, the proxy is assumed to be a HTTP proxy.  
When the --proxy option is not used, the program honors the http_proxy and all_proxy environment variables.

Donations
=========

Donations for the work done in this fork by zoidberg are accepted at
* BBR: `@zoidberg`
* BTC: `1JjGoHdzP5HvtyEZ6MN9LXbh8uTDVgrPY9`

Donations for the work done in this fork by otila are accepted at
* BBR: `@anonymous`
* XMR: `46i538G1mrxAjzvP7cqKNufPoYAmmZyX5NidEcpmEjqgGB8F8vmRax3SuipJ3zkEkkFYQHjM58zDeXyoNgxVVGby4JMGwZt`
* BTC: `1Gpa1DteQozR4Mw94mjZRGb6McaNT6nrTC`

Donations for the work done in this fork by Wolf are accepted at
* BBR: `@wolf`
* XMR: `46sSETXrZGT8bupxdc2MAbLe3PMV9nJTRTE5uaFErXFz6ymyzVdH86KDb9TNoG4ny5QLELfopynWeBSMoT1M2Ga8RBkDqTH`
* BTC: `1WoLFumNUvjCgaCyjFzvFrbGfDddYrKNR`

Credits
=======
CPUMiner-multi was forked from Lucas Jones's cpuminer-multi, and has been developed by
Cryptozoidberg, otila, and Wolf.

License
=======
GPLv2.  See COPYING for details.
