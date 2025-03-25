vlib work
vlib activehdl

vlib activehdl/xilinx_vip
vlib activehdl/xpm
vlib activehdl/axi_infrastructure_v1_1_0
vlib activehdl/axi_vip_v1_1_8
vlib activehdl/processing_system7_vip_v1_0_10
vlib activehdl/xil_defaultlib
vlib activehdl/lib_cdc_v1_0_2
vlib activehdl/lib_pkg_v1_0_2
vlib activehdl/fifo_generator_v13_2_5
vlib activehdl/lib_fifo_v1_0_14
vlib activehdl/blk_mem_gen_v8_4_4
vlib activehdl/lib_bmg_v1_0_13
vlib activehdl/lib_srl_fifo_v1_0_2
vlib activehdl/axi_datamover_v5_1_24
vlib activehdl/axi_vdma_v6_3_10
vlib activehdl/axi_lite_ipif_v3_0_4
vlib activehdl/v_tc_v6_2_1
vlib activehdl/v_tc_v6_1_13
vlib activehdl/v_vid_in_axi4s_v4_0_9
vlib activehdl/v_axi4s_vid_out_v4_0_11
vlib activehdl/interrupt_control_v3_1_4
vlib activehdl/axi_gpio_v2_0_24
vlib activehdl/xlconstant_v1_1_7
vlib activehdl/proc_sys_reset_v5_0_13
vlib activehdl/smartconnect_v1_0
vlib activehdl/axi_register_slice_v2_1_22
vlib activehdl/xlconcat_v2_1_4
vlib activehdl/generic_baseblocks_v2_1_0
vlib activehdl/axi_data_fifo_v2_1_21
vlib activehdl/axi_crossbar_v2_1_23
vlib activehdl/axi_sg_v4_1_13
vlib activehdl/axi_dma_v7_1_23
vlib activehdl/axis_infrastructure_v1_1_0
vlib activehdl/axis_data_fifo_v2_0_4
vlib activehdl/axi_bram_ctrl_v4_1_4
vlib activehdl/axi_uartlite_v2_0_26
vlib activehdl/util_vector_logic_v2_0_1
vlib activehdl/axi_iic_v2_0_25
vlib activehdl/axi_protocol_converter_v2_1_22

vmap xilinx_vip activehdl/xilinx_vip
vmap xpm activehdl/xpm
vmap axi_infrastructure_v1_1_0 activehdl/axi_infrastructure_v1_1_0
vmap axi_vip_v1_1_8 activehdl/axi_vip_v1_1_8
vmap processing_system7_vip_v1_0_10 activehdl/processing_system7_vip_v1_0_10
vmap xil_defaultlib activehdl/xil_defaultlib
vmap lib_cdc_v1_0_2 activehdl/lib_cdc_v1_0_2
vmap lib_pkg_v1_0_2 activehdl/lib_pkg_v1_0_2
vmap fifo_generator_v13_2_5 activehdl/fifo_generator_v13_2_5
vmap lib_fifo_v1_0_14 activehdl/lib_fifo_v1_0_14
vmap blk_mem_gen_v8_4_4 activehdl/blk_mem_gen_v8_4_4
vmap lib_bmg_v1_0_13 activehdl/lib_bmg_v1_0_13
vmap lib_srl_fifo_v1_0_2 activehdl/lib_srl_fifo_v1_0_2
vmap axi_datamover_v5_1_24 activehdl/axi_datamover_v5_1_24
vmap axi_vdma_v6_3_10 activehdl/axi_vdma_v6_3_10
vmap axi_lite_ipif_v3_0_4 activehdl/axi_lite_ipif_v3_0_4
vmap v_tc_v6_2_1 activehdl/v_tc_v6_2_1
vmap v_tc_v6_1_13 activehdl/v_tc_v6_1_13
vmap v_vid_in_axi4s_v4_0_9 activehdl/v_vid_in_axi4s_v4_0_9
vmap v_axi4s_vid_out_v4_0_11 activehdl/v_axi4s_vid_out_v4_0_11
vmap interrupt_control_v3_1_4 activehdl/interrupt_control_v3_1_4
vmap axi_gpio_v2_0_24 activehdl/axi_gpio_v2_0_24
vmap xlconstant_v1_1_7 activehdl/xlconstant_v1_1_7
vmap proc_sys_reset_v5_0_13 activehdl/proc_sys_reset_v5_0_13
vmap smartconnect_v1_0 activehdl/smartconnect_v1_0
vmap axi_register_slice_v2_1_22 activehdl/axi_register_slice_v2_1_22
vmap xlconcat_v2_1_4 activehdl/xlconcat_v2_1_4
vmap generic_baseblocks_v2_1_0 activehdl/generic_baseblocks_v2_1_0
vmap axi_data_fifo_v2_1_21 activehdl/axi_data_fifo_v2_1_21
vmap axi_crossbar_v2_1_23 activehdl/axi_crossbar_v2_1_23
vmap axi_sg_v4_1_13 activehdl/axi_sg_v4_1_13
vmap axi_dma_v7_1_23 activehdl/axi_dma_v7_1_23
vmap axis_infrastructure_v1_1_0 activehdl/axis_infrastructure_v1_1_0
vmap axis_data_fifo_v2_0_4 activehdl/axis_data_fifo_v2_0_4
vmap axi_bram_ctrl_v4_1_4 activehdl/axi_bram_ctrl_v4_1_4
vmap axi_uartlite_v2_0_26 activehdl/axi_uartlite_v2_0_26
vmap util_vector_logic_v2_0_1 activehdl/util_vector_logic_v2_0_1
vmap axi_iic_v2_0_25 activehdl/axi_iic_v2_0_25
vmap axi_protocol_converter_v2_1_22 activehdl/axi_protocol_converter_v2_1_22

