`timescale 1ns / 1ps
     ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 1. 输入主时钟clkin为25MHz。
    // 2. 每一次读写操作，wr595和rd165同时完成一次更新，读和写在一个clkin时钟周期内同时完成，已保证同步。
    // 3. 往TPIC6B595写出的数据要在读写操作之前放在din中；从din[31]位向左串行移出。
    // 4. 从74HC165读回来数据放在dout中，最后一位放在dout[0]位。
    // 5. dout和din的数据位数相同，可选8、16、24、32位，由byte_num决定。	
	  // 6. 开关量的读写操作设置了4种模式，由rw_modes决定，具体含义如下：
	  //    rw_modes=2'b00, 没有读写。禁止开关量的读写操作。
  	//    rw_modes=2'b01, 一次读写。由start的上升沿启动；收到done有效信号就可以读dout的值。
  	//    rw_modes=2'b10, 随时读写。修改din的值会立即发出，可以随时读取dout的值；不输出done信号。
  	//    rw_modes=2'b11, 带时戳读。修改din的值会立即发出，dout的值变化时锁定dout和时戳，同时发出done信号。	
  	// 7. 带时戳读，本质上还是随时读写，只是在dout发生变化的那个done上升沿锁定一个时间点而已。
    // 8. 一次完整的读写所需的时间与byte_num的值相关，最大为272个clkin时钟周期（约为10.88us）。	
    // 9. 一次读写模式下，dout的数据稳定期在done上升沿到来以后，持续时间与byte_num的值相关，
    //    大约1.6us~5.44us；要立即读。	      
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    module onoff_io(
           //FPGA interface
           input clkin,           //25MHz  main clock
           input rst_n,           // low actively
           input start,           //start one write&read or reset last status
           input [2:0] byte_num,  //1,2,3,4 refer to 8,16,24,32 of the io bits respectively
		   input [1:0] rw_modes,  //0,1,2,3 refer to the mode is disenable, once, at once, or timed respectively. 
           input     [31:0] din,  //datas writted to TPIC6B595
           output reg [31:0] dout,//datas read from 74HC165   
           output reg      done,  //feedback to CPU 
		   output    latch_time, 		   
           //board interface
           output cs,                    
           output sclk,         
           output sdo,
           input  sdi            
         );   

	     wire  [31:0] dout_raw;
	     wire         done_raw;	 				 
     //---------------------------------------------------------------------------------------------------
	   reg   [1:0]  start_dly;
       wire         start_cpu;
       
       always@(posedge clkin)
           if(!rst_n)
               start_dly <= 2'b00;
              else if(rw_modes==2'b00)
                   start_dly <= 2'b00; 
                    else            
                   start_dly <= {start_dly[0],start};
      
       assign start_cpu = ~start_dly[1] && start_dly[0];
	 //---------------------------------------------------------------------------------------------------    
             reg   [2:0 ]   onoff_state;
			 reg          start_posedge;
			 reg          start_negedge;
			 
    always @(posedge clkin) begin
        if (~rst_n) begin
                  onoff_state   <= 3'd0;
                  start_posedge <= 1'b0;	
                  start_negedge <= 1'b0;                			  
                  end
          else begin
     		case(onoff_state)
             3'd0 :  if(start_cpu) begin 
                      onoff_state   <= 3'd1;
                      start_posedge <= 1'b0;	
                      start_negedge <= 1'b1;
                      end
             3'd1 :  begin 
                      onoff_state   <= 3'd2;
                      start_posedge <= 1'b1;	
                      start_negedge <= 1'b0;	                                        				  
                      end
             3'd2 :  begin 
                      start_posedge <= 1'b0;	
                      start_negedge <= 1'b0;
					  if(done_raw) 
                       onoff_state   <= 3'd3;
					 end
             3'd3 :  begin					  
					  if ( rw_modes[1]==1'b1) //continuous writing&reading
                          onoff_state   <= 3'd4;					       
					  else                  //only a writing&reading
                          onoff_state   <= 3'd7;					  				  
                      end	
            // continuous writing&reading			
	        // delay 3 clkin cycles    
             3'd4 :   onoff_state   <= 3'd5;	               
             3'd5 :   onoff_state   <= 3'd6;	             
             3'd6 : begin 
                      onoff_state   <= 3'd1;
                      start_posedge <= 1'b0;	
                      start_negedge <= 1'b1;                        
                      end			 
            // only a writing&reading			
             3'd7 :   begin 
                      onoff_state   <= 3'd0;
                      start_posedge <= 1'b0;	
                      start_negedge <= 1'b0;                   
                      end			 
			default:  begin			 
                      onoff_state   <= 3'd0;
                      start_posedge <= 1'b0;	
                      start_negedge <= 1'b0;                    
                      end	
			endcase //onoff_state
		end
	end	
	 //---------------------------------------------------------------------------------------------------
	   reg   [1:0]  done_dly;
       wire         done_posedge;
       reg          done_posedge_dly;    
       always@(posedge clkin)
           if(!rst_n)
               done_dly <= 2'b00;
             else            
               done_dly <= {done_dly[0],done_raw};     
       assign done_posedge = ~done_dly[1] && done_dly[0];
     //delay a clkin cycle       
       always@(posedge clkin)       
          done_posedge_dly <= done_posedge;
     // 
	 // reg   [31:0]   dout_reg1; 
	    reg   [31:0]   dout_reg2; 	   

	  always @(posedge clkin) begin
        if (~rst_n) begin
                  dout      <= 32'd0;
			      dout_reg2 <= 32'd0; end
		  else if (done_posedge) begin
		          dout      <= dout_raw;
			      dout_reg2 <= dout;  end	 
		 end 	           
	     assign latch_time = ((dout != dout_reg2) && done_posedge_dly); 	  
	 //    
	   always@( * )
	     begin
	        case(rw_modes)
	          2'b00 :  done <= 1'b0;
	          2'b01 :  done <= done_posedge_dly;
	          2'b10 :  done <= 1'b0;
	          2'b11 :  done <= (done_raw && latch_time); 
	        endcase
	     end 	 
     //---------------------------------------------------------------------------------------------------
        once_rw    once_inst(
           //FPGA interface
           .clkin        (clkin        ),  // 25MHz  main clock
           .rst_n        (rst_n        ),  // low actively
           .start_posedge(start_posedge),  // start one write&read or reset last status
		   .start_negedge(start_negedge),
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
         always@(negedge clkin)
            if(!rst_n || rd_over)
	    	             enable  <= 1'b0;
	    		else  if(start_posedge)
                         enable  <= 1'b1;			    
                    
         /* Set after one reading(rd_over),reset by CPU(start==1'b0) */
         always@(negedge clkin)
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
                                     
        recv165 :    if(databits_cnt ==  byte_num * 8)       // clock numbers=9?
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
