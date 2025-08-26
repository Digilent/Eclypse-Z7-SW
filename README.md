
# Eclypse Z7 DDR Streaming Demos

## New Vitis workflow

This functionality can be reproduced from Vitis Unified IDE by launching the terminal from
Terminal -> New Terminal which uses the default command line executable from the OS,
or simply manually invoke the native terminal. If this is the chosen method, then it will be necessary to give absolute path to the `checkout.py`
or `checkin.py` file, not relative, or to change current working directory to the desired sw submodule branch.

> `vitis -s <path-to-scripts-repo>checkout.py`

or

> `vitis -s <path-to-scripts-repo>checkin.py`


After checkout, if you encounter build errors for the mm2s_single_transfer_test and s2mm_cyclic_transfer_test apps,
it is likely because Vitis did not correctly recognize the path to the platform drivers.
To work around this issue, open the following two files:
\src\mm2s_single_transfer_test\src\CMakeLists.txt
\src\s2mm_cyclic_transfer_test\src\CMakeLists.txt
Then uncomment and edit line 15 in each of these files with the correct path to the drivers
(e.g. <path_to_your_workspace>/design_1_wrapper/hw/sdt/drivers/ ).
Then open Vitis and rebuild the two apps. They should now build fine.