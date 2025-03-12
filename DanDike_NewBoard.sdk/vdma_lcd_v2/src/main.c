//****************************************Copyright (c)***********************************//
//原子哥在线教学平台：www.yuanzige.com
//技术支持：www.openedv.com
//淘宝店铺：http://openedv.taobao.com
//关注微信公众平台微信号："正点原子"，免费获取ZYNQ & FPGA & STM32 & LINUX资料。
//版权所有，盗版必究。
//Copyright(C) 正点原子 2018-2028
//All rights reserved
//----------------------------------------------------------------------------------------
// File name:           main
// Last modified Date:  2019/10/8 17:25:36
// Last Version:        V1.0
// Descriptions:        LCD屏显示彩条
//----------------------------------------------------------------------------------------
// Created by:          正点原子
// Created date:        2019/10/8 17:25:36
// Version:             V1.0
// Descriptions:        The original version
//
//----------------------------------------------------------------------------------------
//****************************************************************************************//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xil_types.h"
#include "xil_cache.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xaxivdma.h"
#include "xaxivdma_i.h"
#include "display_ctrl/display_ctrl.h"
#include "vdma_api/vdma_api.h"

//宏定义
#define BYTES_PIXEL        3                          //像素字节数，RGB888占3个字节
#define DYNCLK_BASEADDR    XPAR_AXI_DYNCLK_0_BASEADDR //动态时钟基地址
#define VDMA_ID            XPAR_AXIVDMA_0_DEVICE_ID   //VDMA器件ID
#define DISP_VTC_ID        XPAR_VTC_0_DEVICE_ID       //VTC器件ID
#define AXI_GPIO_0_ID      XPAR_AXI_GPIO_0_DEVICE_ID  //PL端  AXI GPIO 0(lcd_id)器件ID
#define AXI_GPIO_0_CHANEL  1                          //PL按键使用AXI GPIO(lcd_id)通道1

//函数声明
void colorbar(u8 *frame, u32 width, u32 height, u32 stride);

//全局变量
XAxiVdma     vdma;
DisplayCtrl  dispCtrl;
XGpio        axi_gpio_inst;   //PL端 AXI GPIO 驱动实例
VideoMode    vd_mode;
//frame buffer的起始地址
unsigned int const frame_buffer_addr = (XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1000000);
unsigned int lcd_id=0;        //LCD ID

int main(void)
{
	XGpio_Initialize(&axi_gpio_inst,AXI_GPIO_0_ID);                //GPIO初始化
	XGpio_SetDataDirection(&axi_gpio_inst,AXI_GPIO_0_CHANEL,0x07); //设置AXI GPIO为输入
	lcd_id = LTDC_PanelID_Read(&axi_gpio_inst,AXI_GPIO_0_CHANEL);  //获取LCD的ID
	XGpio_SetDataDirection(&axi_gpio_inst,AXI_GPIO_0_CHANEL,0x00); //设置AXI GPIO为输出
	xil_printf("LCD ID: %x\r\n",lcd_id);

	//根据获取的LCD的ID号来进行video参数的选择
	switch(lcd_id){
		case 0x4342 : vd_mode = VMODE_480x272; break;  //4.3寸屏,480*272分辨率
		case 0x4384 : vd_mode = VMODE_800x480; break;  //4.3寸屏,800*480分辨率
		case 0x7084 : vd_mode = VMODE_800x480; break;  //7寸屏,800*480分辨率
		case 0x7016 : vd_mode = VMODE_1024x600; break; //7寸屏,1024*600分辨率
		case 0x1018 : vd_mode = VMODE_1280x800; break; //10.1寸屏,1280*800分辨率
		default : vd_mode = VMODE_800x480; break;
	}

	//配置VDMA
	run_vdma_frame_buffer(&vdma, VDMA_ID, vd_mode.width, vd_mode.height,
							frame_buffer_addr,0, 0,ONLY_READ);

    //初始化Display controller
	DisplayInitialize(&dispCtrl, DISP_VTC_ID, DYNCLK_BASEADDR);
    //设置VideoMode
	DisplaySetMode(&dispCtrl, &vd_mode);
	DisplayStart(&dispCtrl);

	//写彩条
	colorbar((u8*)frame_buffer_addr, vd_mode.width,
			vd_mode.height, vd_mode.width*BYTES_PIXEL);
    return 0;
}

//写彩条函数（彩虹色）
void colorbar(u8 *frame, u32 width, u32 height, u32 stride)
{
	u32 color_edge;
	u32 x_pos, y_pos;
	u32 y_stride = 0;
	u8 rgb_r, rgb_b, rgb_g;

	color_edge = width * BYTES_PIXEL / 7;
	for (y_pos = 0; y_pos < height; y_pos++) {
		for (x_pos = 0; x_pos < (width * BYTES_PIXEL); x_pos += BYTES_PIXEL) {
			if (x_pos < color_edge) {                                           //红色
				rgb_r = 0xFF;
				rgb_g = 0;
				rgb_b = 0;
			} else if ((x_pos >= color_edge) && (x_pos < color_edge * 2)) {     //橙色
				rgb_r = 0xFF;
				rgb_g = 0x7F;
				rgb_b = 0;
			} else if ((x_pos >= color_edge * 2) && (x_pos < color_edge * 3)) { //黄色
				rgb_r = 0xFF;
				rgb_g = 0xFF;
				rgb_b = 0;
			} else if ((x_pos >= color_edge * 3) && (x_pos < color_edge * 4)) { //绿色
				rgb_r = 0;
				rgb_g = 0xFF;
				rgb_b = 0;
			} else if ((x_pos >= color_edge * 4) && (x_pos < color_edge * 5)) { //青色
				rgb_r = 0;
				rgb_g = 0xFF;
				rgb_b = 0xFF;
			} else if ((x_pos >= color_edge * 5) && (x_pos < color_edge * 6)) { //蓝色
				rgb_r = 0;
				rgb_g = 0;
				rgb_b = 0xFF;
			} else if ((x_pos >= color_edge * 6) && (x_pos < color_edge * 7)) { //紫色
				rgb_r = 0x8B;
				rgb_g = 0;
				rgb_b = 0xFF;
			}
			frame[x_pos + y_stride + 0] = rgb_b;
			frame[x_pos + y_stride + 1] = rgb_g;
			frame[x_pos + y_stride + 2] = rgb_r;
		}
		y_stride += stride;
	}
	Xil_DCacheFlush();     //刷新Cache，数据更新至DDR3中
	xil_printf("show color bar\r\n");
}
