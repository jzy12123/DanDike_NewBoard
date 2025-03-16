`timescale 1ns / 1ps
///////////////////////////////////////////////////////////////////////////////////////////
//
//1.�������ڣ��룩=3600/�������ÿkWh����������x ���ع��ʣ�kW��
//2.�������尴50%ռ�ձ��������ˣ��������ĸߵ�ƽ�͵͵�ƽʱ��ֱ�Ϊ��ʽ���������ڵ�һ�롣
//3.�ߵ�ƽ�����ƽ������ʱ���ü���ֵpulse_width��ʾ������ֵ��1�൱���ӳ�10ns��100MHzʱ�ӣ���
//4.pulse_width��ֵ��CPU���㣬ͨ��AXI_Lite���߷�������
//5.enable��ʹ���źš�enableΪ1ʱ��pulse_out��������pulse_width����������壬����ֹͣ�����
//6.��������pulse_out��������У�pulse_width��������£�pulse_out��������������仯��
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
