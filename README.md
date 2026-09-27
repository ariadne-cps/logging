

# Ariadne Logging

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0) [![Unix Status](https://github.com/ariadne-cps/logging/workflows/Unix/badge.svg)](https://github.com/ariadne-cps/logging/actions/workflows/unix.yml)
[![Windows Status](https://github.com/ariadne-cps/logging/workflows/Windows/badge.svg)](https://github.com/ariadne-cps/logging/actions/workflows/windows.yml) [![Coverage Status](https://github.com/ariadne-cps/logging/workflows/Coverage/badge.svg)](https://github.com/ariadne-cps/logging/actions/workflows/coverage.yml) [![codecov](https://codecov.io/gh/ariadne-cps/logging/branch/main/graph/badge.svg)](https://codecov.io/gh/ariadne-cps/logging)

Logging is a library for concurrent logging.
It features the following:
1) Print from different threads with no overlapping of the output
2) Support for automatic registration/deregistration of threads (with example Thread implementation provided in the tests)
3) Set relative indentation on all logger calls, even on free functions
4) Set runtime verbosity to efficiently filter out unnecessarily detailed calls
5) Themes for highlighting keywords (and ability to add custom keywords)
6) Different output schedulers to offer different levels of guarantee on output order
7) Support for holding text on the bottom line, useful for progress indicators (provided in the library) and similar displays
8) A lot of configuration options for optionally printing entry/exit functions, thread identifiers, etc.

## Build

Clone the repository together with its Git submodules:

```bash
git clone --recurse-submodules https://github.com/ariadne-cps/logging.git
cd logging
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
ctest --output-on-failure
```

A C++20 compiler and CMake are required.

## Coverage

Configure a separate Debug build with coverage enabled:

```bash
mkdir build-coverage
cd build-coverage
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON
cmake --build . --parallel --target coverage
```

On Ubuntu coverage is generated with GCC/lcov. On macOS it is generated with AppleClang/LLVM coverage tools.

## Contribution guidelines ##

If you would like to contribute to Logging, please contact the developer: 

* Luca Geretti <luca.geretti@univr.it>
