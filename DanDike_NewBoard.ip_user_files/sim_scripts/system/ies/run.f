-makelib ies_lib/xilinx_vip -sv \
  "E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi4stream_vip_axi4streampc.sv" \
  "E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi_vip_axi4pc.sv" \
  "E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/xil_common_vip_pkg.sv" \
  "E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi4stream_vip_pkg.sv" \
  "E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi_vip_pkg.sv" \
  "E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi4stream_vip_if.sv" \
  "E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/axi_vip_if.sv" \
  "E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/clk_vip_if.sv" \
  "E:/vivado/Vivado/2020.2/data/xilinx_vip/hdl/rst_vip_if.sv" \
-endlib
-makelib ies_lib/xpm -sv \
  "E:/vivado/Vivado/2020.2/data/ip/xpm/xpm_cdc/hdl/xpm_cdc.sv" \
  "E:/vivado/Vivado/2020.2/data/ip/xpm/xpm_fifo/hdl/xpm_fifo.sv" \
  "E:/vivado/Vivado/2020.2/data/ip/xpm/xpm_memory/hdl/xpm_memory.sv" \
-endlib
-makelib ies_lib/xpm \
  "E:/vivado/Vivado/2020.2/data/ip/xpm/xpm_VCOMP.vhd" \
-endlib
-makelib ies_lib/axi_infrastructure_v1_1_0 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ec67/hdl/axi_infrastructure_v1_1_vl_rfs.v" \
-endlib
-makelib ies_lib/axi_vip_v1_1_8 -sv \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/94c3/hdl/axi_vip_v1_1_vl_rfs.sv" \
-endlib
-makelib ies_lib/processing_system7_vip_v1_0_10 -sv \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/34f8/hdl/processing_system7_vip_v1_0_vl_rfs.sv" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_processing_system7_0_0/sim/system_processing_system7_0_0.v" \
-endlib
-makelib ies_lib/lib_cdc_v1_0_2 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ef1e/hdl/lib_cdc_v1_0_rfs.vhd" \
-endlib
-makelib ies_lib/lib_pkg_v1_0_2 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/0513/hdl/lib_pkg_v1_0_rfs.vhd" \
-endlib
-makelib ies_lib/fifo_generator_v13_2_5 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/276e/simulation/fifo_generator_vlog_beh.v" \
-endlib
-makelib ies_lib/fifo_generator_v13_2_5 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/276e/hdl/fifo_generator_v13_2_rfs.vhd" \
-endlib
-makelib ies_lib/fifo_generator_v13_2_5 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/276e/hdl/fifo_generator_v13_2_rfs.v" \
-endlib
-makelib ies_lib/lib_fifo_v1_0_14 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/a5cb/hdl/lib_fifo_v1_0_rfs.vhd" \
-endlib
-makelib ies_lib/blk_mem_gen_v8_4_4 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/2985/simulation/blk_mem_gen_v8_4.v" \
-endlib
-makelib ies_lib/lib_bmg_v1_0_13 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/af67/hdl/lib_bmg_v1_0_rfs.vhd" \
-endlib
-makelib ies_lib/lib_srl_fifo_v1_0_2 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/51ce/hdl/lib_srl_fifo_v1_0_rfs.vhd" \
-endlib
-makelib ies_lib/axi_datamover_v5_1_24 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/4ab6/hdl/axi_datamover_v5_1_vh_rfs.vhd" \
-endlib
-makelib ies_lib/axi_vdma_v6_3_10 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl/axi_vdma_v6_3_rfs.v" \
-endlib
-makelib ies_lib/axi_vdma_v6_3_10 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7860/hdl/axi_vdma_v6_3_rfs.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_axi_vdma_0_0/sim/system_axi_vdma_0_0.vhd" \
-endlib
-makelib ies_lib/axi_lite_ipif_v3_0_4 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/66ea/hdl/axi_lite_ipif_v3_0_vh_rfs.vhd" \
-endlib
-makelib ies_lib/v_tc_v6_2_1 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/cd2e/hdl/v_tc_v6_2_vh_rfs.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_v_tc_0_0/sim/system_v_tc_0_0.vhd" \
-endlib
-makelib ies_lib/v_tc_v6_1_13 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/b92e/hdl/v_tc_v6_1_vh_rfs.vhd" \
-endlib
-makelib ies_lib/v_vid_in_axi4s_v4_0_9 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/b2aa/hdl/v_vid_in_axi4s_v4_0_vl_rfs.v" \
-endlib
-makelib ies_lib/v_axi4s_vid_out_v4_0_11 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/1a1e/hdl/v_axi4s_vid_out_v4_0_vl_rfs.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_v_axi4s_vid_out_0_0/sim/system_v_axi4s_vid_out_0_0.v" \
-endlib
-makelib ies_lib/interrupt_control_v3_1_4 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/a040/hdl/interrupt_control_v3_1_vh_rfs.vhd" \
-endlib
-makelib ies_lib/axi_gpio_v2_0_24 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/4318/hdl/axi_gpio_v2_0_vh_rfs.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_axi_gpio_0_0/sim/system_axi_gpio_0_0.vhd" \
-endlib
-makelib ies_lib/xlconstant_v1_1_7 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/fcfc/hdl/xlconstant_v1_1_vl_rfs.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_0/sim/bd_44e3_one_0.v" \
-endlib
-makelib ies_lib/proc_sys_reset_v5_0_13 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8842/hdl/proc_sys_reset_v5_0_vh_rfs.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_1/sim/bd_44e3_psr_aclk_0.vhd" \
-endlib
-makelib ies_lib/smartconnect_v1_0 -sv \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/25b7/hdl/sc_util_v1_0_vl_rfs.sv" \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/c012/hdl/sc_switchboard_v1_0_vl_rfs.sv" \
-endlib
-makelib ies_lib/xil_defaultlib -sv \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_2/sim/bd_44e3_arsw_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_3/sim/bd_44e3_rsw_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_4/sim/bd_44e3_awsw_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_5/sim/bd_44e3_wsw_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_6/sim/bd_44e3_bsw_0.sv" \
-endlib
-makelib ies_lib/smartconnect_v1_0 -sv \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/ea34/hdl/sc_mmu_v1_0_vl_rfs.sv" \
-endlib
-makelib ies_lib/xil_defaultlib -sv \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_7/sim/bd_44e3_s00mmu_0.sv" \
-endlib
-makelib ies_lib/smartconnect_v1_0 -sv \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/4fd2/hdl/sc_transaction_regulator_v1_0_vl_rfs.sv" \
-endlib
-makelib ies_lib/xil_defaultlib -sv \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_8/sim/bd_44e3_s00tr_0.sv" \
-endlib
-makelib ies_lib/smartconnect_v1_0 -sv \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8047/hdl/sc_si_converter_v1_0_vl_rfs.sv" \
-endlib
-makelib ies_lib/xil_defaultlib -sv \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_9/sim/bd_44e3_s00sic_0.sv" \
-endlib
-makelib ies_lib/smartconnect_v1_0 -sv \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/b89e/hdl/sc_axi2sc_v1_0_vl_rfs.sv" \
-endlib
-makelib ies_lib/xil_defaultlib -sv \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_10/sim/bd_44e3_s00a2s_0.sv" \
-endlib
-makelib ies_lib/smartconnect_v1_0 -sv \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/896c/hdl/sc_node_v1_0_vl_rfs.sv" \
-endlib
-makelib ies_lib/xil_defaultlib -sv \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_11/sim/bd_44e3_sarn_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_12/sim/bd_44e3_srn_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_13/sim/bd_44e3_s01mmu_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_14/sim/bd_44e3_s01tr_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_15/sim/bd_44e3_s01sic_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_16/sim/bd_44e3_s01a2s_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_17/sim/bd_44e3_sawn_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_18/sim/bd_44e3_swn_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_19/sim/bd_44e3_sbn_0.sv" \
-endlib
-makelib ies_lib/smartconnect_v1_0 -sv \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7005/hdl/sc_sc2axi_v1_0_vl_rfs.sv" \
-endlib
-makelib ies_lib/xil_defaultlib -sv \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_20/sim/bd_44e3_m00s2a_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_21/sim/bd_44e3_m00arn_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_22/sim/bd_44e3_m00rn_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_23/sim/bd_44e3_m00awn_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_24/sim/bd_44e3_m00wn_0.sv" \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_25/sim/bd_44e3_m00bn_0.sv" \
-endlib
-makelib ies_lib/smartconnect_v1_0 -sv \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/7bd7/hdl/sc_exit_v1_0_vl_rfs.sv" \
-endlib
-makelib ies_lib/xil_defaultlib -sv \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/ip/ip_26/sim/bd_44e3_m00e_0.sv" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_axi_smc_0/bd_0/sim/bd_44e3.v" \
-endlib
-makelib ies_lib/axi_register_slice_v2_1_22 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/af2c/hdl/axi_register_slice_v2_1_vl_rfs.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_axi_smc_0/sim/system_axi_smc_0.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_rst_ps7_0_100M_0/sim/system_rst_ps7_0_100M_0.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_clk_wiz_0_0/system_clk_wiz_0_0_mmcm_pll_drp.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
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
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_clk_wiz_0_0/system_clk_wiz_0_0_clk_wiz.v" \
  "../../../bd/system/ip/system_clk_wiz_0_0/system_clk_wiz_0_0.v" \
-endlib
-makelib ies_lib/xil_defaultlib -sv \
  "../../../bd/system/ipshared/154f/hdl/PWM_AXI.sv" \
  "../../../bd/system/ipshared/154f/hdl/PWM_v2_0.sv" \
  "../../../bd/system/ip/system_PWM_0_0/sim/system_PWM_0_0.sv" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ipshared/214f/src/rgb2lcd.v" \
  "../../../bd/system/ip/system_rgb2lcd_0_1/sim/system_rgb2lcd_0_1.v" \
-endlib
-makelib ies_lib/xlconcat_v2_1_4 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/4b67/hdl/xlconcat_v2_1_vl_rfs.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_xlconcat_0_0/sim/system_xlconcat_0_0.v" \
-endlib
-makelib ies_lib/generic_baseblocks_v2_1_0 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/b752/hdl/generic_baseblocks_v2_1_vl_rfs.v" \
-endlib
-makelib ies_lib/axi_data_fifo_v2_1_21 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/54c0/hdl/axi_data_fifo_v2_1_vl_rfs.v" \
-endlib
-makelib ies_lib/axi_crossbar_v2_1_23 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/bc0a/hdl/axi_crossbar_v2_1_vl_rfs.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_xbar_0/sim/system_xbar_0.v" \
-endlib
-makelib ies_lib/axi_sg_v4_1_13 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/4919/hdl/axi_sg_v4_1_rfs.vhd" \
-endlib
-makelib ies_lib/axi_dma_v7_1_23 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/89d8/hdl/axi_dma_v7_1_vh_rfs.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_axi_dma_0_0/sim/system_axi_dma_0_0.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ipshared/dd64/ad7606_axis.v" \
  "../../../bd/system/ipshared/dd64/ad7606_ser2par128bit.v" \
  "../../../bd/system/ipshared/dd64/adc_whole.v" \
  "../../../bd/system/ip/system_adc_whole_0_0/sim/system_adc_whole_0_0.v" \
-endlib
-makelib ies_lib/axis_infrastructure_v1_1_0 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/8713/hdl/axis_infrastructure_v1_1_vl_rfs.v" \
-endlib
-makelib ies_lib/axis_data_fifo_v2_0_4 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/abd4/hdl/axis_data_fifo_v2_0_vl_rfs.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_axis_data_fifo_0_0/sim/system_axis_data_fifo_0_0.v" \
  "../../../bd/system/ipshared/24ef/dac_spi.v" \
  "../../../bd/system/ipshared/24ef/dds_dac.v" \
  "../../../bd/system/ipshared/24ef/dma_axis_s.v" \
  "../../../bd/system/ipshared/24ef/dac_whole.v" \
  "../../../bd/system/ip/system_dac_whole_0_1/sim/system_dac_whole_0_1.v" \
  "../../../bd/system/ip/system_axis_data_fifo_1_1/sim/system_axis_data_fifo_1_1.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_proc_sys_reset_0_0/sim/system_proc_sys_reset_0_0.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_blk_mem_gen_0_0/sim/system_blk_mem_gen_0_0.v" \
-endlib
-makelib ies_lib/axi_bram_ctrl_v4_1_4 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/c9f0/hdl/axi_bram_ctrl_v4_1_rfs.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_axi_bram_ctrl_0_0/sim/system_axi_bram_ctrl_0_0.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ipshared/6fec/config2ser16bits.v" \
  "../../../bd/system/ipshared/6fec/onoff_io.v" \
  "../../../bd/system/ipshared/6fec/rd_from_serial8bit.v" \
  "../../../bd/system/ipshared/6fec/onoff_config_axi.v" \
  "../../../bd/system/ip/system_onoff_config_axi_0_0/sim/system_onoff_config_axi_0_0.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_rst_ps7_0_142M_0/sim/system_rst_ps7_0_142M_0.vhd" \
  "../../../bd/system/ip/system_axi_gpio_0_1/sim/system_axi_gpio_0_1.vhd" \
-endlib
-makelib ies_lib/axi_uartlite_v2_0_26 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/5edb/hdl/axi_uartlite_v2_0_vh_rfs.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_axi_uartlite_0_0/sim/system_axi_uartlite_0_0.vhd" \
-endlib
-makelib ies_lib/util_vector_logic_v2_0_1 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/3f90/hdl/util_vector_logic_v2_0_vl_rfs.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_util_vector_logic_0_0/sim/system_util_vector_logic_0_0.v" \
-endlib
-makelib ies_lib/axi_iic_v2_0_25 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/1529/hdl/axi_iic_v2_0_vh_rfs.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
  "../../../bd/system/ip/system_axi_iic_0_0/sim/system_axi_iic_0_0.vhd" \
-endlib
-makelib ies_lib/xil_defaultlib \
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
-endlib
-makelib ies_lib/axi_protocol_converter_v2_1_22 \
  "../../../../DanDike_NewBoard.srcs/sources_1/bd/system/ipshared/5cee/hdl/axi_protocol_converter_v2_1_vl_rfs.v" \
-endlib
-makelib ies_lib/xil_defaultlib \
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
-endlib
-makelib ies_lib/xil_defaultlib \
  glbl.v
-endlib

