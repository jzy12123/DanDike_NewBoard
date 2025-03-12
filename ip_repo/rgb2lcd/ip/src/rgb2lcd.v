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
    
    //LCD数据引脚为双向引脚,改成三态引脚的形式  
    input  [VID_OUT_DATA_WIDTH-1:0]   rgb_data_i,
    output [VID_OUT_DATA_WIDTH-1:0]  rgb_data_o,
    output [VID_OUT_DATA_WIDTH-1:0]  rgb_data_t
);

//*****************************************************
//**                  main code
//*****************************************************

// 将RGB888格式转换为RGB676格式
wire [5:0] red_6bit   = vid_data[23:18]; // 取红色通道的高6位
wire [6:0] green_7bit = vid_data[15:9];  // 取绿色通道的高7位
wire [5:0] blue_6bit  = vid_data[7:2];   // 取蓝色通道的高6位

// 将转换后的RGB676格式合并成19位数据
wire [VID_OUT_DATA_WIDTH-1:0] rgb676_data = {red_6bit, green_7bit, blue_6bit};

// 读取LCD ID
assign lcd_id_o[0] = (lcd_id_t[0]==1'b1) ? rgb676_data[18] : 1'bz; // R6最高位
assign lcd_id_o[1] = (lcd_id_t[1]==1'b1) ? rgb676_data[12] : 1'bz; // G7最高位
assign lcd_id_o[2] = (lcd_id_t[2]==1'b1) ? rgb676_data[5]  : 1'bz; // B6最高位

// 处理输出数据
assign rgb_data_o  = (lcd_id_t[0]==1'b1) ? {VID_OUT_DATA_WIDTH{1'bz}} : rgb676_data;
assign rgb_data_t  = {VID_OUT_DATA_WIDTH{lcd_id_t[0]}};

endmodule
