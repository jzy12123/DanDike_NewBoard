//
`timescale 1 ns / 1 ps
//
module dac_spi(
    input rst_n,
    input clkin,      //The freqency is same as sclk,25MHz
    input [127:0] datain,
    input dac_en,      //about 50x1024Hz
    input [7:0] select, 
    output reg cs_n,
    output sclk,
    output [7:0] sdo
    );
 
parameter  idle   = 1'b0;
parameter start   = 1'b1;

reg           state;
reg   [3:0]   clk_cnt;
reg   [15:0]  data0_r;  
reg   [15:0]  data1_r;  
reg   [15:0]  data2_r;  
reg   [15:0]  data3_r;  
reg   [15:0]  data4_r;  
reg   [15:0]  data5_r;  
reg   [15:0]  data6_r;  
reg   [15:0]  data7_r;  
wire  [7:0]   sdo_raw;

always@(negedge clkin )
if(!rst_n) begin
	cs_n    <= 1;
	state   <= idle;
	clk_cnt <= 4'b0;
	data0_r <= 16'd0;
	data1_r <= 16'd0;
	data2_r <= 16'd0;
	data3_r <= 16'd0;
	data4_r <= 16'd0;
	data5_r <= 16'd0;
	data6_r <= 16'd0;
	data7_r <= 16'd0;
	end
else case(state)
	idle:   if(dac_en) begin
			    state   <= start;
			    clk_cnt <= 4'd0;
			    cs_n    <= 0;
			    data0_r <= datain[15:0];
			    data1_r <= datain[31:16];
			    data2_r <= datain[47:32];
			    data3_r <= datain[63:48];
			    data4_r <= datain[79:64];
			    data5_r <= datain[95:80];
			    data6_r <= datain[111:96];
			    data7_r <= datain[127:112];
				end
		else      begin
			    cs_n = 1;
                  end

         start:  if(clk_cnt == 4'd15) begin
                                      state <= idle;
		                              cs_n  <= 1;
		                              clk_cnt <= 4'd0;
                                      end
                          else  begin
                           clk_cnt <= clk_cnt + 1;
                          data0_r  <= {data0_r[14:0],1'b0};
                          data1_r  <= {data1_r[14:0],1'b0};
                          data2_r  <= {data2_r[14:0],1'b0};
                          data3_r  <= {data3_r[14:0],1'b0};
                          data4_r  <= {data4_r[14:0],1'b0};
                          data5_r  <= {data5_r[14:0],1'b0};
                          data6_r  <= {data6_r[14:0],1'b0};
                          data7_r  <= {data7_r[14:0],1'b0};
			              state    <= start;                             
		                  end
          endcase
      assign    sclk    = cs_n?0:clkin;		
      assign    sdo_raw = {data7_r[15],data6_r[15],data5_r[15],data4_r[15],
                       data3_r[15],data2_r[15],data1_r[15],data0_r[15]};  
      assign   sdo      = select & sdo_raw; 
      
endmodule //dac_spi
