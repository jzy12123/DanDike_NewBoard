`timescale 1ns / 1ps
///////////////////////////////////////////////////////////////////////////////////////////
//
//1.�������ڣ��룩=3600/�������ÿkWh����������x ���ع��ʣ�kW��
//2.�������尴50%ռ�ձ��������ˣ��������ĸߵ�ƽ�͵͵�ƽʱ��ֱ�Ϊ��ʽ���������ڵ�һ�롣
//3.�ߵ�ƽ����͵�ƽ������ʱ���ü���ֵpulse_width��ʾ������ֵ��1�൱���ӳ�10ns��100MHzʱ�ӣ���
//4.pulse_width��ֵ��CPU��ѯ��ȡ����������������Ч���ж��ź�������ȡ��
//5.enable��ʹ���źš�enableΪ1ʱ������ʶ���������Ŀ�ȣ�����ֹͣʶ��
//6.��������pulse_out��������У�pulse_width��������£�pulse_out��������������仯��
//
///////////////////////////////////////////////////////////////////////////////////////////
    module power_pulse_in(
        input             rst_n,           //low active
        input             clkin,           //The freqency is 100MHz
        input             enable,          //start 	
        input             pulse_in,		   //
        output reg [31:0] pulse_width,    //�������壨�ߵ�ƽ��������ʱ��  
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
