module rgb2lcd #(
    //parameter define
    parameter    VID_IN_DATA_WIDTH = 24,
    parameter    VID_OUT_DATA_WIDTH = 19  // 6 bits for red, 7 bits for green, 6 bits for blue
)(
    //video input
    input [VID_IN_DATA_WIDTH-1:0]  vid_data,

    //video out
    //lcd id
    input  [2:0]                  lcd_id_i, 
    input  [2:0]                  lcd_id_t,
    output [2:0]                  lcd_id_o,
    
    //LCD��������Ϊ˫������,�ĳ���̬���ŵ���ʽ  
    input  [VID_OUT_DATA_WIDTH-1:0]   rgb_data_i,
    output [VID_OUT_DATA_WIDTH-1:0]  rgb_data_o,
    output [VID_OUT_DATA_WIDTH-1:0]  rgb_data_t
);

//*****************************************************
//**                  main code
//*****************************************************

// ��RGB888��ʽת��ΪRGB676��ʽ
wire [5:0] red_6bit   = vid_data[23:18]; // ȡ��ɫͨ���ĸ�6λ
wire [6:0] green_7bit = vid_data[15:9];  // ȡ��ɫͨ���ĸ�7λ
wire [5:0] blue_6bit  = vid_data[7:2];   // ȡ��ɫͨ���ĸ�6λ

// ��ת�����RGB676��ʽ�ϲ���19λ����
wire [VID_OUT_DATA_WIDTH-1:0] rgb676_data = {red_6bit, green_7bit, blue_6bit};

// ��ȡLCD ID
assign lcd_id_o[0] = (lcd_id_t[0]==1'b1) ? rgb676_data[18] : 1'bz; // R6���λ
assign lcd_id_o[1] = (lcd_id_t[1]==1'b1) ? rgb676_data[12] : 1'bz; // G7���λ
assign lcd_id_o[2] = (lcd_id_t[2]==1'b1) ? rgb676_data[5]  : 1'bz; // B6���λ

// �����������
assign rgb_data_o  = (lcd_id_t[0]==1'b1) ? {VID_OUT_DATA_WIDTH{1'bz}} : rgb676_data;
assign rgb_data_t  = {VID_OUT_DATA_WIDTH{lcd_id_t[0]}};

endmodule
