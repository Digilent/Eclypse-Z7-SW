# Eclypse Z7 Zmod ADC 1410 SDK Workspace

## Description

This is a branch of the Eclypse Z7 board containing the SDK workspace with the Zmod ADC 1410 set in Zmod connector A. The project is configured to work with the [zmodlib](https://github.com/Digilent/zmodlib) in order to show case the the Zmod ADC 1410 usage with the Elypse Z7. The workspace has both the bare-metal and the linux project in them, both projects fulfill the same functionality regardless of OS platform. The functionality of the SDK Demo is documented inside the code via comments.

The bare-metal  hardware platform used for this Petalinux project has been imported from the [Hardware repository](https://github.com/Digilent/Eclypse-Z7-HW/tree/zmod_adc/master) of the same branch as this. In order to minimize confusion, currently the commit message of the imported hardware platform contains the commit hash of the exported hardware platform of the aforementioned Hardware repository.

The Linux project works with both the Petalinux project as is and the ~~[SD card image](https://reference.digilentinc.com/vivado/installing-vivado/start)~~ provided by us.

For more details on how to use the [zmodlib](https://github.com/Digilent/zmodlib) and how to set up your environment please visit [Zmod Base Library User Guide](https://reference.digilentinc.com/reference/zmod/zmodbaselibraryuserguide)

## First and Foremost

* Our projects use submodules to bring in libraries. Use --recursive when cloning or `git submodule init` and `git submodule update`, if cloned already non-recursively.

## Requirements

* **Eclypse Z7**
* **Zmod ADC 1410**
* **Vivado 2019.1 Installation with Xilinx SDK**: To set up Vivado, see the [Installing Vivado and Digilent Board Files Tutorial](https://reference.digilentinc.com/vivado/installing-vivado/start).

## Setup

For settings up the the Demo please visit [Environment Setup](https://reference.digilentinc.com/reference/zmod/zmodbaselibraryuserguide#environment_setup)

## Next Steps

This demo can be used as a basis for other projects by modifying the demo which showcases both the capabilities of the hardware and the zmodlib functions which are implemented.


## Additional Notes

For more information on the Eclypse Z7, visit it's [Eclypse Z7 Resource Center](https://reference.digilentinc.com/reference/programmable-logic/eclypse-z7/start) on the Digilent Wiki.

For more details on how to use the [zmodlib](https://github.com/Digilent/zmodlib) and how to set up your environment please visit [Zmod Base Library User Guide](https://reference.digilentinc.com/reference/zmod/zmodbaselibraryuserguide)

For more information on the Zmod ADC 1410, please visit it's [Zmod ADC Resource Center](https://reference.digilentinc.com/reference/zmod/zmodadc/start) on the Digilent Wiki.

For more information on how our git and porject flow is set up please refer to [Eclypse Z7 Git Repositoies](https://reference.digilentinc.com/reference/programmable-logic/eclypse-z7/git).

For technical support or questions, please post on the [Digilent Forum](forum.digilentinc.com).
