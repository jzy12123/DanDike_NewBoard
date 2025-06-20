`timescale 1ns / 1ps

module rd_from_serial8bit(
   //FPGA interface
    input clkin,        //25MHz
    input rst_n,
    input enable,
    output reg [7:0] dataout,
    //ADDC board interface
    output reg load_n,   //load parallel datas when low actively,LD_R 
    output sclk,         //CLK_R
    input sdi            //DATA_R
    );
   
     reg            state     ;
     reg   [7:0]   datain_reg;  
     reg   [2:0]    clk_cnt   ;
//
always@(negedge clkin)
if(!rst_n) begin
	load_n = 0;
	state = 1'b0;
	clk_cnt = 3'd0;
	end
else case(state)
	1'b0:   if(enable) begin
			    state = 1'b1;	
			    clk_cnt = 3'd0;
			    load_n = 1;
			end
		else      begin
			    load_n = 0;
			    clk_cnt = 3'd0;
			    end

   1'b1:  if(clk_cnt == 3'd7) begin
                                  state = 1'b0;
	                              load_n  = 0;
	                              clk_cnt = 3'd0;
                                  end
                      else  begin
                                    clk_cnt = clk_cnt + 1;
	                        end
          endcase
  //
  always@(negedge sclk)
    if(!rst_n)
        datain_reg <= 8'b0;
        else 
 	          datain_reg <= {datain_reg[6:0],sdi};	
 //
 always@(posedge clkin)
    if(!rst_n)
        dataout <= 8'b0;
        else if(!load_n)
                dataout <= datain_reg;
 //               	                             
      assign    sclk = (load_n==1'b0) ? 1'b0 : clkin;		
// 
 
endmodule
