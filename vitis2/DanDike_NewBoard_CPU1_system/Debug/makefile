#
# This file was created by Xilinx Vitis tool.
#


#
# tools
#

RM = rm -f
RMDIR = rm -rf



#
# variables
#

SDCARD_GEN = sdcard_gen
GENERATE_BIF = ::scw::generate_bif

XPFM_PATH = C:/Users/saber/Desktop/share/DanDike_NewBoard/vitis2/system_wrapper/export/system_wrapper/system_wrapper.xpfm
SYS_CONFIG = system_wrapper
DOMAINS = rtos_ps7_crotex9_1
BIF_PATH = C:/Users/saber/Desktop/share/DanDike_NewBoard/vitis2/DanDike_NewBoard_CPU1_system/Debug/system.bif

BITSTREAM_PATH = C:/Users/saber/Desktop/share/DanDike_NewBoard/vitis2/DanDike_NewBoard_CPU1/_ide/bitstream/system_wrapper.bit
SD_FILES = --elf C:/Users/saber/Desktop/share/DanDike_NewBoard/vitis2/DanDike_NewBoard_CPU1/Debug/DanDike_NewBoard_CPU1.elf,ps7_cortexa9_1

#
# primary build targets
#

all: package

package:
	-@echo Generating bif file for the system project
	generate_system_bif.bat 13371 ${XPFM_PATH} ${DOMAINS} ${BIF_PATH}
	${SDCARD_GEN} --xpfm ${XPFM_PATH} --sys_config ${SYS_CONFIG} --bif ${BIF_PATH} --bitstream ${BITSTREAM_PATH} ${SD_FILES}

clean:
	$(RMDIR) ./_sds
	${RMDIR} package
	${RM} system.bif

.PHONY: all package clean


