## Definition
A cross-compiler is a compiler that runs on platform A (the host) but generates executables for platform B (the target).

## Setup

### Clone Binutils, GCC and GDB sources

```sh
git clone git://sourceware.org/git/binutils-gdb.git

git clone https://gcc.gnu.org/git/gcc.git

git clone https://sourceware.org/git/binutils-gdb.git
```

### Mac M1

To set up a cross-compiler for the ARM64 architecture on a Mac M1, you can use the following environment variables:

```sh
export TARGET=aarch64-elf
export PREFIX="$HOME/opt/cross"
export PATH="$PREFIX/bin:$PATH"
```

### Brew install dependencies

```sh
brew install gmp mpfr libmpc texinfo
```

### Build Binutils

This compiles the binutils (assembler, disassembler, and various other useful stuff), runnable on your system but handling code in the format specified by $TARGET.

```sh
mkdir build-binutils
cd build-binutils
../binutils-gdb/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror --with-gmp=/opt/homebrew --with-mpfr=/opt/homebrew  --with-mpc=/opt/homebrew --with-texinfo=/opt/homebrew
make
make install
```

### Build GCC

we are just building the compiler and libgcc here. We are not building and installing any standard C library (like newlib or glibc) for the target platform. This means that the cross-compiler built here will be able to compile freestanding programs (programs that do not depend on any underlying OS or C library). 

```sh
mkdir build-gcc
cd build-gcc
../gcc/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --with-gmp=/opt/homebrew --with-mpfr=/opt/homebrew  --with-mpc=/opt/homebrew --with-texinfo=/opt/homebrew
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
```

We build libgcc, a low level support library that the compiler expects available at compile time. Linking against libgcc provides integer, floating point, decimal, stack unwinding (useful for exception handling) and other support functions.

Note we are not running make install for the entire GCC, just for the compiler and libgcc.

## Build GDB

In case you want to debug your programs, you can also build GDB as a cross-debugger:

```sh
mkdir build-gdb
cd build-gdb
../gdb.x.y.z/configure --target=$TARGET --prefix="$PREFIX" --disable-werror
make all-gdb
make install-gdb
```

Same as before, we are only building and installing GDB itself, not any target libraries or headers.

## Verify Installation

```sh
$HOME/opt/cross/bin/aarch64-elf-gcc --version
$HOME/opt/cross/bin/aarch64-elf-ld --version
$HOME/opt/cross/bin/aarch64-elf-gdb --version
```