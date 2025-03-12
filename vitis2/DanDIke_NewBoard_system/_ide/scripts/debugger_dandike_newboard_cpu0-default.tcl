# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: C:\Users\saber\Desktop\DanDike_NewBoard\vitis2\DanDIke_NewBoard_system\_ide\scripts\debugger_dandike_newboard_cpu0-default.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source C:\Users\saber\Desktop\DanDike_NewBoard\vitis2\DanDIke_NewBoard_system\_ide\scripts\debugger_dandike_newboard_cpu0-default.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Digilent JTAG-SMT2 210251A08870" && level==0 && jtag_device_ctx=="jsn-JTAG-SMT2-210251A08870-23727093-0"}
fpga -file C:/Users/saber/Desktop/DanDike_NewBoard/vitis2/DanDIke_NewBoard_CPU0/_ide/bitstream/system_wrapper.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw C:/Users/saber/Desktop/DanDike_NewBoard/vitis2/system_wrapper/export/system_wrapper/hw/system_wrapper.xsa -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
source C:/Users/saber/Desktop/DanDike_NewBoard/vitis2/DanDIke_NewBoard_CPU0/_ide/psinit/ps7_init.tcl
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
dow C:/Users/saber/Desktop/DanDike_NewBoard/vitis2/DanDIke_NewBoard_CPU0/Debug/DanDIke_NewBoard_CPU0.elf
configparams force-mem-access 0
targets -set -nocase -filter {name =~ "*A9*#0"}
con
