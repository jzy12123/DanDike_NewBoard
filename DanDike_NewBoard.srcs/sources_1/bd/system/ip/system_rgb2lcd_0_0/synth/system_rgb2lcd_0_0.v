// (c) Copyright 1995-2025 Xilinx, Inc. All rights reserved.
// 
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
// 
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
// 
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
// 
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
// 
// DO NOT MODIFY THIS FILE.


// IP VLNV: openedv.com:user:rgb2lcd:1.0
// IP Revision: 3

(* X_CORE_INFO = "rgb2lcd,Vivado 2020.2" *)
(* CHECK_LICENSE_TYPE = "system_rgb2lcd_0_0,rgb2lcd,{}" *)
(* IP_DEFINITION_SOURCE = "package_project" *)
(* DowngradeIPIdentifiedWarnings = "yes" *)
module system_rgb2lcd_0_0 (
  vid_data,
  lcd_id_i,
  lcd_id_t,
  lcd_id_o,
  rgb_data_i,
  rgb_data_o,
  rgb_data_t
);

input wire [23 : 0] vid_data;
(* X_INTERFACE_INFO = "openedv.com:user:LCD_ID:1.0 lcd_id TRI_I" *)
input wire [2 : 0] lcd_id_i;
(* X_INTERFACE_INFO = "openedv.com:user:LCD_ID:1.0 lcd_id TRI_T" *)
input wire [2 : 0] lcd_id_t;
(* X_INTERFACE_INFO = "openedv.com:user:LCD_ID:1.0 lcd_id TRI_O" *)
output wire [2 : 0] lcd_id_o;
(* X_INTERFACE_INFO = "xilinx.com:interface:gpio:1.0 rgb_data TRI_I" *)
input wire [18 : 0] rgb_data_i;
(* X_INTERFACE_INFO = "xilinx.com:interface:gpio:1.0 rgb_data TRI_O" *)
output wire [18 : 0] rgb_data_o;
(* X_INTERFACE_INFO = "xilinx.com:interface:gpio:1.0 rgb_data TRI_T" *)
output wire [18 : 0] rgb_data_t;

  rgb2lcd #(
    .VID_IN_DATA_WIDTH(24),
    .VID_OUT_DATA_WIDTH(19)
  ) inst (
    .vid_data(vid_data),
    .lcd_id_i(lcd_id_i),
    .lcd_id_t(lcd_id_t),
    .lcd_id_o(lcd_id_o),
    .rgb_data_i(rgb_data_i),
    .rgb_data_o(rgb_data_o),
    .rgb_data_t(rgb_data_t)
  );
endmodule
