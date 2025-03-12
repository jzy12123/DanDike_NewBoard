//****************************************Copyright (c)***********************************//
//ԭ�Ӹ����߽�ѧƽ̨��www.yuanzige.com
//����֧�֣�www.openedv.com
//�Ա����̣�http://openedv.taobao.com
//��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡZYNQ & FPGA & STM32 & LINUX���ϡ�
//��Ȩ���У�����ؾ���
//Copyright(C) ����ԭ�� 2018-2028
//All rights reserved
//----------------------------------------------------------------------------------------
// File name:           main
// Last modified Date:  2019/10/8 17:25:36
// Last Version:        V1.0
// Descriptions:        LCD����ʾ����
//----------------------------------------------------------------------------------------
// Created by:          ����ԭ��
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

//�궨��
#define BYTES_PIXEL        3                          //�����ֽ�����RGB888ռ3���ֽ�
#define DYNCLK_BASEADDR    XPAR_AXI_DYNCLK_0_BASEADDR //��̬ʱ�ӻ���ַ
#define VDMA_ID            XPAR_AXIVDMA_0_DEVICE_ID   //VDMA����ID
#define DISP_VTC_ID        XPAR_VTC_0_DEVICE_ID       //VTC����ID
#define AXI_GPIO_0_ID      XPAR_AXI_GPIO_0_DEVICE_ID  //PL��  AXI GPIO 0(lcd_id)����ID
#define AXI_GPIO_0_CHANEL  1                          //PL����ʹ��AXI GPIO(lcd_id)ͨ��1

//��������
void colorbar(u8 *frame, u32 width, u32 height, u32 stride);

//ȫ�ֱ���
XAxiVdma     vdma;
DisplayCtrl  dispCtrl;
XGpio        axi_gpio_inst;   //PL�� AXI GPIO ����ʵ��
VideoMode    vd_mode;
//frame buffer����ʼ��ַ
unsigned int const frame_buffer_addr = (XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1000000);
unsigned int lcd_id=0;        //LCD ID

int main(void)
{
	XGpio_Initialize(&axi_gpio_inst,AXI_GPIO_0_ID);                //GPIO��ʼ��
	XGpio_SetDataDirection(&axi_gpio_inst,AXI_GPIO_0_CHANEL,0x07); //����AXI GPIOΪ����
	lcd_id = LTDC_PanelID_Read(&axi_gpio_inst,AXI_GPIO_0_CHANEL);  //��ȡLCD��ID
	XGpio_SetDataDirection(&axi_gpio_inst,AXI_GPIO_0_CHANEL,0x00); //����AXI GPIOΪ���
	xil_printf("LCD ID: %x\r\n",lcd_id);

	//���ݻ�ȡ��LCD��ID��������video������ѡ��
	switch(lcd_id){
		case 0x4342 : vd_mode = VMODE_480x272; break;  //4.3����,480*272�ֱ���
		case 0x4384 : vd_mode = VMODE_800x480; break;  //4.3����,800*480�ֱ���
		case 0x7084 : vd_mode = VMODE_800x480; break;  //7����,800*480�ֱ���
		case 0x7016 : vd_mode = VMODE_1024x600; break; //7����,1024*600�ֱ���
		case 0x1018 : vd_mode = VMODE_1280x800; break; //10.1����,1280*800�ֱ���
		default : vd_mode = VMODE_800x480; break;
	}

	//����VDMA
	run_vdma_frame_buffer(&vdma, VDMA_ID, vd_mode.width, vd_mode.height,
							frame_buffer_addr,0, 0,ONLY_READ);

    //��ʼ��Display controller
	DisplayInitialize(&dispCtrl, DISP_VTC_ID, DYNCLK_BASEADDR);
    //����VideoMode
	DisplaySetMode(&dispCtrl, &vd_mode);
	DisplayStart(&dispCtrl);

	//д����
	colorbar((u8*)frame_buffer_addr, vd_mode.width,
			vd_mode.height, vd_mode.width*BYTES_PIXEL);
    return 0;
}

//д�����������ʺ�ɫ��
void colorbar(u8 *frame, u32 width, u32 height, u32 stride)
{
	u32 color_edge;
	u32 x_pos, y_pos;
	u32 y_stride = 0;
	u8 rgb_r, rgb_b, rgb_g;

	color_edge = width * BYTES_PIXEL / 7;
	for (y_pos = 0; y_pos < height; y_pos++) {
		for (x_pos = 0; x_pos < (width * BYTES_PIXEL); x_pos += BYTES_PIXEL) {
			if (x_pos < color_edge) {                                           //��ɫ
				rgb_r = 0xFF;
				rgb_g = 0;
				rgb_b = 0;
			} else if ((x_pos >= color_edge) && (x_pos < color_edge * 2)) {     //��ɫ
				rgb_r = 0xFF;
				rgb_g = 0x7F;
				rgb_b = 0;
			} else if ((x_pos >= color_edge * 2) && (x_pos < color_edge * 3)) { //��ɫ
				rgb_r = 0xFF;
				rgb_g = 0xFF;
				rgb_b = 0;
			} else if ((x_pos >= color_edge * 3) && (x_pos < color_edge * 4)) { //��ɫ
				rgb_r = 0;
				rgb_g = 0xFF;
				rgb_b = 0;
			} else if ((x_pos >= color_edge * 4) && (x_pos < color_edge * 5)) { //��ɫ
				rgb_r = 0;
				rgb_g = 0xFF;
				rgb_b = 0xFF;
			} else if ((x_pos >= color_edge * 5) && (x_pos < color_edge * 6)) { //��ɫ
				rgb_r = 0;
				rgb_g = 0;
				rgb_b = 0xFF;
			} else if ((x_pos >= color_edge * 6) && (x_pos < color_edge * 7)) { //��ɫ
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
	Xil_DCacheFlush();     //ˢ��Cache�����ݸ�����DDR3��
	xil_printf("show color bar\r\n");
}
