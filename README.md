# Eclypse Z7 Out-of-Box Demo SDK Workspace

## Description

This is a branch of the Eclypse Z7 Software Repository containing the SDK workspace for the Baremetal Software Out-of-Box Demo. This demo is programmed into the board's flash during manufacturing.

The hardware is based on the `oob/master` branch of [Eclypse-Z7-HW](https://github.com/Digilent/Eclypse-Z7-HW).

## Requirements

* **Eclypse Z7**
* **Vivado 2019.1 Installation with Xilinx SDK**: To set up Vivado, see the [Installing Vivado and Digilent Board Files Tutorial](https://reference.digilentinc.com/vivado/installing-vivado/start).

## Setup

To gain access to this demo's sources, it is recommended to clone the [Eclypse-Z7](https://github.com/Digilent/Eclypse-Z7) repo's `oob/master` branch. Since our repositories use submodules to bring in additional sources, and to separate different domains, the `--recursive` flag should be used when cloning. The command below can be used in a console application with access to [Git](https://git-scm.com/) to gain access to all hardware and software sources for this demo:

> `git clone --recursive https://github.com/Digilent/Eclypse-Z7 -b oob/master`

For instructions on setting up the demo see the *Quick Guide* section of [digilent-vivado-scripts' README](https://github.com/Digilent/digilent-vivado-scripts/blob/5dee436ae7810e2fa098d298308567c73736a639/README.md.

## Next Steps

This demo can be used as a basis for other projects by modifying the software or hardware designs.

## Additional Notes

For more information on the Eclypse Z7, visit its [Resource Center](https://reference.digilentinc.com/reference/programmable-logic/eclypse-z7/start) on the Digilent Wiki.

For more information on how our git and project flow is set up, please refer to the [Eclypse Z7 Git Repositories](https://reference.digilentinc.com/reference/programmable-logic/eclypse-z7/git) documentation.

For technical support or questions, please post on the [Digilent Forum](forum.digilentinc.com).