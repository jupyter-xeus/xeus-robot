# xeus-robot

`xeus-robot` is a Jupyter kernel for [Robot Framework](https://robotframework.org/) based on the native implementation of the Jupyter protocol [xeus](https://github.com/jupyter-xeus/xeus).

[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/jupyter-xeus/xeus-robot/master?urlpath=/lab/tree/notebooks/xrobot.ipynb)

## Installation

### Installing from source

You can install `xeus-robot` from the sources, you first need to install its dependencies:

```bash
conda install -c conda-forge xeus-python xtl cmake cppzmq nlohmann_json pybind11 pybind11_json robotframework robotframework-interpreter
```

Then you can compile the sources:

```bash
cmake -D CMAKE_INSTALL_PREFIX=$CONDA_PREFIX .
make install -j6
```

## Dependencies

``xeus-robot`` depends on

 - [xeus-python](https://github.com/jupyter-xeus/xeus-python)
 - [xtl](https://github.com/xtensor-stack/xtl)
 - [pybind11](https://github.com/pybind/pybind11)
 - [pybind11_json](https://github.com/pybind/pybind11_json)
 - [nlohmann_json](https://github.com/nlohmann/json)
 - [robotframework-interpreter](https://github.com/jupyter-xeus/robotframework-interpreter)
 - [robotframework-lsp](https://github.com/robocorp/robotframework-lsp)

 
| `xeus-robot`|  `xeus-python`  |      `xtl`      | `cppzmq` | `nlohmann_json` | `pybind11`     | `pybind11_json`   | `robotframework-interpreter` | `robotframework-lsp` |
|-------------|-----------------|-----------------|----------|-----------------|----------------|-------------------|------------------------------|----------------------|
|   master    |  >=0.9.1,<0.10  |  >=0.6.8,<0.7   |  ~4.7.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.0.1,<0.1                |   >=0.4.2,<0.5       |
|   0.0.3     |  >=0.8.7,<0.9   |  >=0.6.8,<0.7   |  ~4.7.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.0.1,<0.1                |   >=0.4.2,<0.5       |
|   0.0.2     |  >=0.8.5,<0.9   |  >=0.6.8,<0.7   |  ~4.4.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.0.1,<0.1                |   >=0.4.2,<0.5       |
|   0.0.1     |  >=0.8.5,<0.9   |  >=0.6.8,<0.7   |  ~4.4.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.0.1,<0.1                |   >=0.4.2,<0.5       |

