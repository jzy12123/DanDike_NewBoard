//-----------------------------------------------------------------------------//
//
//BRAM的读地址，只有复位时清零，然后就没有明确地址
//如果一批读数据不是1024个采样点，中间停下来，下次读回接着前面的地址
//
//----------------------------------------------------------------------------//
module dds_dac(         
         // FPGA interface
		 input          clkin    ,//100MHz
         input          rst_n    ,
         input          enable   , //The DDS_DAC works when enable high,stops when enable low
         input [15:0]   freq_div , //
		 
 		 output wire [127:0] stream_data_dds,  //
		 output reg          enable_dac,       // 
		
         //BRAM interface
         input  [31:0]     ram_rd_data,
         output            ram_clk,
         output            ram_en,
         output [31:0]     ram_addr,
         output [3:0]      ram_we,
         output [31:0]     ram_wr_data,
         output            ram_rst
       );
    //
    wire [31:0] data0, data1, data2, data3;
    reg [15:0]  counter;
    reg [1:0]   flow_cnt;
    reg [2:0]   dly_cnt;    
    reg         clk_analog;     //analog signal freqence of dds  
    reg         start_rd;
	 
     // generating clk_analog signal      
     always@(posedge clkin)
        if(!rst_n) begin
           counter    <= 16'd0;
           clk_analog <= 1'b0;
           end
           else if (enable) begin
                   if(counter >= (freq_div-1)) begin
                         counter    <= 16'd0;
                         clk_analog <= 1'b1;
                         end
                         else begin
                              counter    <= counter + 1;
                              clk_analog <= 1'b0;
                              end
					     end		  
                   else begin
                        counter    <= 16'd0;
                        clk_analog <= 1'b0;
                        end
		   
     //generating start_rd and enable_dac signal
     always@(posedge clkin)
        if(!rst_n) begin
            flow_cnt  <= 2'd0;
            start_rd  <= 1'b0;
            enable_dac <= 1'b0; 
            dly_cnt   <= 3'd0; 
            end
         else begin
              case(flow_cnt)
               2'd0 :   if(clk_analog) begin
                           flow_cnt  <= 2'd1;
                           start_rd  <= 1'b1;
                           enable_dac <= 1'b0;
                           dly_cnt   <= 3'd0;
                         end
                2'd1 :     begin
                            flow_cnt  <= 2'd2;
                            start_rd  <= 1'b0;
                            enable_dac <= 1'b0;
                            dly_cnt   <= 3'd0;
                            end
                 2'd2 : if (dly_cnt == 3'd4) begin
                            flow_cnt  <= 2'd3;
                            start_rd  <= 1'b0;
                            enable_dac <= 1'b1;
                            dly_cnt   <= 3'd0;
                            end
                            else
                               dly_cnt <= dly_cnt + 1'b1;  
                 2'd3 : if (dly_cnt == 3'd4) begin
                            flow_cnt  <= 2'd0;
                            start_rd  <= 1'b0;
                            enable_dac <= 1'b0;
                            dly_cnt   <= 3'd0;
                            end
                            else
                               dly_cnt <= dly_cnt + 1'b1;       
                 default :  begin                                          
                             flow_cnt  <= 2'd0;
                             start_rd  <= 1'b0;
                             enable_dac <= 1'b0; 
                             dly_cnt   <= 3'd0;                                      
                             end
                   endcase
               end

	 //	
     assign  stream_data_dds = {data3,data2,data1,data0};	 
     //       
     assign  ram_clk = clkin;
     assign  ram_we  = 4'd0;
     assign  ram_wr_data = 32'd0;
     assign  ram_rst = ~rst_n;  

          //
     bram_rd u_bram_rd(
              // FPGA interface 
              .clk(clkin),      //100MHz
              .rst_n(rst_n),
              .start_rd(start_rd),
			  
              //BRAM interface
              .ram_rd_data(ram_rd_data),
              .ram_en(ram_en),
              .ram_addr(ram_addr),
			  
              //DAC interface
              .data0(data0),
              .data1(data1),
              .data2(data2),
              .data3(data3)
            );
     //
endmodule 
//
module bram_rd(
         input clk,      //100MHz
         input rst_n,
         input start_rd,
         //BRAM interface
         input  [31:0] ram_rd_data,
         output reg ram_en,
         output reg [31:0] ram_addr,
         //DAC interface
         output reg [31:0] data0,
         output reg [31:0] data1,
         output reg [31:0] data2,
         output reg [31:0] data3
       );
//
reg [2:0]  flow_cnt;
//       
//根据读开始信号,从BRAM中读出数据。需要5个clock周期
//
always @(negedge clk) 
    if(!rst_n) begin
        flow_cnt <= 3'd0;
        ram_en <= 1'b0;
        ram_addr <= 32'd0;
        data0 <= 32'd0;      
        data1 <= 32'd0;      
        data2 <= 32'd0;      
        data3 <= 32'd0;       
        end
    else begin
     case(flow_cnt)
           3'd0 : begin                     //浪费一个clock
                if(start_rd) begin
                    flow_cnt <= 3'd1;
                    ram_en <= 1'b1;
                          end
                       end
           3'd1 : begin                     //生成第二个BRAM字符地址                                                               
                   flow_cnt <= 3'd2;
                   ram_addr <= ram_addr + 32'd4;   //地址累加4
                   data0 <= ram_rd_data;
                  end
           3'd2 : begin                     //生成第三个BRAM字符地址                                                               
                   flow_cnt <= 3'd3;
                   ram_addr <= ram_addr + 32'd4;   //地址累加4
                  data1 <= ram_rd_data;
                  end
           3'd3 : begin                    //生成第四个BRAM字符地址                                                               
                   flow_cnt <= 3'd4;
                   ram_addr <= ram_addr + 32'd4; //地址累加4
                   data2 <= ram_rd_data;
                  end
           3'd4 : begin                    //
                   flow_cnt <= 3'd5;
                   ram_addr <= ram_addr + 32'd4; //地址累加4
                  data3 <= ram_rd_data;
                   ram_en <= 1'b0;                  
                   end			   
           3'd5 : begin                    //
                   flow_cnt <= 3'd0;
                   end
          default : begin
                   flow_cnt <= 3'd0;
                   ram_addr <= ram_addr;        //地址不变
                    ram_en <= 1'b0;
                           end
          endcase    
       end
      //   
endmodule      