vlog -work xilinx_vip  -sv2k12 "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi4stream_vip_axi4streampc.sv" \
"E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi_vip_axi4pc.sv" \
"E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/xil_common_vip_pkg.sv" \
"E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi4stream_vip_pkg.sv" \
"E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi_vip_pkg.sv" \
"E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi4stream_vip_if.sv" \
"E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi_vip_if.sv" \
"E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/clk_vip_if.sv" \
"E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/rst_vip_if.sv" \

vlog -work xpm  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"E:/vivado/Vivado/2020.2/data/ip/xpm/xpm_cdc/hdl/xpm_cdc.sv" \
"E:/vivado/Vivado/2020.2/data/ip/xpm/xpm_fifo/hdl/xpm_fifo.sv" \
"E:/vivado/Vivado/2020.2/data/ip/xpm/xpm_memory/hdl/xpm_memory.sv" \

vcom -work xpm -93 \
"E:/vivado/Vivado/2020.2/data/ip/xpm/xpm_VCOMP.vhd" \

vlog -work axi_infrastructure_v1_1_0  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl/axi_infrastructure_v1_1_vl_rfs.v" \

vlog -work axi_vip_v1_1_8  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/94c3/hdl/axi_vip_v1_1_vl_rfs.sv" \

vlog -work processing_system7_vip_v1_0_10  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl/processing_system7_vip_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_processing_system7_0_0/sim/system_processing_system7_0_0.v" \

vcom -work lib_cdc_v1_0_2 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ef1e/hdl/lib_cdc_v1_0_rfs.vhd" \

vcom -work lib_pkg_v1_0_2 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/0513/hdl/lib_pkg_v1_0_rfs.vhd" \

vlog -work fifo_generator_v13_2_5  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/276e/simulation/fifo_generator_vlog_beh.v" \

vcom -work fifo_generator_v13_2_5 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/276e/hdl/fifo_generator_v13_2_rfs.vhd" \

vlog -work fifo_generator_v13_2_5  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/276e/hdl/fifo_generator_v13_2_rfs.v" \

vcom -work lib_fifo_v1_0_14 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/a5cb/hdl/lib_fifo_v1_0_rfs.vhd" \

vlog -work blk_mem_gen_v8_4_4  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/2985/simulation/blk_mem_gen_v8_4.v" \

vcom -work lib_bmg_v1_0_13 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/af67/hdl/lib_bmg_v1_0_rfs.vhd" \

vcom -work lib_srl_fifo_v1_0_2 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/51ce/hdl/lib_srl_fifo_v1_0_rfs.vhd" \

vcom -work axi_datamover_v5_1_24 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/4ab6/hdl/axi_datamover_v5_1_vh_rfs.vhd" \

vlog -work axi_vdma_v6_3_10  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl/axi_vdma_v6_3_rfs.v" \

vcom -work axi_vdma_v6_3_10 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl/axi_vdma_v6_3_rfs.vhd" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_axi_vdma_0_0/sim/system_axi_vdma_0_0.vhd" \

