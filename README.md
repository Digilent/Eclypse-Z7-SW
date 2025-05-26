
# Eclypse Z7 SDK Workspace

## New Vitis workflow

This functionality can be reproduced from Vitis IDE by launching the terminal from
Terminal -> New Terminal which uses the default command line executable from the OS,
or simply manually invoke the native terminal. If this is the choosen method, then it will be necessary to give absolute path to the `checkout.py`
or `checkin.py` file, not relative, or to change current working directory to the wanted sw submodule branch.

> `vitis -s <path-to-scripts-repo>checkout.py`

or

> `vitis -s <path-to-scripts-repo>checkin.py`
