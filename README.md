
# Eclypse Z7 - Vitis Migration v2024.1

## Workspaces

* ws-Eclypse-Z7-OOB - oob/master, ...
* ws-Eclypse-Z7-ddr-streaming - ddr-streaming, ...

## Help

In some cases source files of drivers coming from platform are
taken and passed to compiler, here for ddr-streaming projects,
in `CMakeLists.txt` build file constant `BASE_PATH_DRIVERS` need
to have absolute path to `drivers` located in platform directory.
For other projects, relative paths have worked, but here not.