vcom -work axi_lite_ipif_v3_0_4 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/66ea/hdl/axi_lite_ipif_v3_0_vh_rfs.vhd" \

vcom -work v_tc_v6_2_1 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/cd2e/hdl/v_tc_v6_2_vh_rfs.vhd" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_v_tc_0_0/sim/system_v_tc_0_0.vhd" \

vcom -work v_tc_v6_1_13 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/b92e/hdl/v_tc_v6_1_vh_rfs.vhd" \

vlog -work v_vid_in_axi4s_v4_0_9  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/b2aa/hdl/v_vid_in_axi4s_v4_0_vl_rfs.v" \

vlog -work v_axi4s_vid_out_v4_0_11  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/1a1e/hdl/v_axi4s_vid_out_v4_0_vl_rfs.v" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_v_axi4s_vid_out_0_0/sim/system_v_axi4s_vid_out_0_0.v" \

vcom -work interrupt_control_v3_1_4 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/a040/hdl/interrupt_control_v3_1_vh_rfs.vhd" \

vcom -work axi_gpio_v2_0_24 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/4318/hdl/axi_gpio_v2_0_vh_rfs.vhd" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_axi_gpio_0_0/sim/system_axi_gpio_0_0.vhd" \

vlog -work xlconstant_v1_1_7  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/fcfc/hdl/xlconstant_v1_1_vl_rfs.v" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_0/sim/bd_44e3_one_0.v" \

vcom -work proc_sys_reset_v5_0_13 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8842/hdl/proc_sys_reset_v5_0_vh_rfs.vhd" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_1/sim/bd_44e3_psr_aclk_0.vhd" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/sc_util_v1_0_vl_rfs.sv" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/c012/hdl/sc_switchboard_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_2/sim/bd_44e3_arsw_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_3/sim/bd_44e3_rsw_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_4/sim/bd_44e3_awsw_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_5/sim/bd_44e3_wsw_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_6/sim/bd_44e3_bsw_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ea34/hdl/sc_mmu_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_7/sim/bd_44e3_s00mmu_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/4fd2/hdl/sc_transaction_regulator_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_8/sim/bd_44e3_s00tr_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8047/hdl/sc_si_converter_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_9/sim/bd_44e3_s00sic_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/b89e/hdl/sc_axi2sc_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_10/sim/bd_44e3_s00a2s_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/sc_node_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_11/sim/bd_44e3_sarn_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_12/sim/bd_44e3_srn_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_13/sim/bd_44e3_s01mmu_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_14/sim/bd_44e3_s01tr_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_15/sim/bd_44e3_s01sic_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_16/sim/bd_44e3_s01a2s_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_17/sim/bd_44e3_sawn_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_18/sim/bd_44e3_swn_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_19/sim/bd_44e3_sbn_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7005/hdl/sc_sc2axi_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_20/sim/bd_44e3_m00s2a_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_21/sim/bd_44e3_m00arn_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_22/sim/bd_44e3_m00rn_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_23/sim/bd_44e3_m00awn_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_24/sim/bd_44e3_m00wn_0.sv" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_25/sim/bd_44e3_m00bn_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7bd7/hdl/sc_exit_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_26/sim/bd_44e3_m00e_0.sv" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axi_smc_0/bd_0/sim/bd_44e3.v" \

