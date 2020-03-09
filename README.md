# Eclypse Z7 Zmod ADC 1410 + Zmod DAC 1411 SDK Workspace

## Description

This is a branch of the Eclypse Z7 board containing the SDK workspace with the Zmod ADC 1410 set in Zmod connector A and Zmod DAC 1411 set in Zmod connector B. The project is configured to work with the [zmodlib](https://github.com/Digilent/zmodlib) in order to showcase the use of the Zmod ADC 1410 and Zmod DAC 1411 with the Eclypse Z7. The workspace contains projects targeting both bare-metal and linux, which possess the same functionality regardless of OS platform. The functionality of the SDK demo is documented inside the code via comments.

The bare-metal hardware platform used for this Petalinux project has been imported from the [Hardware repository](https://github.com/Digilent/Eclypse-Z7-HW/tree/zmod_adc_dac/master) of the same branch as this. In order to minimize confusion, currently the commit message of the imported hardware platform contains the commit hash of the exported hardware platform of the aforementioned Hardware repository.

The Linux project works with both the Petalinux project as is and the [SD card image](https://github.com/Digilent/Eclypse-Z7/releases) provided by us.

For more details on how to use the [zmodlib](https://github.com/Digilent/zmodlib) and how to set up your environment please visit the [Zmod Base Library User Guide](https://reference.digilentinc.com/reference/zmod/zmodbaselibraryuserguide)

## First and Foremost

* Our projects use submodules to bring in libraries. Use --recursive when cloning or `git submodule init` and `git submodule update`, if cloned already non-recursively.

## Requirements

* **Eclypse Z7**
* **Zmod ADC 1410**
* **Zmod DAC 1411**
* **Vivado 2019.1 Installation with Xilinx SDK**: To set up Vivado, see the [Installing Vivado and Digilent Board Files Tutorial](https://reference.digilentinc.com/vivado/installing-vivado/start).

## Setup

For instructions on setting up the demo please visit the [Environment Setup](https://reference.digilentinc.com/reference/zmod/zmodbaselibraryuserguide#environment_setup) section of the Zmod Base Library User Guide.

## Next Steps

This demo can be used as a basis for other projects by modifying the software to take advantage of the capabilities of the provided hardware and zmodlib functions.

## Additional Notes

For more information on the Eclypse Z7, visit its [Resource Center](https://reference.digilentinc.com/reference/programmable-logic/eclypse-z7/start) on the Digilent Wiki.

For more details on how to use the [zmodlib](https://github.com/Digilent/zmodlib) and for instructions on how to set up your environment please visit the [Zmod Base Library User Guide](https://reference.digilentinc.com/reference/zmod/zmodbaselibraryuserguide)

For more information on the Zmod DAC 1411, please visit its [Resource Center](https://reference.digilentinc.com/reference/zmod/zmoddac/start) on the Digilent Wiki.

For more information on the Zmod ADC 1410, please visit its [Resource Center](https://reference.digilentinc.com/reference/zmod/zmodadc/start) on the Digilent Wiki.

For more information on how our git and project flow is set up, please refer to the [Eclypse Z7 Git Repositories](https://reference.digilentinc.com/reference/programmable-logic/eclypse-z7/git) documentation.

For technical support or questions, please post on the [Digilent Forum](https://forum.digilentinc.com/).
