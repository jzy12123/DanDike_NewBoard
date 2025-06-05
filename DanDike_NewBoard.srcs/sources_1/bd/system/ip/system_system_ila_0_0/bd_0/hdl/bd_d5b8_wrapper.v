//Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Command: generate_target bd_d5b8_wrapper.bd
//Design : bd_d5b8_wrapper
//Purpose: IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module bd_d5b8_wrapper
   (SLOT_0_ONOFF_onoff_cs,
    SLOT_0_ONOFF_onoff_sclk,
    SLOT_0_ONOFF_onoff_sdi,
    SLOT_0_ONOFF_onoff_sdo,
    clk,
    probe0);
  input SLOT_0_ONOFF_onoff_cs;
  input SLOT_0_ONOFF_onoff_sclk;
  input SLOT_0_ONOFF_onoff_sdi;
  input SLOT_0_ONOFF_onoff_sdo;
  input clk;
  input [0:0]probe0;

  wire SLOT_0_ONOFF_onoff_cs;
  wire SLOT_0_ONOFF_onoff_sclk;
  wire SLOT_0_ONOFF_onoff_sdi;
  wire SLOT_0_ONOFF_onoff_sdo;
  wire clk;
  wire [0:0]probe0;

  bd_d5b8 bd_d5b8_i
       (.SLOT_0_ONOFF_onoff_cs(SLOT_0_ONOFF_onoff_cs),
        .SLOT_0_ONOFF_onoff_sclk(SLOT_0_ONOFF_onoff_sclk),
        .SLOT_0_ONOFF_onoff_sdi(SLOT_0_ONOFF_onoff_sdi),
        .SLOT_0_ONOFF_onoff_sdo(SLOT_0_ONOFF_onoff_sdo),
        .clk(clk),
        .probe0(probe0));
endmodule