vlog -work axi_register_slice_v2_1_22  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/af2c/hdl/axi_register_slice_v2_1_vl_rfs.v" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axi_smc_0/sim/system_axi_smc_0.v" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_rst_ps7_0_100M_0/sim/system_rst_ps7_0_100M_0.vhd" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_clk_wiz_0_0/system_clk_wiz_0_0_mmcm_pll_drp.v" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_clk_wiz_0_0/proc_common_v3_00_a/hdl/src/vhdl/system_clk_wiz_0_0_conv_funs_pkg.vhd" \
"../../../bd/system/ip/system_clk_wiz_0_0/proc_common_v3_00_a/hdl/src/vhdl/system_clk_wiz_0_0_proc_common_pkg.vhd" \
"../../../bd/system/ip/system_clk_wiz_0_0/proc_common_v3_00_a/hdl/src/vhdl/system_clk_wiz_0_0_ipif_pkg.vhd" \
"../../../bd/system/ip/system_clk_wiz_0_0/proc_common_v3_00_a/hdl/src/vhdl/system_clk_wiz_0_0_family_support.vhd" \
"../../../bd/system/ip/system_clk_wiz_0_0/proc_common_v3_00_a/hdl/src/vhdl/system_clk_wiz_0_0_family.vhd" \
"../../../bd/system/ip/system_clk_wiz_0_0/proc_common_v3_00_a/hdl/src/vhdl/system_clk_wiz_0_0_soft_reset.vhd" \
"../../../bd/system/ip/system_clk_wiz_0_0/proc_common_v3_00_a/hdl/src/vhdl/system_clk_wiz_0_0_pselect_f.vhd" \
"../../../bd/system/ip/system_clk_wiz_0_0/axi_lite_ipif_v1_01_a/hdl/src/vhdl/system_clk_wiz_0_0_address_decoder.vhd" \
"../../../bd/system/ip/system_clk_wiz_0_0/axi_lite_ipif_v1_01_a/hdl/src/vhdl/system_clk_wiz_0_0_slave_attachment.vhd" \
"../../../bd/system/ip/system_clk_wiz_0_0/axi_lite_ipif_v1_01_a/hdl/src/vhdl/system_clk_wiz_0_0_axi_lite_ipif.vhd" \
"../../../bd/system/ip/system_clk_wiz_0_0/system_clk_wiz_0_0_clk_wiz_drp.vhd" \
"../../../bd/system/ip/system_clk_wiz_0_0/system_clk_wiz_0_0_axi_clk_config.vhd" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_clk_wiz_0_0/system_clk_wiz_0_0_clk_wiz.v" \
"../../../bd/system/ip/system_clk_wiz_0_0/system_clk_wiz_0_0.v" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ipshared/154f/hdl/PWM_AXI.sv" \
"../../../bd/system/ipshared/154f/hdl/PWM_v2_0.sv" \
"../../../bd/system/ip/system_PWM_0_0/sim/system_PWM_0_0.sv" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ipshared/214f/src/rgb2lcd.v" \
"../../../bd/system/ip/system_rgb2lcd_0_1/sim/system_rgb2lcd_0_1.v" \

vlog -work xlconcat_v2_1_4  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/4b67/hdl/xlconcat_v2_1_vl_rfs.v" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_xlconcat_0_0/sim/system_xlconcat_0_0.v" \

vlog -work generic_baseblocks_v2_1_0  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/b752/hdl/generic_baseblocks_v2_1_vl_rfs.v" \

vlog -work axi_data_fifo_v2_1_21  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/54c0/hdl/axi_data_fifo_v2_1_vl_rfs.v" \

vlog -work axi_crossbar_v2_1_23  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/bc0a/hdl/axi_crossbar_v2_1_vl_rfs.v" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_xbar_0/sim/system_xbar_0.v" \

vcom -work axi_sg_v4_1_13 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/4919/hdl/axi_sg_v4_1_rfs.vhd" \

vcom -work axi_dma_v7_1_23 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/89d8/hdl/axi_dma_v7_1_vh_rfs.vhd" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_axi_dma_0_0/sim/system_axi_dma_0_0.vhd" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ipshared/dd64/ad7606_axis.v" \
"../../../bd/system/ipshared/dd64/ad7606_ser2par128bit.v" \
"../../../bd/system/ipshared/dd64/adc_whole.v" \
"../../../bd/system/ip/system_adc_whole_0_0/sim/system_adc_whole_0_0.v" \

vlog -work axis_infrastructure_v1_1_0  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl/axis_infrastructure_v1_1_vl_rfs.v" \

vlog -work axis_data_fifo_v2_0_4  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/abd4/hdl/axis_data_fifo_v2_0_vl_rfs.v" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_axis_data_fifo_0_0/sim/system_axis_data_fifo_0_0.v" \
"../../../bd/system/ipshared/24ef/dac_spi.v" \
"../../../bd/system/ipshared/24ef/dds_dac.v" \
"../../../bd/system/ipshared/24ef/dma_axis_s.v" \
"../../../bd/system/ipshared/24ef/dac_whole.v" \
"../../../bd/system/ip/system_dac_whole_0_1/sim/system_dac_whole_0_1.v" \
"../../../bd/system/ip/system_axis_data_fifo_1_1/sim/system_axis_data_fifo_1_1.v" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_proc_sys_reset_0_0/sim/system_proc_sys_reset_0_0.vhd" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_blk_mem_gen_0_0/sim/system_blk_mem_gen_0_0.v" \

