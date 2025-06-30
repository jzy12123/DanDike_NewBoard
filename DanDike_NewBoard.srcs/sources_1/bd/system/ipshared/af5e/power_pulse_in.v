`timescale 1ns / 1ps
///////////////////////////////////////////////////////////////////////////////////////////
//
//1.脉冲周期（秒）=3600/电表常数（每kWh的脉冲数）x 负载功率（kW）
//2.电能脉冲按50%占空比输出，因此，输出脉冲的高电平和低电平时间分别为上式中脉冲周期的一半。
//3.高电平（或低电平）持续时间用计数值pulse_width表示；计数值加1相当于延长10ns（100MHz时钟）。
//4.pulse_width的值由CPU查询读取，或者由上升沿有效的中断信号启动读取。
//5.enable是使能信号。enable为1时，启动识别电能脉冲的宽度，否则，停止识别。
//6.电能脉冲pulse_out输出过程中，pulse_width如果被更新，pulse_out的脉冲立即跟随变化。
//
///////////////////////////////////////////////////////////////////////////////////////////
    module power_pulse_in(
        input             rst_n,           //low active
        input             clkin,           //The freqency is 100MHz
        input             enable,          //start 	
        input             pulse_in,		   //
        output reg [31:0] pulse_width,    //电能脉冲（高电平）持续的时间  
        output  reg      intrpt           //interrupt,9 cycles of clkin,posedge valid 
        );

    //finding out the pulse edges 
	reg   [1:0] pulse_dly;
	wire        pulse_posedge;
	wire        pulse_negedge;
    
    always@(posedge clkin)
        if(!rst_n)
                pulse_dly <= 2'b00;
           else
                pulse_dly <= {pulse_dly[0],pulse_in};
   
    assign pulse_posedge = ~pulse_dly[1] &&  pulse_dly[0];
    assign pulse_negedge =  pulse_dly[1] && ~pulse_dly[0];  

    //
    reg   [31:0]  counter;  
    reg           pulse_state;	
	
    always@(posedge clkin)
        if(!rst_n) begin
                counter <= 32'd0;
                pulse_state <= 1'b0;
				pulse_width <= 32'd0;
               end
 
        else  begin	
        if (enable)
         case(pulse_state) 
		   1'b0 : if(pulse_posedge == 1'b1) begin
			             counter <= 32'd1;
                         pulse_state   <= 1'b1 ;
                        end										
		   1'b1 : if(pulse_negedge == 1'b1) begin
                        pulse_width <= counter;
                        pulse_state <= 1'b0;
                       end
                     else begin
                            counter <= counter + 1;
                          end
             endcase     
		  else begin
                counter <= 32'd0;
                pulse_state <= 1'b0;
				pulse_width <= 32'd0;
               end    
      end
     
      //generating a interrupt signal
     reg   [3:0]  intrpt_count;  
     reg          intrpt_state;	
	
    always@(posedge clkin)
        if(!rst_n) begin
                intrpt_count <= 4'd0;
                intrpt_state <= 1'b0;
				intrpt       <= 1'd0;
               end
        else  begin	
          if (enable)            
           case(intrpt_state)
             1'b0 : if(pulse_negedge) begin
                           intrpt_state <= 1'b1;
				           intrpt       <= 1'd1;
                         end                       
                     else begin
                           intrpt_count <= 4'd0;
                           intrpt_state <= 1'b0;
				           intrpt       <= 1'd0;
                         end          
             1'b1 : if(intrpt_count[3]) begin
                           intrpt_count <= 4'd0;
                           intrpt_state <= 1'b0;
				           intrpt       <= 1'd0;
                         end                       
                     else 
                       intrpt_count <= intrpt_count + 1;
              endcase
           else begin
                intrpt_count <= 4'd0;
                intrpt_state <= 1'b0;
				intrpt       <= 1'd0;
               end
           end          
      
   endmodule //power_pulse_in
