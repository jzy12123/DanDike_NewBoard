# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct C:\Users\saber\Desktop\share\DanDike_NewBoard\vitis1\system_wrapper\platform.tcl
# 
# OR launch xsct and run below command.
# source C:\Users\saber\Desktop\share\DanDike_NewBoard\vitis1\system_wrapper\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {system_wrapper}\
-hw {C:\Users\saber\Desktop\share\DanDike_NewBoard\vitis1\system_wrapper.xsa}\
-fsbl-target {psu_cortexa53_0} -out {C:/Users/saber/Desktop/share/DanDike_NewBoard/vitis1}

platform write
domain create -name {standalone_ps7_cortexa9_1} -display-name {standalone_ps7_cortexa9_1} -os {standalone} -proc {ps7_cortexa9_1} -runtime {cpp} -arch {32-bit} -support-app {empty_application}
platform generate -domains 
platform active {system_wrapper}
domain active {zynq_fsbl}
domain active {standalone_ps7_cortexa9_1}
platform generate -quick
platform generate
platform clean
platform generate
platform active {system_wrapper}
domain active {zynq_fsbl}
bsp reload
bsp reload
platform generate -domains 
platform generate -domains zynq_fsbl 
platform generate -domains zynq_fsbl 
