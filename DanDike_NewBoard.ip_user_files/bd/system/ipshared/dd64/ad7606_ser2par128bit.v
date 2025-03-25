`timescale 1ns / 10ps

//////////////////////////////////////////////////////////////////////////////////

module ad7606_ser2par128bit

   (
    //FPGA interface	
	input           clkin,           //12.5MHz main clock,generally 10MHz~17MHz
    input           rst_n,
    input           conv_start,       //High state must continued more than 80ns(one 12.5MHz cycle)
    output  reg     rd_en,
    output [127:0]  dout,            //channel 1 to channel 8 from MSB to LSB

	//ADDC interfaces
    output yad_rst,
    output reg yad_cvn,
    output reg yad_cs,
    output yad_ck,
    input yad_sa,
    input yad_sb,
    input yad_busy
    );
   
  // 

parameter  idle  = 3'd0 ;
parameter  s1    = 3'd1;
parameter  s2    = 3'd2;
parameter  s3    = 3'd3;
parameter  s4    = 3'd4;

reg    [3:0]  state;
reg    [5:0]  count1;
reg           count_end;            
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
         state <= s1; yad_cvn <= 1; rd_en<=0;end
                                    
s1 :  //40ns of CONVST X high to BUSY high 
        if (yad_busy) state <= s2;

s2 :    if(!yad_busy) begin 
         state <= s3; yad_cvn <= 0; end

s3 : begin state <= s4; yad_cs <= 0;    end    

s4 :     if(count_end)  begin state <= idle; yad_cs <= 1;rd_en <=1; end

endcase

assign yad_ck = yad_cs?1'b1:clkin;

always@(negedge yad_ck or negedge rst_n)
if(!rst_n) begin
      count1      <= 6'd0;
      count_end   <= 1'b0; 
      end
else if(count1==6'd63)begin
                     count1 <= 6'd0;
                     count_end <= 1'b1; end
                else begin
                     count1 <= count1 + 1;
                     count_end <= 1'b0;  end	 
	 
//------------------------------------------------------------------------------	 
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
