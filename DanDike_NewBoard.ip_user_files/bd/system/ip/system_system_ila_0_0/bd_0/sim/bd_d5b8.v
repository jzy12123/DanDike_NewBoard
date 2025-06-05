//Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Command: generate_target bd_d5b8.bd
//Design : bd_d5b8
//Purpose: IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

(* CORE_GENERATION_INFO = "bd_d5b8,IP_Integrator,{x_ipVendor=xilinx.com,x_ipLibrary=BlockDiagram,x_ipName=bd_d5b8,x_ipVersion=1.00.a,x_ipLanguage=VERILOG,numBlks=1,numReposBlks=1,numNonXlnxBlks=0,numHierBlks=0,maxHierDepth=0,numSysgenBlks=0,numHlsBlks=0,numHdlrefBlks=0,numPkgbdBlks=0,bdsource=SBD,synth_mode=Global}" *) (* HW_HANDOFF = "system_system_ila_0_0.hwdef" *) 
module bd_d5b8
   (SLOT_0_ONOFF_onoff_cs,
    SLOT_0_ONOFF_onoff_sclk,
    SLOT_0_ONOFF_onoff_sdi,
    SLOT_0_ONOFF_onoff_sdo,
    clk,
    probe0);
  (* X_INTERFACE_INFO = "xilinx.com:user:OnOff:1.0 SLOT_0_ONOFF onoff_cs" *) input SLOT_0_ONOFF_onoff_cs;
  (* X_INTERFACE_INFO = "xilinx.com:user:OnOff:1.0 SLOT_0_ONOFF onoff_sclk" *) input SLOT_0_ONOFF_onoff_sclk;
  (* X_INTERFACE_INFO = "xilinx.com:user:OnOff:1.0 SLOT_0_ONOFF onoff_sdi" *) input SLOT_0_ONOFF_onoff_sdi;
  (* X_INTERFACE_INFO = "xilinx.com:user:OnOff:1.0 SLOT_0_ONOFF onoff_sdo" *) input SLOT_0_ONOFF_onoff_sdo;
  (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 CLK.CLK CLK" *) (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME CLK.CLK, CLK_DOMAIN system_processing_system7_0_0_FCLK_CLK0, FREQ_HZ 1e+08, FREQ_TOLERANCE_HZ 0, INSERT_VIP 0, PHASE 0.000" *) input clk;
  input [0:0]probe0;

  wire SLOT_0_ONOFF_onoff_cs_1;
  wire SLOT_0_ONOFF_onoff_sclk_1;
  wire SLOT_0_ONOFF_onoff_sdi_1;
  wire SLOT_0_ONOFF_onoff_sdo_1;
  wire clk_1;
  wire [0:0]probe0_1;

  assign SLOT_0_ONOFF_onoff_cs_1 = SLOT_0_ONOFF_onoff_cs;
  assign SLOT_0_ONOFF_onoff_sclk_1 = SLOT_0_ONOFF_onoff_sclk;
  assign SLOT_0_ONOFF_onoff_sdi_1 = SLOT_0_ONOFF_onoff_sdi;
  assign SLOT_0_ONOFF_onoff_sdo_1 = SLOT_0_ONOFF_onoff_sdo;
  assign clk_1 = clk;
  assign probe0_1 = probe0[0];
  bd_d5b8_ila_lib_0 ila_lib
       (.clk(clk_1),
        .probe0(probe0_1),
        .probe1(SLOT_0_ONOFF_onoff_cs_1),
        .probe2(SLOT_0_ONOFF_onoff_sdi_1),
        .probe3(SLOT_0_ONOFF_onoff_sclk_1),
        .probe4(SLOT_0_ONOFF_onoff_sdo_1));
endmodule
