`timescale 1ns / 1ps
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 1. 输入主时钟clkin为25MHz。
    // 2. 每一次读写操作，wr595和rd165同时完成一次更新，读和写在一个clkin时钟周期内同时完成，已保证同步。
    // 3. 往TPIC6B595写出的数据要在读写操作之前放在din中；从din[31]位向左串行移出。
    // 4. 从74HC165读回来数据放在dout中，最后一位放在dout[0]位。
    // 5. dout和din的数据位数相同，可选8、16、24、32位，由byte_num决定。	
	// 6. start信号为高电平期间，硬件周期性读写，读写周期为100us。新读出的数据和上一个数据不同时，生成
	//    latch_time（打时戳用）和done（中断用）信号。
	// 7. start信号为低电平时，关闭该功能。
    // 8. 一次完整的读写所需的时间与byte_num的值相关，最大为272个clkin时钟周期（约为10.88us）。	
    // 9. ------------------	
    // 10. 2025年6月13日修改:把连续读写改为周期读写，读写周期为100us。
    //     采样时刻到done的上升沿的时延为byte_num x 320 + 30ns，所以，开关量8位的时延350ns，32位1310ns。
	//
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    module onoff_io(
           //FPGA interface
           input clkin,           //25MHz  main clock
           input rst_n,           // low actively
           input start,           //start one write&read or reset last status
           input [2:0] byte_num,  //1,2,3,4 refer to 8,16,24,32 of the io bits respectively
                                  //
           input     [31:0] din,  //datas writted to TPIC6B595
           output reg [31:0] dout, //datas read from 74HC165   
           output          done,   //feedback to CPU 
		   output reg latch_time, 		   
           //board interface
           output cs,                    
           output sclk,         
           output sdo,
           input  sdi            
         );   

    //------------生成100us的周期信号gap100us----------------
        reg           gap100us;
        reg [11:0]    gap_counter;
		
       always@(posedge clkin)
           if(!rst_n) begin
               gap100us <= 1'b0;
			   gap_counter <= 12'd0;
			  end 
              else if(start)		
                  begin
				   if(gap_counter == 12'd2499) begin //delay 2500x0.04us=100us
                        gap100us <= 1'b1;
			            gap_counter <= 12'd0;
			          end  
                      else begin
					    gap100us <= 1'b0;
                        gap_counter <= gap_counter + 1;
					  end	
                  end
                  else				  
                    begin
                      gap100us <= 1'b0;
                      gap_counter <= 12'd0;
                    end 
     //-------------------------------------------------------------------------- 
		  reg   gap100us_dly;	
        always@(posedge clkin)  //delay a clkin cycle
		        gap100us_dly <= gap100us;   

	 //--------------------------------------------------------------------------			
	 //  生成latch_time和done信号 
	      wire         done_raw;
	      wire [31:0]  dout_raw;	
	      reg          done_dly;
          wire         done_posedge;
          reg          done_posedge_dly;
  
        always@(posedge clkin)  
           begin            
             done_dly <= done_raw;  
             done_posedge_dly <= done_posedge;
           end 		   
        assign done_posedge = ~done_dly && done_raw;  
     // 
	    reg   [31:0]   dout_reg; 	   
	    always @(posedge clkin) 
         if(!rst_n) begin
			      dout <= 32'd0; 	 
	              dout_reg <= 32'd0;
	            end        
            else if (done_posedge) begin
			      dout <= dout_raw; 	 
	              dout_reg <= dout;
	            end  
	  //                      
        always@(posedge clkin) 
            if(!rst_n)
	              latch_time <= 1'b0;
	    	  else  if(done_posedge_dly)		
	              latch_time <= (dout != dout_reg); 	
                else
	              latch_time <= 1'b0;		     
 	 //
         assign  done = latch_time ;	
		 
     //---------------------------------------------------------------------------------------------------
        once_rw    once_inst(
           //FPGA interface
           .clkin        (clkin        ),  // 25MHz  main clock
           .rst_n        (rst_n        ),  // low actively
           .start_posedge(gap100us     ),  // start one write&read or reset last status
		   .start_negedge(gap100us_dly ),
           .byte_num     (byte_num     ),  // 1,2,3,4 refer to 8,16,24,32 of the io bits respectively
           .din          (din          ),  // datas writted to TPIC6B595
           .dout         (dout_raw     ),  // datas read from 74HC165   
           .done         (done_raw     ),  // feedback to CPU 
           //board interface
           .cs        (cs),                    
           .sclk      (sclk),         
           .sdo       (sdo),
           .sdi       (sdi)            
         );     
  
   endmodule // onoff_io
   
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 1. 每次启动前，start先发0复位上次的done信号，再发1启动本次操作。操作完成后，自动发出操作结束信号done。
    //    CPU在收到done信号后，才能对start信号清0，否则，done信号无法被清0。
    // 2. 每启动一次操作，wr595和rd165同时完成一次更新，读和写在一个clkin时钟周期内同时完成，已保证同步。
    // 3. 往TPIC6B595写出的数据放在din中，从din[31]位向左串行移出。
    // 4. 从74HC165读回来数据放在dout中，最后一位放在dout[0]位。
    // 5. dout和din的数据位数相同，可选8、16、24、32位，由byte_num决定。
    // 6. 输入主时钟为25MHz。
    // 7. 一次完整的操作(包括读出数据的时间)，大约需要（268+2）个clkin时钟周期。
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    module once_rw(
           //FPGA interface        
           input clkin,            //25MHz  main clock
           input rst_n,            // low actively
           input start_posedge,    //start one write&read or reset last status
		   input start_negedge,    //reset the done signal
           input [2:0] byte_num,   //1,2,3,4 refer to 8,16,24,32 of the io bits respectively
           input      [31:0]  din, //datas writted to TPIC6B595
           output reg [31:0] dout, //datas read from 74HC165   
           output reg        done, //feedback to CPU 
           //board interface
           output reg cs,                    
           output     sclk,         
           output     sdo,
           input      sdi            
         );     

        reg         rd_over;
        reg         enable ;     //can start one new wr_rd.  

	    /* Set by CPU(start),reset after one reading(rd_over)          */
         always@(posedge clkin)
            if(!rst_n || rd_over)
	    	             enable  <= 1'b0;
	    		else  if(start_posedge)
                         enable  <= 1'b1;			    
                    
         /* Set after one reading(rd_over),reset by CPU(start==1'b0) */
         always@(posedge clkin)
          if(!rst_n || start_negedge)
	                         done  <= 1'b0;
	   	        else  if(rd_over)
                             done  <= 1'b1;		   
	     
       /*============================================================================*/          
             reg            sclk_en     ;
             reg   [5:0]    databits_cnt;      
             reg   [4:0]    state     ;
             reg   [31:0]   datain_reg;  
             reg   [1:0]    clk_div;
             wire           clk6m;
             
             parameter  idle      = 5'b00001;
             parameter  tran595   = 5'b00010;
             parameter  latch     = 5'b00100;
             parameter  recv165   = 5'b01000;
             parameter  over      = 5'b10000;    
             
         always@(posedge clkin)
             if(!rst_n) clk_div <= 2'b00;  
                else  clk_div <= clk_div + 1;  
                
         assign    clk6m = clk_div[1];        
         assign    sclk = sclk_en ? clk6m : 1'b0 ;        
         assign    sdo = datain_reg[31];        
                 
        always@(negedge clk6m or negedge rst_n)
            if(!rst_n) begin
                cs           <=  0;
                sclk_en      <=  0;
                state        <=  idle;
                databits_cnt <=  6'd0;
                rd_over      <=  1'b0 ;
                datain_reg   <= 32'd0;                       
                end
            else case(state)
            idle  :   if(enable) begin
                        state        <= tran595;
                        databits_cnt <= 6'd0; 
                        sclk_en      <= 1'b1;
                        cs           <= 1'b0;
                        rd_over      <= 1'b0;    
                        datain_reg   <= din ;                                            
                         end
                else   begin
                        cs           <=  0;
                        sclk_en      <=  0;
                        state        <=  idle;
                        databits_cnt <=  6'd0;
                        rd_over      <=  1'b0 ;      
                        end
      
         tran595 :    if(databits_cnt == byte_num * 8-1)       // 
                           begin
                             state <= latch;
                             cs    <= 1;                             
                             databits_cnt <= 6'd0;  
                             sclk_en   <= 0;                       
                           end     
                     else  begin      
                             databits_cnt <= databits_cnt + 1; 
                             datain_reg   <= {datain_reg[30:0],1'b0};                                                                                      
                           end
     
          latch :     begin 
                            state <= recv165 ;
                            cs    <= 1'b0;   
                            sclk_en  <= 1'b1;                                                               
                          end   
                                     
        recv165 :    if(databits_cnt ==  byte_num * 8-1)       // clock numbers=8
                           begin
                             state   <= over;
                             rd_over <= 1'b1;
                             sclk_en <= 1'b0;                        
                           end     
                     else  begin      
                             databits_cnt <= databits_cnt + 1;                                                    
                           end
                           
          over :      begin
                          state   <= idle ;
                          rd_over <= 1'b0 ;
                        end 
         default :    begin
                          cs           <=  0;
                          sclk_en      <=  0;
                          state        <=  idle;
                          databits_cnt <=  6'd0;
                          rd_over      <=  1'b0 ;      
                          end  
                           
                  endcase
//--------------------------------------------------------------------------------//     
     //read datas from HC165                       
       always@(posedge clk6m or negedge rst_n)
           if(!rst_n) begin
                       dout   <= 32'd0;                   
                       end
             else if(state[3]==1'b1)   //recv165    
                       dout <= {dout[30:0],sdi};                                                         

  endmodule // once_rw
