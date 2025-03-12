`timescale 1ns / 1ps
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // 1. 每次启动前，start先发0复位上次的状态，再发1启动本次配置。
    // 2. 每启动一次配置，wr595和rd165同时完成一次更新，读和写在一个clkin时钟周期内同时完成，已保证同步。
    // 3. 往TPIC6B595写出的数据放在din中，从din[31]位向左串行移出。
    // 4. 从74HC165读回来数据放在dout中，最后一位放在dout[0]位。
    // 5. dout和din的数据位数相同，可选8、16、24、32位，由byte_num决定。
    // 6. 输入主时钟为25MHz。
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    module onoff_io(
           //FPGA interface
           input clkin,           //25MHz  main clock
           input rst_n,           // low actively
           input start,          //start one write&read or reset last status
           input [2:0] byte_num,      //1,2,3,4 refer to 8,16,24,32 of the io bits respectively
           input      [31:0] din,    //datas writted to TPIC6B595
           output reg [31:0] dout,  //datas read from 74HC165   
           output reg      done,    //feedback to CPU 
           //board interface
           output reg cs,                    
           output sclk,         
           output sdo,
           input  sdi            
         );     

        reg         rd_over ;
        reg         enable ;     //can start one new wr_rd.  

    //--------------------------------------------------------------------------------
	    reg   start_dly;
	    wire  start_posedge;
	    wire  start_negedge;
        
        always@(posedge clkin)
            if(!rst_n)
                    start_dly <= 1'b0;
               else
                    start_dly <= start;
        
        assign start_posedge = ~start_dly && start;
        assign start_negedge = start_dly && ~start; 	
        
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
                           
          over :      state <= idle ;

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

  endmodule