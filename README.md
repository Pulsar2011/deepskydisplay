# DeepSkyDisplay

The DSTdisplay library of the DeepSky Project: Qt display of FITS images, data cubes, tables and colour composites, built on QCustomPlot.

## Building

Requires [DeepSkyFits](../deepskyfits) (DSTfits) 1.5 or later, installed under `DeepSkyROOT`, and optionally Qt6
(without it, the GUI is disabled).

Everything is built out of source: configuring in the source directory (`cmake .`) is refused.

```bash
git submodule update --init --recursive
cmake --preset default        # Release, in build/      (or: cmake -B build)
cmake --build build -j
ctest --preset default        # the tests run in build/, and what they write stays there
cmake --build build --target install
```

Other presets: `debug` (Debug, in `build-debug/`) and `asan` (Debug + AddressSanitizer, in `build-asan/`):
`cmake --preset asan && cmake --build --preset asan -j && ctest --preset asan`.

Options: `-DCMAKE_INSTALL_PREFIX=<path>` (default: `$DeepSkyROOT`, or the parent directory), `-DWITH_ASAN=ON`,
`-DBUILD_TESTS=OFF`. `-DWITH_DEBUG=ON` is deprecated: it means Debug + AddressSanitizer.

The demonstration programs of `util/` are built into `build/bin`.

Former in-source builds left files in the sources (`CMakeCache.txt`, `Makefile`, `lib/`, ...); remove them once
with `cmake -P cmake/CleanInSourceBuild.cmake`.

## License

Unlike the other DeepSky libraries, DeepSkyDisplay is licensed under the [GNU General Public License v3 or later](LICENSES/GPL-3.0.txt), because it ships [QCustomPlot](https://www.qcustomplot.com) (`include/DSTdisplay/QtDisplay.h`, `src/QtDysplay.cxx`), which its author Emanuel Eichhammer distributes under the GPL or a separate commercial licence. See [LICENSE.md](LICENSE.md).

Copyright (c) 2026 William Gillard, except QCustomPlot.
