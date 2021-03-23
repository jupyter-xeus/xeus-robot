![xeus-robot](images/xeus-robot.svg)

[![Azure Pipelines](https://dev.azure.com/jupyter-xeus/jupyter-xeus/_apis/build/status/jupyter-xeus.xeus-robot?branchName=master)](https://dev.azure.com/jupyter-xeus/jupyter-xeus/_build/latest?definitionId=3&branchName=master)
[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/jupyter-xeus/xeus-robot/stable?urlpath=/lab/tree/notebooks/xrobot.ipynb)

`xeus-robot` is a Jupyter kernel for [Robot Framework](https://robotframework.org/) based on the native implementation of the Jupyter protocol [xeus](https://github.com/jupyter-xeus/xeus).

## Installation

### Using conda

```bash
conda install -c conda-forge xeus-robot
```

### Using pip

Depending on the platform, PyPI wheels may be available for xeus-robot.

```bash
pip install xeus-robot
```

### Installing from source

You can install `xeus-robot` from the sources, you first need to install its dependencies:

```bash
conda install -c conda-forge xeus-python xtl cmake cppzmq nlohmann_json pybind11 pybind11_json robotframework-interpreter ipywidgets jupyterlab_robotmode
```

Then you can compile the sources:

```bash
cmake -D CMAKE_INSTALL_PREFIX=$CONDA_PREFIX .
make install -j6
```

### Install the syntax highlighting and widgets for JupyterLab 1 and 2 (It is automatically installed for JupyterLab 3)

```bash
jupyter labextension install @marketsquare/jupyterlab_robotmode @jupyter-widgets/jupyterlab-manager
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
|   master    |  >=0.12.1,<0.13 |  >=0.7.0,<0.8   |  ~4.7.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.6.6,<0.7                |   >=0.4.2,<0.5       |
|   0.3.2     |  >=0.12.1,<0.13 |  >=0.7.0,<0.8   |  ~4.7.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.6.6,<0.7                |   >=0.4.2,<0.5       |
|   0.3.1     |  >=0.11.3,<0.12 |  >=0.7.0,<0.8   |  ~4.7.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.6.3,<0.7                |   >=0.4.2,<0.5       |
|   0.3.0     |  >=0.11.1,<0.12 |  >=0.7.0,<0.8   |  ~4.7.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.6.3,<0.7                |   >=0.4.2,<0.5       |
|   0.2.2     |  >=0.10.2,<0.11 |  >=0.7.0,<0.8   |  ~4.7.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.6.2,<0.7                |   >=0.4.2,<0.5       |
|   0.2.1     |  >=0.10.2,<0.11 |  >=0.7.0,<0.8   |  ~4.7.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.6.2,<0.7                |   >=0.4.2,<0.5       |
|   0.2.0     |  >=0.10.0,<0.11 |  >=0.7.0,<0.8   |  ~4.7.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.6.2,<0.7                |   >=0.4.2,<0.5       |
|   0.1.1     |  >=0.9.5,<0.10  |  >=0.6.8,<0.7   |  ~4.7.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.6.2,<0.7                |   >=0.4.2,<0.5       |
|   0.1.0     |  >=0.9.5,<0.10  |  >=0.6.8,<0.7   |  ~4.7.1  |  >=3.6.1,<4.0   | >=2.2.4,<3.0   | >=0.2.6,<0.3      |  >=0.6.1,<0.7                |   >=0.4.2,<0.5       |


## Examples

### Code completion
![Code completion](images/completion.gif)

### Code completion using Selenium selectors
![Code completion with selenium](images/completion2.gif)

### Custom RobotFramework library in Python
![Custom Python library](images/python.gif)

### Debugger support in JupyterLab 3
![Debugger](images/debugger.gif)

### Custom Keywords testing
![Test Keyword](images/keywords.gif)
