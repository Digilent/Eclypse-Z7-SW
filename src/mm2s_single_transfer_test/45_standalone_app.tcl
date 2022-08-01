# This script will create an application named after the script directory
# based on the C "Empty Application" template and for a specific processor
# It imports all source files from the src subdirectory as LINKS.
# BUG 2020.1: linker script imported as link won't build, import by setting
# linker-script app config as a workaround.
# It also sets some C/C++ build settings
# Workspace should be set externally

set script [info script] 
set script_dir [file normalize [file dirname $script]]

puts "INFO: Running $script"

set app_name [file tail $script_dir]

# Variables created by checkin.tcl
set lang "c"
set domain "standalone_ps7_cortexa9_0"
set platform "design_1_wrapper"
set sysproj "mm2s_single_transfer_test_system"

# Handle dependent variables
if {$lang == "c"} {
	set template "Empty Application(C)"
} elseif {$lang == "c++"} {
	set template "Empty Application (C++)"
} else {
	return -code error "invalid language selection in [file tail $script]; $lang should be c or c++"
}


# unused `app create` arguments:
# -os, -arch, and -proc are inferred from -domain?
# -hw conflicts with -platform usage
# -template must be specified, otherwise Hello World by default
app create -name $app_name -lang $lang -template $template -domain $domain -platform $platform -sysproj $sysproj

# even the "Empty Application" template creates lscript.ld and Readme
# can cause confusion later when linking linker script
# delete all files created by the template
if { [catch {file delete -- {*}[glob -directory [file normalize [getws]/$app_name/src] *]} result] } {
    puts "INFO: emptying src/ dir after project creation failed with $result"
}

importsources -name $app_name -path $script_dir/src -soft-link

# Set project settings per build configuration (handled by checkin)
app config -set -name $app_name build-config Release
app config -set -name $app_name assembler-flags {}
app config -set -name $app_name compiler-misc {-c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard}
app config -set -name $app_name compiler-optimization {Optimize more (-O2)}
app config -add -name $app_name include-path $script_dir/src
app config -set -name $app_name linker-script $script_dir/src/lscript.ld
app config -set -name $app_name build-config Debug
app config -set -name $app_name assembler-flags {}
app config -set -name $app_name compiler-misc {-c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard}
app config -set -name $app_name compiler-optimization {None (-O0)}
app config -add -name $app_name include-path $script_dir/src
app config -set -name $app_name linker-script $script_dir/src/lscript.ld
