//Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2020.2 (win64) Build 3064766 Wed Nov 18 09:12:45 MST 2020
//Date        : Wed Mar 26 15:11:08 2025
//Host        : DESKTOP-L4NOM67 running 64-bit major release  (build 9200)
//Command     : generate_target system_wrapper.bd
//Design      : system_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module system_wrapper
   (AD_0_ad_busy,
    AD_0_ad_ck,
    AD_0_ad_cs,
    AD_0_ad_cvn,
    AD_0_ad_rst,
    AD_0_ad_sa,
    AD_0_ad_sb,
    Coder_A,
    Coder_B,
    Coder_Int,
    DA_0_da_clk,
    DA_0_da_cs,
    DA_0_da_sdo,
    DDR_addr,
    DDR_ba,
    DDR_cas_n,
    DDR_ck_n,
    DDR_ck_p,
    DDR_cke,
    DDR_cs_n,
    DDR_dm,
    DDR_dq,
    DDR_dqs_n,
    DDR_dqs_p,
    DDR_odt,
    DDR_ras_n,
    DDR_reset_n,
    DDR_we_n,
    FIXED_IO_ddr_vrn,
    FIXED_IO_ddr_vrp,
    FIXED_IO_mio,
    FIXED_IO_ps_clk,
    FIXED_IO_ps_porb,
    FIXED_IO_ps_srstb,
    GPIO_BEEP_tri_o,
    IIC_LCD_0_scl_io,
    IIC_LCD_0_sda_io,
    KeyBoard_IIC_scl_io,
    KeyBoard_IIC_sda_io,
    MDIO_ETHERNET_1_0_mdc,
    MDIO_ETHERNET_1_0_mdio_io,
    OnOff_0_onoff_cs,
    OnOff_0_onoff_sclk,
    OnOff_0_onoff_sdi,
    OnOff_0_onoff_sdo,
    RGMII_0_rd,
    RGMII_0_rx_ctl,
    RGMII_0_rxc,
    RGMII_0_td,
    RGMII_0_tx_ctl,
    RGMII_0_txc,
    RdSerial_0_rd_load,
    RdSerial_0_rd_sclk,
    RdSerial_0_rd_sdi,
    UART_GPS_rxd,
    UART_GPS_txd,
    WrSerial_0_wr_load,
    WrSerial_0_wr_sclk,
    WrSerial_0_wr_sdo,
    gpio0_tri_io,
    lcd_bl,
    lcd_clk,
    lcd_de,
    lcd_hsync,
    lcd_vsync,
    pulse_p_in_0,
    pulse_p_out_0,
    pulse_q_in_0,
    pulse_q_out_0,
    rgb_data_tri_io,
    yad_os_0);
  input AD_0_ad_busy;
  output AD_0_ad_ck;
  output AD_0_ad_cs;
  output AD_0_ad_cvn;
  output AD_0_ad_rst;
  input AD_0_ad_sa;
  input AD_0_ad_sb;
  input Coder_A;
  input Coder_B;
  input [0:0]Coder_Int;
  output DA_0_da_clk;
  output DA_0_da_cs;
  output [7:0]DA_0_da_sdo;
  inout [14:0]DDR_addr;
  inout [2:0]DDR_ba;
  inout DDR_cas_n;
  inout DDR_ck_n;
  inout DDR_ck_p;
  inout DDR_cke;
  inout DDR_cs_n;
  inout [3:0]DDR_dm;
  inout [31:0]DDR_dq;
  inout [3:0]DDR_dqs_n;
  inout [3:0]DDR_dqs_p;
  inout DDR_odt;
  inout DDR_ras_n;
  inout DDR_reset_n;
  inout DDR_we_n;
  inout FIXED_IO_ddr_vrn;
  inout FIXED_IO_ddr_vrp;
  inout [53:0]FIXED_IO_mio;
  inout FIXED_IO_ps_clk;
  inout FIXED_IO_ps_porb;
  inout FIXED_IO_ps_srstb;
  output [0:0]GPIO_BEEP_tri_o;
  inout IIC_LCD_0_scl_io;
  inout IIC_LCD_0_sda_io;
  inout KeyBoard_IIC_scl_io;
  inout KeyBoard_IIC_sda_io;
  output MDIO_ETHERNET_1_0_mdc;
  inout MDIO_ETHERNET_1_0_mdio_io;
  output OnOff_0_onoff_cs;
  output OnOff_0_onoff_sclk;
  input OnOff_0_onoff_sdi;
  output OnOff_0_onoff_sdo;
  input [3:0]RGMII_0_rd;
  input RGMII_0_rx_ctl;
  input RGMII_0_rxc;
  output [3:0]RGMII_0_td;
  output RGMII_0_tx_ctl;
  output RGMII_0_txc;
  output RdSerial_0_rd_load;
  output RdSerial_0_rd_sclk;
  input RdSerial_0_rd_sdi;
  input UART_GPS_rxd;
  output UART_GPS_txd;
  output WrSerial_0_wr_load;
  output WrSerial_0_wr_sclk;
  output WrSerial_0_wr_sdo;
  inout [2:0]gpio0_tri_io;
  output [0:0]lcd_bl;
  output lcd_clk;
  output lcd_de;
  output lcd_hsync;
  output lcd_vsync;
  input pulse_p_in_0;
  output pulse_p_out_0;
  input pulse_q_in_0;
  output pulse_q_out_0;
  inout [18:0]rgb_data_tri_io;
  output [1:0]yad_os_0;

  wire AD_0_ad_busy;
  wire AD_0_ad_ck;
  wire AD_0_ad_cs;
  wire AD_0_ad_cvn;
  wire AD_0_ad_rst;
  wire AD_0_ad_sa;
  wire AD_0_ad_sb;
  wire Coder_A;
  wire Coder_B;
  wire [0:0]Coder_Int;
  wire DA_0_da_clk;
  wire DA_0_da_cs;
  wire [7:0]DA_0_da_sdo;
  wire [14:0]DDR_addr;
  wire [2:0]DDR_ba;
  wire DDR_cas_n;
  wire DDR_ck_n;
  wire DDR_ck_p;
  wire DDR_cke;
  wire DDR_cs_n;
  wire [3:0]DDR_dm;
  wire [31:0]DDR_dq;
  wire [3:0]DDR_dqs_n;
  wire [3:0]DDR_dqs_p;
  wire DDR_odt;
  wire DDR_ras_n;
  wire DDR_reset_n;
  wire DDR_we_n;
  wire FIXED_IO_ddr_vrn;
  wire FIXED_IO_ddr_vrp;
  wire [53:0]FIXED_IO_mio;
  wire FIXED_IO_ps_clk;
  wire FIXED_IO_ps_porb;
  wire FIXED_IO_ps_srstb;
  wire [0:0]GPIO_BEEP_tri_o;
  wire IIC_LCD_0_scl_i;
  wire IIC_LCD_0_scl_io;
  wire IIC_LCD_0_scl_o;
  wire IIC_LCD_0_scl_t;
  wire IIC_LCD_0_sda_i;
  wire IIC_LCD_0_sda_io;
  wire IIC_LCD_0_sda_o;
  wire IIC_LCD_0_sda_t;
  wire KeyBoard_IIC_scl_i;
  wire KeyBoard_IIC_scl_io;
  wire KeyBoard_IIC_scl_o;
  wire KeyBoard_IIC_scl_t;
  wire KeyBoard_IIC_sda_i;
  wire KeyBoard_IIC_sda_io;
  wire KeyBoard_IIC_sda_o;
  wire KeyBoard_IIC_sda_t;
  wire MDIO_ETHERNET_1_0_mdc;
  wire MDIO_ETHERNET_1_0_mdio_i;
  wire MDIO_ETHERNET_1_0_mdio_io;
  wire MDIO_ETHERNET_1_0_mdio_o;
  wire MDIO_ETHERNET_1_0_mdio_t;
  wire OnOff_0_onoff_cs;
  wire OnOff_0_onoff_sclk;
  wire OnOff_0_onoff_sdi;
  wire OnOff_0_onoff_sdo;
  wire [3:0]RGMII_0_rd;
  wire RGMII_0_rx_ctl;
  wire RGMII_0_rxc;
  wire [3:0]RGMII_0_td;
  wire RGMII_0_tx_ctl;
  wire RGMII_0_txc;
  wire RdSerial_0_rd_load;
  wire RdSerial_0_rd_sclk;
  wire RdSerial_0_rd_sdi;
  wire UART_GPS_rxd;
  wire UART_GPS_txd;
  wire WrSerial_0_wr_load;
  wire WrSerial_0_wr_sclk;
  wire WrSerial_0_wr_sdo;
  wire [0:0]gpio0_tri_i_0;
  wire [1:1]gpio0_tri_i_1;
  wire [2:2]gpio0_tri_i_2;
  wire [0:0]gpio0_tri_io_0;
  wire [1:1]gpio0_tri_io_1;
  wire [2:2]gpio0_tri_io_2;
  wire [0:0]gpio0_tri_o_0;
  wire [1:1]gpio0_tri_o_1;
  wire [2:2]gpio0_tri_o_2;
  wire [0:0]gpio0_tri_t_0;
  wire [1:1]gpio0_tri_t_1;
  wire [2:2]gpio0_tri_t_2;
  wire [0:0]lcd_bl;
  wire lcd_clk;
  wire lcd_de;
  wire lcd_hsync;
  wire lcd_vsync;
  wire pulse_p_in_0;
  wire pulse_p_out_0;
  wire pulse_q_in_0;
  wire pulse_q_out_0;
  wire [0:0]rgb_data_tri_i_0;
  wire [1:1]rgb_data_tri_i_1;
  wire [10:10]rgb_data_tri_i_10;
  wire [11:11]rgb_data_tri_i_11;
  wire [12:12]rgb_data_tri_i_12;
  wire [13:13]rgb_data_tri_i_13;
  wire [14:14]rgb_data_tri_i_14;
  wire [15:15]rgb_data_tri_i_15;
  wire [16:16]rgb_data_tri_i_16;
  wire [17:17]rgb_data_tri_i_17;
  wire [18:18]rgb_data_tri_i_18;
  wire [2:2]rgb_data_tri_i_2;
  wire [3:3]rgb_data_tri_i_3;
  wire [4:4]rgb_data_tri_i_4;
  wire [5:5]rgb_data_tri_i_5;
  wire [6:6]rgb_data_tri_i_6;
  wire [7:7]rgb_data_tri_i_7;
  wire [8:8]rgb_data_tri_i_8;
  wire [9:9]rgb_data_tri_i_9;
  wire [0:0]rgb_data_tri_io_0;
  wire [1:1]rgb_data_tri_io_1;
  wire [10:10]rgb_data_tri_io_10;
  wire [11:11]rgb_data_tri_io_11;
  wire [12:12]rgb_data_tri_io_12;
  wire [13:13]rgb_data_tri_io_13;
  wire [14:14]rgb_data_tri_io_14;
  wire [15:15]rgb_data_tri_io_15;
  wire [16:16]rgb_data_tri_io_16;
  wire [17:17]rgb_data_tri_io_17;
  wire [18:18]rgb_data_tri_io_18;
  wire [2:2]rgb_data_tri_io_2;
  wire [3:3]rgb_data_tri_io_3;
  wire [4:4]rgb_data_tri_io_4;
  wire [5:5]rgb_data_tri_io_5;
  wire [6:6]rgb_data_tri_io_6;
  wire [7:7]rgb_data_tri_io_7;
  wire [8:8]rgb_data_tri_io_8;
  wire [9:9]rgb_data_tri_io_9;
  wire [0:0]rgb_data_tri_o_0;
  wire [1:1]rgb_data_tri_o_1;
  wire [10:10]rgb_data_tri_o_10;
  wire [11:11]rgb_data_tri_o_11;
  wire [12:12]rgb_data_tri_o_12;
  wire [13:13]rgb_data_tri_o_13;
  wire [14:14]rgb_data_tri_o_14;
  wire [15:15]rgb_data_tri_o_15;
  wire [16:16]rgb_data_tri_o_16;
  wire [17:17]rgb_data_tri_o_17;
  wire [18:18]rgb_data_tri_o_18;
  wire [2:2]rgb_data_tri_o_2;
  wire [3:3]rgb_data_tri_o_3;
  wire [4:4]rgb_data_tri_o_4;
  wire [5:5]rgb_data_tri_o_5;
  wire [6:6]rgb_data_tri_o_6;
  wire [7:7]rgb_data_tri_o_7;
  wire [8:8]rgb_data_tri_o_8;
  wire [9:9]rgb_data_tri_o_9;
  wire [0:0]rgb_data_tri_t_0;
  wire [1:1]rgb_data_tri_t_1;
  wire [10:10]rgb_data_tri_t_10;
  wire [11:11]rgb_data_tri_t_11;
  wire [12:12]rgb_data_tri_t_12;
  wire [13:13]rgb_data_tri_t_13;
  wire [14:14]rgb_data_tri_t_14;
  wire [15:15]rgb_data_tri_t_15;
  wire [16:16]rgb_data_tri_t_16;
  wire [17:17]rgb_data_tri_t_17;
  wire [18:18]rgb_data_tri_t_18;
  wire [2:2]rgb_data_tri_t_2;
  wire [3:3]rgb_data_tri_t_3;
  wire [4:4]rgb_data_tri_t_4;
  wire [5:5]rgb_data_tri_t_5;
  wire [6:6]rgb_data_tri_t_6;
  wire [7:7]rgb_data_tri_t_7;
  wire [8:8]rgb_data_tri_t_8;
  wire [9:9]rgb_data_tri_t_9;
  wire [1:0]yad_os_0;

  IOBUF IIC_LCD_0_scl_iobuf
       (.I(IIC_LCD_0_scl_o),
        .IO(IIC_LCD_0_scl_io),
        .O(IIC_LCD_0_scl_i),
        .T(IIC_LCD_0_scl_t));
  IOBUF IIC_LCD_0_sda_iobuf
       (.I(IIC_LCD_0_sda_o),
        .IO(IIC_LCD_0_sda_io),
        .O(IIC_LCD_0_sda_i),
        .T(IIC_LCD_0_sda_t));
  IOBUF KeyBoard_IIC_scl_iobuf
       (.I(KeyBoard_IIC_scl_o),
        .IO(KeyBoard_IIC_scl_io),
        .O(KeyBoard_IIC_scl_i),
        .T(KeyBoard_IIC_scl_t));
  IOBUF KeyBoard_IIC_sda_iobuf
       (.I(KeyBoard_IIC_sda_o),
        .IO(KeyBoard_IIC_sda_io),
        .O(KeyBoard_IIC_sda_i),
        .T(KeyBoard_IIC_sda_t));
  IOBUF MDIO_ETHERNET_1_0_mdio_iobuf
       (.I(MDIO_ETHERNET_1_0_mdio_o),
        .IO(MDIO_ETHERNET_1_0_mdio_io),
        .O(MDIO_ETHERNET_1_0_mdio_i),
        .T(MDIO_ETHERNET_1_0_mdio_t));
  IOBUF gpio0_tri_iobuf_0
       (.I(gpio0_tri_o_0),
        .IO(gpio0_tri_io[0]),
        .O(gpio0_tri_i_0),
        .T(gpio0_tri_t_0));
  IOBUF gpio0_tri_iobuf_1
       (.I(gpio0_tri_o_1),
        .IO(gpio0_tri_io[1]),
        .O(gpio0_tri_i_1),
        .T(gpio0_tri_t_1));
  IOBUF gpio0_tri_iobuf_2
       (.I(gpio0_tri_o_2),
        .IO(gpio0_tri_io[2]),
        .O(gpio0_tri_i_2),
        .T(gpio0_tri_t_2));
  IOBUF rgb_data_tri_iobuf_0
       (.I(rgb_data_tri_o_0),
        .IO(rgb_data_tri_io[0]),
        .O(rgb_data_tri_i_0),
        .T(rgb_data_tri_t_0));
  IOBUF rgb_data_tri_iobuf_1
       (.I(rgb_data_tri_o_1),
        .IO(rgb_data_tri_io[1]),
        .O(rgb_data_tri_i_1),
        .T(rgb_data_tri_t_1));
  IOBUF rgb_data_tri_iobuf_10
       (.I(rgb_data_tri_o_10),
        .IO(rgb_data_tri_io[10]),
        .O(rgb_data_tri_i_10),
        .T(rgb_data_tri_t_10));
  IOBUF rgb_data_tri_iobuf_11
       (.I(rgb_data_tri_o_11),
        .IO(rgb_data_tri_io[11]),
        .O(rgb_data_tri_i_11),
        .T(rgb_data_tri_t_11));
  IOBUF rgb_data_tri_iobuf_12
       (.I(rgb_data_tri_o_12),
        .IO(rgb_data_tri_io[12]),
        .O(rgb_data_tri_i_12),
        .T(rgb_data_tri_t_12));
  IOBUF rgb_data_tri_iobuf_13
       (.I(rgb_data_tri_o_13),
        .IO(rgb_data_tri_io[13]),
        .O(rgb_data_tri_i_13),
        .T(rgb_data_tri_t_13));
  IOBUF rgb_data_tri_iobuf_14
       (.I(rgb_data_tri_o_14),
        .IO(rgb_data_tri_io[14]),
        .O(rgb_data_tri_i_14),
        .T(rgb_data_tri_t_14));
  IOBUF rgb_data_tri_iobuf_15
       (.I(rgb_data_tri_o_15),
        .IO(rgb_data_tri_io[15]),
        .O(rgb_data_tri_i_15),
        .T(rgb_data_tri_t_15));
  IOBUF rgb_data_tri_iobuf_16
       (.I(rgb_data_tri_o_16),
        .IO(rgb_data_tri_io[16]),
        .O(rgb_data_tri_i_16),
        .T(rgb_data_tri_t_16));
  IOBUF rgb_data_tri_iobuf_17
       (.I(rgb_data_tri_o_17),
        .IO(rgb_data_tri_io[17]),
        .O(rgb_data_tri_i_17),
        .T(rgb_data_tri_t_17));
  IOBUF rgb_data_tri_iobuf_18
       (.I(rgb_data_tri_o_18),
        .IO(rgb_data_tri_io[18]),
        .O(rgb_data_tri_i_18),
        .T(rgb_data_tri_t_18));
  IOBUF rgb_data_tri_iobuf_2
       (.I(rgb_data_tri_o_2),
        .IO(rgb_data_tri_io[2]),
        .O(rgb_data_tri_i_2),
        .T(rgb_data_tri_t_2));
  IOBUF rgb_data_tri_iobuf_3
       (.I(rgb_data_tri_o_3),
        .IO(rgb_data_tri_io[3]),
        .O(rgb_data_tri_i_3),
        .T(rgb_data_tri_t_3));
  IOBUF rgb_data_tri_iobuf_4
       (.I(rgb_data_tri_o_4),
        .IO(rgb_data_tri_io[4]),
        .O(rgb_data_tri_i_4),
        .T(rgb_data_tri_t_4));
  IOBUF rgb_data_tri_iobuf_5
       (.I(rgb_data_tri_o_5),
        .IO(rgb_data_tri_io[5]),
        .O(rgb_data_tri_i_5),
        .T(rgb_data_tri_t_5));
  IOBUF rgb_data_tri_iobuf_6
       (.I(rgb_data_tri_o_6),
        .IO(rgb_data_tri_io[6]),
        .O(rgb_data_tri_i_6),
        .T(rgb_data_tri_t_6));
  IOBUF rgb_data_tri_iobuf_7
       (.I(rgb_data_tri_o_7),
        .IO(rgb_data_tri_io[7]),
        .O(rgb_data_tri_i_7),
        .T(rgb_data_tri_t_7));
  IOBUF rgb_data_tri_iobuf_8
       (.I(rgb_data_tri_o_8),
        .IO(rgb_data_tri_io[8]),
        .O(rgb_data_tri_i_8),
        .T(rgb_data_tri_t_8));
  IOBUF rgb_data_tri_iobuf_9
       (.I(rgb_data_tri_o_9),
        .IO(rgb_data_tri_io[9]),
        .O(rgb_data_tri_i_9),
        .T(rgb_data_tri_t_9));
  system system_i
       (.AD_0_ad_busy(AD_0_ad_busy),
        .AD_0_ad_ck(AD_0_ad_ck),
        .AD_0_ad_cs(AD_0_ad_cs),
        .AD_0_ad_cvn(AD_0_ad_cvn),
        .AD_0_ad_rst(AD_0_ad_rst),
        .AD_0_ad_sa(AD_0_ad_sa),
        .AD_0_ad_sb(AD_0_ad_sb),
        .Coder_A(Coder_A),
        .Coder_B(Coder_B),
        .Coder_Int(Coder_Int),
        .DA_0_da_clk(DA_0_da_clk),
        .DA_0_da_cs(DA_0_da_cs),
        .DA_0_da_sdo(DA_0_da_sdo),
        .DDR_addr(DDR_addr),
        .DDR_ba(DDR_ba),
        .DDR_cas_n(DDR_cas_n),
        .DDR_ck_n(DDR_ck_n),
        .DDR_ck_p(DDR_ck_p),
        .DDR_cke(DDR_cke),
        .DDR_cs_n(DDR_cs_n),
        .DDR_dm(DDR_dm),
        .DDR_dq(DDR_dq),
        .DDR_dqs_n(DDR_dqs_n),
        .DDR_dqs_p(DDR_dqs_p),
        .DDR_odt(DDR_odt),
        .DDR_ras_n(DDR_ras_n),
        .DDR_reset_n(DDR_reset_n),
        .DDR_we_n(DDR_we_n),
        .FIXED_IO_ddr_vrn(FIXED_IO_ddr_vrn),
        .FIXED_IO_ddr_vrp(FIXED_IO_ddr_vrp),
        .FIXED_IO_mio(FIXED_IO_mio),
        .FIXED_IO_ps_clk(FIXED_IO_ps_clk),
        .FIXED_IO_ps_porb(FIXED_IO_ps_porb),
        .FIXED_IO_ps_srstb(FIXED_IO_ps_srstb),
        .GPIO_BEEP_tri_o(GPIO_BEEP_tri_o),
        .IIC_LCD_0_scl_i(IIC_LCD_0_scl_i),
        .IIC_LCD_0_scl_o(IIC_LCD_0_scl_o),
        .IIC_LCD_0_scl_t(IIC_LCD_0_scl_t),
        .IIC_LCD_0_sda_i(IIC_LCD_0_sda_i),
        .IIC_LCD_0_sda_o(IIC_LCD_0_sda_o),
        .IIC_LCD_0_sda_t(IIC_LCD_0_sda_t),
        .KeyBoard_IIC_scl_i(KeyBoard_IIC_scl_i),
        .KeyBoard_IIC_scl_o(KeyBoard_IIC_scl_o),
        .KeyBoard_IIC_scl_t(KeyBoard_IIC_scl_t),
        .KeyBoard_IIC_sda_i(KeyBoard_IIC_sda_i),
        .KeyBoard_IIC_sda_o(KeyBoard_IIC_sda_o),
        .KeyBoard_IIC_sda_t(KeyBoard_IIC_sda_t),
        .MDIO_ETHERNET_1_0_mdc(MDIO_ETHERNET_1_0_mdc),
        .MDIO_ETHERNET_1_0_mdio_i(MDIO_ETHERNET_1_0_mdio_i),
        .MDIO_ETHERNET_1_0_mdio_o(MDIO_ETHERNET_1_0_mdio_o),
        .MDIO_ETHERNET_1_0_mdio_t(MDIO_ETHERNET_1_0_mdio_t),
        .OnOff_0_onoff_cs(OnOff_0_onoff_cs),
        .OnOff_0_onoff_sclk(OnOff_0_onoff_sclk),
        .OnOff_0_onoff_sdi(OnOff_0_onoff_sdi),
        .OnOff_0_onoff_sdo(OnOff_0_onoff_sdo),
        .RGMII_0_rd(RGMII_0_rd),
        .RGMII_0_rx_ctl(RGMII_0_rx_ctl),
        .RGMII_0_rxc(RGMII_0_rxc),
        .RGMII_0_td(RGMII_0_td),
        .RGMII_0_tx_ctl(RGMII_0_tx_ctl),
        .RGMII_0_txc(RGMII_0_txc),
        .RdSerial_0_rd_load(RdSerial_0_rd_load),
        .RdSerial_0_rd_sclk(RdSerial_0_rd_sclk),
        .RdSerial_0_rd_sdi(RdSerial_0_rd_sdi),
        .UART_GPS_rxd(UART_GPS_rxd),
        .UART_GPS_txd(UART_GPS_txd),
        .WrSerial_0_wr_load(WrSerial_0_wr_load),
        .WrSerial_0_wr_sclk(WrSerial_0_wr_sclk),
        .WrSerial_0_wr_sdo(WrSerial_0_wr_sdo),
        .gpio0_tri_i({gpio0_tri_i_2,gpio0_tri_i_1,gpio0_tri_i_0}),
        .gpio0_tri_o({gpio0_tri_o_2,gpio0_tri_o_1,gpio0_tri_o_0}),
        .gpio0_tri_t({gpio0_tri_t_2,gpio0_tri_t_1,gpio0_tri_t_0}),
        .lcd_bl(lcd_bl),
        .lcd_clk(lcd_clk),
        .lcd_de(lcd_de),
        .lcd_hsync(lcd_hsync),
        .lcd_vsync(lcd_vsync),
        .pulse_p_in_0(pulse_p_in_0),
        .pulse_p_out_0(pulse_p_out_0),
        .pulse_q_in_0(pulse_q_in_0),
        .pulse_q_out_0(pulse_q_out_0),
        .rgb_data_tri_i({rgb_data_tri_i_18,rgb_data_tri_i_17,rgb_data_tri_i_16,rgb_data_tri_i_15,rgb_data_tri_i_14,rgb_data_tri_i_13,rgb_data_tri_i_12,rgb_data_tri_i_11,rgb_data_tri_i_10,rgb_data_tri_i_9,rgb_data_tri_i_8,rgb_data_tri_i_7,rgb_data_tri_i_6,rgb_data_tri_i_5,rgb_data_tri_i_4,rgb_data_tri_i_3,rgb_data_tri_i_2,rgb_data_tri_i_1,rgb_data_tri_i_0}),
        .rgb_data_tri_o({rgb_data_tri_o_18,rgb_data_tri_o_17,rgb_data_tri_o_16,rgb_data_tri_o_15,rgb_data_tri_o_14,rgb_data_tri_o_13,rgb_data_tri_o_12,rgb_data_tri_o_11,rgb_data_tri_o_10,rgb_data_tri_o_9,rgb_data_tri_o_8,rgb_data_tri_o_7,rgb_data_tri_o_6,rgb_data_tri_o_5,rgb_data_tri_o_4,rgb_data_tri_o_3,rgb_data_tri_o_2,rgb_data_tri_o_1,rgb_data_tri_o_0}),
        .rgb_data_tri_t({rgb_data_tri_t_18,rgb_data_tri_t_17,rgb_data_tri_t_16,rgb_data_tri_t_15,rgb_data_tri_t_14,rgb_data_tri_t_13,rgb_data_tri_t_12,rgb_data_tri_t_11,rgb_data_tri_t_10,rgb_data_tri_t_9,rgb_data_tri_t_8,rgb_data_tri_t_7,rgb_data_tri_t_6,rgb_data_tri_t_5,rgb_data_tri_t_4,rgb_data_tri_t_3,rgb_data_tri_t_2,rgb_data_tri_t_1,rgb_data_tri_t_0}),
        .yad_os_0(yad_os_0));
endmodule
