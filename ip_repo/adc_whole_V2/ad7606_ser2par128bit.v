`timescale 1ns / 10ps
//-------------------------------------------------------------------------------------------------//
// 1. 主时钟为12.5MHz，周期80ns。                                                                  //
// 2. yad_cnv上升沿到yad_cs下降沿时间为60个clkin周期；yad_cs下降沿到rd_en上升沿为64个clkin周期。   //
// 3. 一次ADC的启动信号为conv_start;rd_en为高电平时读取数据。                                      //
// 4. 一次完整的转换与读取周期最大为9.7us；所以，ADC最大采样速率为2048x50=102.4kSPS。              //
//------------------------------------------------------------------------------------------------ //

module ad7606_ser2par128bit

   (
    //FPGA interface	
	input           clkin,           // 12.5MHz main clock,generally 10MHz~17MHz
    input           rst_n,
    input           conv_start,      // High state must continued more than 80ns(one 12.5MHz cycle)
    output  reg     rd_en, 
    output [127:0]  dout,            // channel 1 to channel 8 from MSB to LSB

	//ADDC interfaces
    output     yad_rst,
    output reg yad_cvn,
    output reg yad_cs,
    output     yad_ck,
    input      yad_sa,
    input      yad_sb
    );
   
  // 

parameter  idle  = 2'd0;
parameter  s1    = 2'd1;
parameter  s2    = 2'd2;
parameter  s3    = 2'd3;

reg    [1:0]  state;
reg    [5:0]  count2;
reg           count2_end; 
reg    [5:0]  count1;
reg           count1_end;            
reg    [63:0] data_a, data_b;  
	 
always@(posedge clkin or negedge rst_n )
if(!rst_n)
    begin
        state    <= idle;
        yad_cs   <= 1'b1;
        yad_cvn  <= 1'b0;
        rd_en    <= 0;    
          end
else
case(state)
idle :  if(conv_start)    begin 
         state <= s1; yad_cvn <= 1; rd_en<=0; end
                                    
s1 :  //delay 60 clkin  
        if (count2_end)   begin
		 state <= s2; yad_cvn <= 0;  end  
		 
s2 :	 begin state <= s3; yad_cs <= 0;    end 	 

s3 :    if(count1_end)     begin 
         state <= idle; yad_cs <= 1; rd_en <=1; end

endcase

//-----------------------------------------------
always@(posedge clkin or negedge rst_n)
if(!rst_n) begin
      count2      <= 6'd0;
      count2_end  <= 1'b0; 
      end
else if(yad_cvn) begin
     if (count2==6'd58)begin
                     count2     <= 6'd0;
                     count2_end <= 1'b1; end
                else begin
                     count2     <= count2 + 1;
                     count2_end <= 1'b0;  end	 
	   end
//-------------------------------------------------
assign yad_ck = yad_cs?1'b1:clkin;

always@(negedge yad_ck or negedge rst_n)
if(!rst_n) begin
      count1      <= 6'd0;
      count1_end  <= 1'b0; 
      end
else if(count1==6'd63)begin
                     count1 <= 6'd0;
                     count1_end <= 1'b1; end
                else begin
                     count1 <= count1 + 1;
                     count1_end <= 1'b0;  end	 
	 
//------------------------------------------------	 
    always@(negedge yad_ck or negedge rst_n)
       if(!rst_n) begin
             data_a <= 64'd0;
             data_b <= 64'd0;
             end
          else    begin
             data_a <= {data_a[62:0],yad_sa};
             data_b <= {data_b[62:0],yad_sb};
             end
    
	assign  dout = {data_a[63:32],data_a[31:0],data_b[63:32],data_b[31:0]};
   
    //
    assign yad_rst = ~rst_n;
    // 

endmodule