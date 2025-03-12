`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2024/03/01 09:42:29
// Design Name: 
// Module Name: rotary_encoder
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module rotary_encoder(
            input clkin,
            input sys_rst_n,
            input a_in,
            input b_in,
            output reg intrpt,
            output  [31:0] spd
            );

        parameter  idle  = 2'd0 ;
        parameter  s1    = 2'd1;
		parameter  s2    = 2'd2;
		parameter  s3    = 2'd3;

		reg  [1:0]  state;
	    reg    direction;
        reg  [30:0] count;

	assign  spd = {direction,count}; 

	always@(posedge clkin or negedge sys_rst_n )
		if(!sys_rst_n)
			begin
				state <= idle;
       			count <= 32'd0;
                direction <= 1'b0;
                intrpt <= 0;                 
           end
		else
		case(state)
			idle : 
					if(a_in^b_in)	begin state <= s1; direction <= a_in; end
											
			s1 : begin
					if(!(a_in|b_in)) begin state <= s2; count <= 31'd0; intrpt <= 1'b0; end
						else if( a_in^b_in)  state <= state;
							else state <= idle;
					end
			s2 : begin
					if(a_in^b_in) state <= s3;
						 else count <= count + 1;
					end	
			s3 : begin
					if(a_in&&b_in)  state <= idle;
                         else intrpt <= 1'b1;
                    end
          endcase
endmodule


