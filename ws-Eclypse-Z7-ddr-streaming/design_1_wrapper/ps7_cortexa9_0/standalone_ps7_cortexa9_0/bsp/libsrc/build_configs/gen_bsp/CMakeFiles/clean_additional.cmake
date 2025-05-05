# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "D:\\PrjMig-Vitis_2024.1\\tests\\eclypse-z7\\sw\\ws1\\design_1_wrapper\\ps7_cortexa9_0\\standalone_ps7_cortexa9_0\\bsp\\include\\sleep.h"
  "D:\\PrjMig-Vitis_2024.1\\tests\\eclypse-z7\\sw\\ws1\\design_1_wrapper\\ps7_cortexa9_0\\standalone_ps7_cortexa9_0\\bsp\\include\\xiltimer.h"
  "D:\\PrjMig-Vitis_2024.1\\tests\\eclypse-z7\\sw\\ws1\\design_1_wrapper\\ps7_cortexa9_0\\standalone_ps7_cortexa9_0\\bsp\\include\\xtimer_config.h"
  "D:\\PrjMig-Vitis_2024.1\\tests\\eclypse-z7\\sw\\ws1\\design_1_wrapper\\ps7_cortexa9_0\\standalone_ps7_cortexa9_0\\bsp\\lib\\libxiltimer.a"
  )
endif()
