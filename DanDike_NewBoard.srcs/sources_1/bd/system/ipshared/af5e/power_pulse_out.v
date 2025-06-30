`timescale 1ns / 1ps
///////////////////////////////////////////////////////////////////////////////////////////
//
//1.脉冲周期（秒）=3600/电表常数（每kWh的脉冲数）x 负载功率（kW）
//2.电能脉冲按50%占空比输出，因此，输出脉冲的高电平和低电平时间分别为上式中脉冲周期的一半。
//3.高电平（或电平）持续时间用计数值pulse_width表示；计数值加1相当于延长10ns（100MHz时钟）。
//4.pulse_width的值由CPU计算，通过AXI_Lite总线发过来。
//5.enable是使能信号。enable为1时，pulse_out按给定的pulse_width输出电能脉冲，否则，停止输出。
//6.电能脉冲pulse_out输出过程中，pulse_width如果被更新，pulse_out的脉冲立即跟随变化。
//
///////////////////////////////////////////////////////////////////////////////////////////
    module power_pulse_out(
        input          rst_n,           //low active
        input          clkin,           //The freqency is 100MHz
        input   [31:0] pulse_width,     //high or low level longth  
        input          enable,          //start output
        output  reg    pulse_out
        );
     
    reg           pulse_turn;
    reg   [31:0]  counter;  
     
    always@(posedge clkin)
        if(!rst_n) begin
               counter    <= 32'd0;
               pulse_turn <= 1'b0;
               end
           else if (enable) begin
                   if(counter >= (pulse_width-1)) begin
                             counter    <= 32'd0;
                             pulse_turn <= 1'b1;
                             end
                         else begin
                               counter    <= counter + 1;
                               pulse_turn <= 1'b0;
                              end
					     end		  
                   else begin
                        counter     <= 32'd0;
                         pulse_turn <= 1'b0;
                        end

     always@(posedge clkin)
	    if(!rst_n)
            pulse_out <= 1'b0;			
           else if(enable) begin
		         if(pulse_turn)
                     pulse_out <= ~pulse_out;
					end
                  else				 
                     pulse_out <= 1'b0; 
					 
   endmodule //power_pulse_out
