# `syssnap`
This is the repo of `syssnap`, a header-only C++ library for getting snapshots of processes in Linux.

## Description
`syssnap` is a header-only C++ library for getting snapshots of the processes on Linux.
It is based on the `procfs` filesystem, to be parsed with [`prox`](https://github.com/ruben-laso/prox).
This snapshots can be used to get information about the processes, but also to migrate then to other nodes in a NUMA system.
Everytime you make some "virtual" changes to the processes mapping, don't forget to "commit" them so the system applies them.

## Installation
`syssnap` is a header-only library, so installing should be fairly easy (make sure you have the dependencies installed).
You can simply copy the header files to your project directory and include them in your source code.

Or, you can install it system-wide by running the following commands:
```bash
mkdir build
cd build
cmake ..
sudo cmake --build . --target install
```

### Dependencies
`syssnap` depends on the following libraries:
- [`prox`](https://github.com/ruben-laso/prox)
- [`range-v3`](https://github.com/ericniebler/range-v3)
- [`fmt`](https://github.com/fmtlib/fmt)
- [`libnuma`](https://man7.org/linux/man-pages/man3/numa.3.html)

Additionally, `syssnap` example depends on the following libraries:
- [`spdlog`](https://github.com/gabime/spdlog)
- [`CLI11`](https://github.com/CLIUtils/CLI11)

For testing, `syssnap` depends on the following libraries:
- [Google Test](https://github.com/google/googletest)

## Usage
Wiki is WIP. For now, you can check the [example](example) directory for examples.

## Contributing
Anyone is welcome to contribute to this project, just behave yourself :smiley:.

Please use the `clang-format` and `clang-tidy` tools before committing.

## License
This project is licensed under the GNU GPLv3 license. See [LICENSE](LICENSE) for more details.

## Contact
If you have any questions, feel free to send me an [email](mailto:laso@par.tuwien.ac.at).