vcom -work axi_bram_ctrl_v4_1_4 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/c9f0/hdl/axi_bram_ctrl_v4_1_rfs.vhd" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_axi_bram_ctrl_0_0/sim/system_axi_bram_ctrl_0_0.vhd" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ipshared/6fec/config2ser16bits.v" \
"../../../bd/system/ipshared/6fec/onoff_io.v" \
"../../../bd/system/ipshared/6fec/rd_from_serial8bit.v" \
"../../../bd/system/ipshared/6fec/onoff_config_axi.v" \
"../../../bd/system/ip/system_onoff_config_axi_0_0/sim/system_onoff_config_axi_0_0.v" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_rst_ps7_0_142M_0/sim/system_rst_ps7_0_142M_0.vhd" \
"../../../bd/system/ip/system_axi_gpio_0_1/sim/system_axi_gpio_0_1.vhd" \

vcom -work axi_uartlite_v2_0_26 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/5edb/hdl/axi_uartlite_v2_0_vh_rfs.vhd" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_axi_uartlite_0_0/sim/system_axi_uartlite_0_0.vhd" \

vlog -work util_vector_logic_v2_0_1  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/3f90/hdl/util_vector_logic_v2_0_vl_rfs.v" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_util_vector_logic_0_0/sim/system_util_vector_logic_0_0.v" \

vcom -work axi_iic_v2_0_25 -93 \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/1529/hdl/axi_iic_v2_0_vh_rfs.vhd" \

vcom -work xil_defaultlib -93 \
"../../../bd/system/ip/system_axi_iic_0_0/sim/system_axi_iic_0_0.vhd" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ipshared/2451/hdl/coder_v1_0_S00_AXI.v" \
"../../../bd/system/ipshared/2451/hdl/rotary_encoder.v" \
"../../../bd/system/ipshared/2451/hdl/coder_v1_0.v" \
"../../../bd/system/ip/system_coder_0_0/sim/system_coder_0_0.v" \
"../../../bd/system/ip/system_util_vector_logic_0_1/sim/system_util_vector_logic_0_1.v" \
"../../../bd/system/ipshared/934f/rgmii_rx.v" \
"../../../bd/system/ipshared/934f/rgmii_tx.v" \
"../../../bd/system/ipshared/934f/gmii_to_rgmii.v" \
"../../../bd/system/ip/system_gmii2rgmii_0_0/sim/system_gmii2rgmii_0_0.v" \
"../../../bd/system/ipshared/1858/power_pulse_in.v" \
"../../../bd/system/ipshared/1858/power_pulse_out.v" \
"../../../bd/system/ipshared/1858/power_pulse_v1_AXI.v" \
"../../../bd/system/ip/system_power_pulse_v1_AXI_0_0/sim/system_power_pulse_v1_AXI_0_0.v" \
"../../../bd/system/ip/system_tier2_xbar_0_0/sim/system_tier2_xbar_0_0.v" \
"../../../bd/system/ip/system_tier2_xbar_1_0/sim/system_tier2_xbar_1_0.v" \
"../../../bd/system/ip/system_tier2_xbar_2_0/sim/system_tier2_xbar_2_0.v" \

vlog -work axi_protocol_converter_v2_1_22  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/5cee/hdl/axi_protocol_converter_v2_1_vl_rfs.v" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/verilog" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/d0f7" "+incdir+../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl" "+incdir+E:/vivado/Vivado/2020.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_auto_pc_0_1/sim/system_auto_pc_0.v" \
"../../../bd/system/ip/system_auto_pc_7_1/sim/system_auto_pc_7.v" \
"../../../bd/system/ip/system_auto_pc_1_1/sim/system_auto_pc_1.v" \
"../../../bd/system/ip/system_auto_pc_2/sim/system_auto_pc_2.v" \
"../../../bd/system/ip/system_auto_pc_3_1/sim/system_auto_pc_3.v" \
"../../../bd/system/ip/system_auto_pc_4_1/sim/system_auto_pc_4.v" \
"../../../bd/system/ip/system_auto_pc_5/sim/system_auto_pc_5.v" \
"../../../bd/system/ip/system_auto_pc_6/sim/system_auto_pc_6.v" \
"../../../bd/system/ip/system_auto_pc_8/sim/system_auto_pc_8.v" \
"../../../bd/system/sim/system.v" \

vlog -work xil_defaultlib \
"glbl.v"

