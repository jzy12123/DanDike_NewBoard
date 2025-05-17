
# =============================================================================
# AD模块 (模数转换器) 相关引脚
# =============================================================================
set_property PACKAGE_PIN P15 [get_ports AD_0_ad_busy]
set_property PACKAGE_PIN R16 [get_ports AD_0_ad_ck]
set_property PACKAGE_PIN R17 [get_ports AD_0_ad_cs]
set_property PACKAGE_PIN R19 [get_ports AD_0_ad_cvn]
set_property PACKAGE_PIN P18 [get_ports AD_0_ad_rst]
set_property PACKAGE_PIN T17 [get_ports AD_0_ad_sa]
set_property PACKAGE_PIN R18 [get_ports AD_0_ad_sb]
set_property PACKAGE_PIN N17 [get_ports {yad_os_0[0]}]
set_property PACKAGE_PIN P19 [get_ports {yad_os_0[1]}]

set_property IOSTANDARD LVCMOS33 [get_ports AD_0_ad_busy]
set_property IOSTANDARD LVCMOS33 [get_ports AD_0_ad_ck]
set_property IOSTANDARD LVCMOS33 [get_ports AD_0_ad_cs]
set_property IOSTANDARD LVCMOS33 [get_ports AD_0_ad_cvn]
set_property IOSTANDARD LVCMOS33 [get_ports AD_0_ad_rst]
set_property IOSTANDARD LVCMOS33 [get_ports AD_0_ad_sa]
set_property IOSTANDARD LVCMOS33 [get_ports AD_0_ad_sb]
set_property IOSTANDARD LVCMOS33 [get_ports {yad_os_0[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {yad_os_0[1]}]

# =============================================================================
# DA模块 (数模转换器) 相关引脚
# =============================================================================
set_property PACKAGE_PIN W19 [get_ports {DA_0_da_sdo[0]}]
set_property PACKAGE_PIN V20 [get_ports {DA_0_da_sdo[1]}]
set_property PACKAGE_PIN T20 [get_ports {DA_0_da_sdo[2]}]
set_property PACKAGE_PIN N20 [get_ports {DA_0_da_sdo[3]}]
set_property PACKAGE_PIN W20 [get_ports {DA_0_da_sdo[4]}]
set_property PACKAGE_PIN U19 [get_ports {DA_0_da_sdo[5]}]
set_property PACKAGE_PIN T19 [get_ports {DA_0_da_sdo[6]}]
set_property PACKAGE_PIN U20 [get_ports {DA_0_da_sdo[7]}]
set_property PACKAGE_PIN N18 [get_ports DA_0_da_clk]
set_property PACKAGE_PIN P20 [get_ports DA_0_da_cs]

set_property IOSTANDARD LVCMOS33 [get_ports {DA_0_da_sdo[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {DA_0_da_sdo[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {DA_0_da_sdo[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {DA_0_da_sdo[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {DA_0_da_sdo[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {DA_0_da_sdo[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {DA_0_da_sdo[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {DA_0_da_sdo[7]}]
set_property IOSTANDARD LVCMOS33 [get_ports DA_0_da_clk]
set_property IOSTANDARD LVCMOS33 [get_ports DA_0_da_cs]

# =============================================================================
# LCD显示屏相关引脚
# =============================================================================
set_property PACKAGE_PIN V6 [get_ports lcd_clk]
set_property PACKAGE_PIN U5 [get_ports lcd_de]
set_property PACKAGE_PIN V5 [get_ports lcd_hsync]
set_property PACKAGE_PIN T5 [get_ports lcd_vsync]
set_property PACKAGE_PIN U7 [get_ports {lcd_bl[0]}]

set_property IOSTANDARD LVCMOS33 [get_ports lcd_clk]
set_property IOSTANDARD LVCMOS33 [get_ports lcd_de]
set_property IOSTANDARD LVCMOS33 [get_ports lcd_hsync]
set_property IOSTANDARD LVCMOS33 [get_ports lcd_vsync]
set_property IOSTANDARD LVCMOS33 [get_ports {lcd_bl[0]}]

# =============================================================================
# GPIO相关引脚
# =============================================================================
set_property PACKAGE_PIN T9 [get_ports {gpio0_tri_io[0]}]
set_property PACKAGE_PIN V7 [get_ports {gpio0_tri_io[1]}]
set_property PACKAGE_PIN U9 [get_ports {gpio0_tri_io[2]}]
set_property PACKAGE_PIN U14 [get_ports {GPIO_BEEP_tri_o[0]}]

set_property IOSTANDARD LVCMOS33 [get_ports {gpio0_tri_io[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio0_tri_io[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio0_tri_io[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {GPIO_BEEP_tri_o[0]}]

# =============================================================================
# RGB数据相关引脚
# =============================================================================
set_property PACKAGE_PIN W9 [get_ports {rgb_data_tri_io[0]}]
set_property PACKAGE_PIN Y9 [get_ports {rgb_data_tri_io[1]}]
set_property PACKAGE_PIN Y8 [get_ports {rgb_data_tri_io[2]}]
set_property PACKAGE_PIN Y7 [get_ports {rgb_data_tri_io[3]}]
set_property PACKAGE_PIN Y6 [get_ports {rgb_data_tri_io[4]}]
set_property PACKAGE_PIN W6 [get_ports {rgb_data_tri_io[5]}]
set_property PACKAGE_PIN Y11 [get_ports {rgb_data_tri_io[6]}]
set_property PACKAGE_PIN W11 [get_ports {rgb_data_tri_io[7]}]
set_property PACKAGE_PIN Y13 [get_ports {rgb_data_tri_io[8]}]
set_property PACKAGE_PIN Y12 [get_ports {rgb_data_tri_io[9]}]
set_property PACKAGE_PIN V11 [get_ports {rgb_data_tri_io[10]}]
set_property PACKAGE_PIN V10 [get_ports {rgb_data_tri_io[11]}]
set_property PACKAGE_PIN W10 [get_ports {rgb_data_tri_io[12]}]
set_property PACKAGE_PIN W15 [get_ports {rgb_data_tri_io[13]}]
set_property PACKAGE_PIN V15 [get_ports {rgb_data_tri_io[14]}]
set_property PACKAGE_PIN Y14 [get_ports {rgb_data_tri_io[15]}]
set_property PACKAGE_PIN W14 [get_ports {rgb_data_tri_io[16]}]
set_property PACKAGE_PIN W13 [get_ports {rgb_data_tri_io[17]}]
set_property PACKAGE_PIN V12 [get_ports {rgb_data_tri_io[18]}]

set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[7]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[8]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[9]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[10]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[11]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[12]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[13]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[14]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[15]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[16]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[17]}]
set_property IOSTANDARD LVCMOS33 [get_ports {rgb_data_tri_io[18]}]

# =============================================================================
# 编码器相关引脚
# =============================================================================
set_property PACKAGE_PIN T15 [get_ports Coder_A]
set_property PACKAGE_PIN T14 [get_ports Coder_B]
set_property PACKAGE_PIN U15 [get_ports {Coder_Int[0]}]

set_property IOSTANDARD LVCMOS33 [get_ports Coder_A]
set_property IOSTANDARD LVCMOS33 [get_ports Coder_B]
set_property IOSTANDARD LVCMOS33 [get_ports {Coder_Int[0]}]

# =============================================================================
# OnOff模块相关引脚
# =============================================================================
set_property PACKAGE_PIN T11 [get_ports OnOff_0_onoff_cs]
set_property PACKAGE_PIN T12 [get_ports OnOff_0_onoff_sclk]
set_property PACKAGE_PIN U10 [get_ports OnOff_0_onoff_sdi]
set_property PACKAGE_PIN U12 [get_ports OnOff_0_onoff_sdo]

set_property IOSTANDARD LVCMOS33 [get_ports OnOff_0_onoff_cs]
set_property IOSTANDARD LVCMOS33 [get_ports OnOff_0_onoff_sclk]
set_property IOSTANDARD LVCMOS33 [get_ports OnOff_0_onoff_sdi]
set_property IOSTANDARD LVCMOS33 [get_ports OnOff_0_onoff_sdo]

# =============================================================================
# 读写串行接口相关引脚
# =============================================================================
set_property PACKAGE_PIN Y17 [get_ports RdSerial_0_rd_load]
set_property PACKAGE_PIN Y16 [get_ports RdSerial_0_rd_sclk]
set_property PACKAGE_PIN W16 [get_ports RdSerial_0_rd_sdi]
set_property PACKAGE_PIN V18 [get_ports WrSerial_0_wr_load]
set_property PACKAGE_PIN Y19 [get_ports WrSerial_0_wr_sclk]
set_property PACKAGE_PIN Y18 [get_ports WrSerial_0_wr_sdo]

set_property IOSTANDARD LVCMOS33 [get_ports RdSerial_0_rd_load]
set_property IOSTANDARD LVCMOS33 [get_ports RdSerial_0_rd_sclk]
set_property IOSTANDARD LVCMOS33 [get_ports RdSerial_0_rd_sdi]
set_property IOSTANDARD LVCMOS33 [get_ports WrSerial_0_wr_load]
set_property IOSTANDARD LVCMOS33 [get_ports WrSerial_0_wr_sclk]
set_property IOSTANDARD LVCMOS33 [get_ports WrSerial_0_wr_sdo]

# =============================================================================
# UART GPS相关引脚
# =============================================================================
set_property PACKAGE_PIN L14 [get_ports UART_GPS_rxd]
set_property PACKAGE_PIN N16 [get_ports UART_GPS_txd]

set_property IOSTANDARD LVCMOS33 [get_ports UART_GPS_rxd]
set_property IOSTANDARD LVCMOS33 [get_ports UART_GPS_txd]

# =============================================================================
# RGMII网络接口相关引脚
# =============================================================================
set_property PACKAGE_PIN H16 [get_ports {RGMII_0_rd[3]}]
set_property PACKAGE_PIN H17 [get_ports {RGMII_0_rd[2]}]
set_property PACKAGE_PIN A20 [get_ports {RGMII_0_rd[1]}]
set_property PACKAGE_PIN B19 [get_ports {RGMII_0_rd[0]}]
set_property PACKAGE_PIN D20 [get_ports {RGMII_0_td[3]}]
set_property PACKAGE_PIN D19 [get_ports {RGMII_0_td[2]}]
set_property PACKAGE_PIN C20 [get_ports {RGMII_0_td[1]}]
set_property PACKAGE_PIN D18 [get_ports {RGMII_0_td[0]}]
set_property PACKAGE_PIN E17 [get_ports RGMII_0_rx_ctl]
set_property PACKAGE_PIN K17 [get_ports RGMII_0_rxc]
set_property PACKAGE_PIN K18 [get_ports RGMII_0_tx_ctl]
set_property PACKAGE_PIN B20 [get_ports RGMII_0_txc]

set_property IOSTANDARD LVCMOS33 [get_ports {RGMII_0_rd[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGMII_0_rd[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGMII_0_rd[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGMII_0_rd[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGMII_0_td[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGMII_0_td[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGMII_0_td[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGMII_0_td[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports RGMII_0_rx_ctl]
set_property IOSTANDARD LVCMOS33 [get_ports RGMII_0_rxc]
set_property IOSTANDARD LVCMOS33 [get_ports RGMII_0_tx_ctl]
set_property IOSTANDARD LVCMOS33 [get_ports RGMII_0_txc]

# =============================================================================
# MDIO以太网管理接口相关引脚
# =============================================================================
set_property PACKAGE_PIN F20 [get_ports MDIO_ETHERNET_1_0_mdc]
set_property PACKAGE_PIN F19 [get_ports MDIO_ETHERNET_1_0_mdio_io]

set_property IOSTANDARD LVCMOS33 [get_ports MDIO_ETHERNET_1_0_mdc]
set_property IOSTANDARD LVCMOS33 [get_ports MDIO_ETHERNET_1_0_mdio_io]

# =============================================================================
# I2C接口相关引脚
# =============================================================================
# LCD I2C接口
set_property PACKAGE_PIN U8 [get_ports IIC_LCD_0_scl_io]
set_property PACKAGE_PIN V8 [get_ports IIC_LCD_0_sda_io]

set_property IOSTANDARD LVCMOS33 [get_ports IIC_LCD_0_scl_io]
set_property IOSTANDARD LVCMOS33 [get_ports IIC_LCD_0_sda_io]

# 键盘I2C接口
set_property PACKAGE_PIN P14 [get_ports KeyBoard_IIC_scl_io]
set_property PACKAGE_PIN R14 [get_ports KeyBoard_IIC_sda_io]
set_property PACKAGE_PIN V13 [get_ports {key_BoardINT0[0]}]


set_property IOSTANDARD LVCMOS33 [get_ports KeyBoard_IIC_scl_io]
set_property IOSTANDARD LVCMOS33 [get_ports KeyBoard_IIC_sda_io]
set_property IOSTANDARD LVCMOS33 [get_ports {key_BoardINT0[0]}]
# =============================================================================
# 脉冲相关引脚
# =============================================================================
set_property PACKAGE_PIN M14 [get_ports pulse_p_in_0]
set_property PACKAGE_PIN J14 [get_ports pulse_p_out_0]
set_property PACKAGE_PIN M15 [get_ports pulse_q_in_0]
set_property PACKAGE_PIN L16 [get_ports pulse_q_out_0]

set_property IOSTANDARD LVCMOS33 [get_ports pulse_p_in_0]
set_property IOSTANDARD LVCMOS33 [get_ports pulse_p_out_0]
set_property IOSTANDARD LVCMOS33 [get_ports pulse_q_in_0]
set_property IOSTANDARD LVCMOS33 [get_ports pulse_q_out_0]



# =============================================================================
# 额外配置（上拉、驱动强度等）
# =============================================================================
# I2C上拉配置
set_property PULLUP true [get_ports IIC_LCD_0_scl_io]
set_property PULLUP true [get_ports IIC_LCD_0_sda_io]
set_property PULLUP true [get_ports KeyBoard_IIC_scl_io]
set_property PULLUP true [get_ports KeyBoard_IIC_sda_io]

# GPIO上拉配置
set_property PULLUP true [get_ports {gpio0_tri_io[1]}]
set_property PULLUP true [get_ports {gpio0_tri_io[2]}]

# 编码器中断上拉
set_property PULLUP true [get_ports {Coder_Int[0]}]

# 驱动强度和转换速率配置
set_property SLEW SLOW [get_ports KeyBoard_IIC_scl_io]
set_property SLEW FAST [get_ports IIC_LCD_0_scl_io]
set_property SLEW FAST [get_ports IIC_LCD_0_sda_io]
set_property DRIVE 12 [get_ports KeyBoard_IIC_sda_io]




# =============================================================================
# 测试
# =============================================================================




set_property PACKAGE_PIN H15 [get_ports RTCEEPROM_IIC_scl_io]
set_property PACKAGE_PIN J15 [get_ports RTCEEPROM_IIC_sda_io]
set_property IOSTANDARD LVCMOS33 [get_ports RTCEEPROM_IIC_scl_io]
set_property IOSTANDARD LVCMOS33 [get_ports RTCEEPROM_IIC_sda_io]
set_property PULLUP true [get_ports RTCEEPROM_IIC_scl_io]
set_property PULLUP true [get_ports RTCEEPROM_IIC_sda_io]

set_property PULLUP true [get_ports AD_0_ad_sa]
set_property PULLUP true [get_ports AD_0_ad_sb]
set_property PULLDOWN true [get_ports AD_0_ad_busy]
