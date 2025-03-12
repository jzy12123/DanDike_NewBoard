# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: C:\Users\saber\Desktop\share\DanDike_NewBoard\vitis3\DK51\_ide\scripts\systemdebugger_dk51_standalone.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source C:\Users\saber\Desktop\share\DanDike_NewBoard\vitis3\DK51\_ide\scripts\systemdebugger_dk51_standalone.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Digilent JTAG-SMT2 210251A08870" && level==0 && jtag_device_ctx=="jsn-JTAG-SMT2-210251A08870-23727093-0"}
fpga -file C:/Users/saber/Desktop/share/DanDike_NewBoard/vitis3/CPU0/_ide/bitstream/system_wrapper.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw C:/Users/saber/Desktop/share/DanDike_NewBoard/vitis3/system_wrapper/export/system_wrapper/hw/system_wrapper.xsa -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
source C:/Users/saber/Desktop/share/DanDike_NewBoard/vitis3/CPU0/_ide/psinit/ps7_init.tcl
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
dow C:/Users/saber/Desktop/share/DanDike_NewBoard/vitis3/CPU0/Debug/CPU0.elf
targets -set -nocase -filter {name =~ "*A9*#1"}
dow C:/Users/saber/Desktop/share/DanDike_NewBoard/vitis3/CPU1/Debug/CPU1.elf
configparams force-mem-access 0
bpadd -addr &main
