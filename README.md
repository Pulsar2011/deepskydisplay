# DeepSkyDisplay

The DSTdisplay library of the DeepSky Project: Qt display of FITS images, data cubes, tables and colour composites, built on QCustomPlot.

## Building

Requires Qt6 and [DeepSkyFits](../deepskyfits) (DSTfits) 1.5 or later, installed under `DeepSkyROOT`.

```bash
git submodule update --init --recursive
cmake .
make -j
make install
```

## License

Unlike the other DeepSky libraries, DeepSkyDisplay is licensed under the [GNU General Public License v3 or later](LICENSES/GPL-3.0.txt), because it ships [QCustomPlot](https://www.qcustomplot.com) (`include/DSTdisplay/QtDisplay.h`, `src/QtDysplay.cxx`), which its author Emanuel Eichhammer distributes under the GPL or a separate commercial licence. See [LICENSE.md](LICENSE.md).

Copyright (c) 2026 William Gillard, except QCustomPlot.
