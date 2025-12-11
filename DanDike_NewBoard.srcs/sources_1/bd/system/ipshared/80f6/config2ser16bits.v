`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 1. 每次启动前，start先发0复位上次的状态，再发1启动本次配置。
// 2. ld1595和ld595只能分别配置，每次配置一类器件；也就是说，每次配置只能一个发1。
// 3. 两类器件的被发送数据共用数据寄存器din0 ~ din3；数据都从最高位排列.
// 4. 以din0为例，配置1595时，din0 = {ub，ua}，而配置595时，din0 = {ub, 8'b0, ua, 8'b0}，
//    因为两类器件的配置数据ua和ub长度分别是16bit和8bit。
// 5. 因为start信号有效时，立即会去锁定配置数据din0 ~ din3以及lad1595和ld595的值，所以CPU发送数据的顺序是
//      din0、din1、din2、din3和{start, ld1595, ld595}。
// 6. 输入主时钟为25MHz。
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
module config2ser16bits(
    //FPGA interface
    input clkin,           //25MHz  main clock
    input rst_n,
    input start,          //start one congfiguration
    input ld1595_en,
    input ld595_en,
    input [31:0] din0,    //ub + ua  
    input [31:0] din1,    //ux + uc  
    input [31:0] din2,    //ib + ia  
    input [31:0] din3,    //ix + ic  
    output reg config_done, //feedback to CPU 
    //ADDA board interface
    output reg load,          //LD_W            
    output sclk,          //CLK_W 
    output sdo            //DATA_W
    );
    
    // [UA, UB, UC, UX, IA, IB, IC, IX, 0, 0, 0, 0, 0, SCLK, LD1595, LD595]
     wire  [15:0]   d_seri16bit;
     reg   [15:0]   data_ua, data_ub, data_uc, data_ux;    
     reg   [15:0]   data_ia, data_ib, data_ic, data_ix;  
     reg            sclk16, ld1595, ld595;
     reg            ld1595_en_reg, ld595_en_reg;
     //
    wire  [3:0] count_len;
    reg   sent_over ;
    reg   enable ;     //can start one new configuration  

    //------------------------------------------------------------------
	reg   [1:0] start_dly;
	wire  start_posedge;
	wire  start_negedge;
    
    always@(posedge clkin)
        if(!rst_n)
                start_dly <= 2'b00;
           else
                start_dly <= {start_dly[0],start};;
   
    assign start_posedge = ~start_dly[1] && start_dly[0];
    assign start_negedge = start_dly[1] && ~start_dly[0]; 	


	/* Set by CPU(start),reset after one configuration(sent_over)           */
     always@(negedge clkin)
        if(!rst_n || sent_over)
		             enable  <= 1'b0;
			else  if(start_posedge)
                     enable  <= 1'b1;			
            
   /* Set after one configuration(sent_over),reset by CPU(start==1'b0)      */
       always@(negedge clkin)
        if(!rst_n || start_negedge)
		       config_done  <= 1'b0;
			else  if(sent_over)
                     config_done  <= 1'b1;	 
                  
    //  configuration data length
     assign count_len = (ld1595_en_reg && ~ld595_en_reg) ? 4'd15 :
                        (~ld1595_en_reg && ld595_en_reg) ? 4'd7 : 4'd0;
    //
     assign d_seri16bit = {data_ua[15], data_ub[15], data_uc[15], data_ux[15],                       
                           data_ia[15], data_ib[15], data_ic[15], data_ix[15],
                           sclk16, ld1595, ld595,1'b1, 1'b0, 1'b1, 1'b0, 1'b1 };     
             
   /*==========================================================================*/
         reg   [6:0]    state     ;
         reg   [15:0]   datain_reg;  
         reg   [3:0]    clk_cnt   ;
         reg   [4:0]    spi_cnt   ;
         parameter  idle      = 7'b0000001;
         parameter  rd_dframe = 7'b0000010;
         parameter  send_spi  = 7'b0000100;
         parameter  rd_word   = 7'b0001000;
         parameter  latch_before = 7'b0010000;        
         parameter  latch     = 7'b0100000;
         parameter  rst_sent_over = 7'b1000000;
         
    always@(negedge clkin)
        if(!rst_n) begin
        	load <= 1;
        	state <= idle;
        	datain_reg <= 16'd0;
        	clk_cnt <= 4'd0;
        	spi_cnt <= 5'd0;
        	sent_over <= 1'b0 ;
        	//shift
            sclk16 <= 1'b0;
            ld1595 <= 1'b1;
            ld595  <= 1'b0;   
            //--------------------
             data_ua <= 16'd0;    
             data_ub <= 16'd0;
             data_uc <= 16'd0;   
             data_ux <= 16'd0;              
             data_ia <= 16'd0;   
             data_ib <= 16'd0;                
             data_ic <= 16'd0;    
             data_ix <= 16'd0; 
             ld1595_en_reg <= 1'b0;
             ld595_en_reg  <= 1'b0;       	                    	
        	end
        else case(state)
    	idle  :   if(enable) begin
    			    state <= rd_dframe;
    			    clk_cnt <= 4'd0;
    			    spi_cnt <= 5'b0;
    			    load <= 1;
    			    //load useful datas
     	            sclk16 <= 1'b0;
                    ld1595 <= 1'b1;
                    ld595  <= 1'b0;                            
                    data_ua <= din0[15:0 ];    
                    data_ub <= din0[31:16];
                    data_uc <= din1[15:0 ];    
                    data_ux <= din1[31:16];                
                    data_ia <= din2[15:0 ];    
                    data_ib <= din2[31:16];                
                    data_ic <= din3[15:0 ];    
                    data_ix <= din3[31:16];
                    ld1595_en_reg <= ld1595_en;
                    ld595_en_reg  <= ld595_en;       			    
    			end
    		else      begin
    		        state   <= idle;
    			    load    <= 1;
    			    clk_cnt <= 4'd0;
    			    spi_cnt <= 5'b0;
    			    sent_over <= 1'b0;
    			    sclk16 <= 1'b0;
                    ld1595 <= 1'b1;
                    ld595  <= 1'b0;        
    			    end
  
      rd_dframe:  begin
                    load <= 0;
     			    datain_reg <= d_seri16bit;	                   
                    state   <= send_spi;
                   end
                   
      send_spi:  if(clk_cnt == 4'd15)       // One 16bit data of spi-word had been sent
                       begin
                         state <= rd_word;
    	                 load  <= 1;
    	                 clk_cnt <= 4'd0;    	                 
    	               end     
                 else  begin      
                         clk_cnt = clk_cnt + 1;
                         datain_reg = {datain_reg[14:0],1'b0};                      	                               
    	               end
 
      rd_word:   if(spi_cnt == (count_len + 1)) // count_len words have finished 
                    begin 
                        state <= latch_before;
                        load  <= 1;    
                        sclk16 <= 1'b1;
                        ld1595 <= (ld1595_en_reg && ~ld595_en_reg) ? 1'b0 : 1'b1;
                        ld595  <= (~ld1595_en_reg && ld595_en_reg) ? 1'b1 : 1'b0;                                                               
                      end              
                else  
                     begin     //shift datas to reload one spi-word              
                      if(sclk16 == 1'b1) begin  //genarate one SCLK posedge for sampling and shifting bits 
                                 state   <= rd_dframe ;
   			                     data_ua <= {data_ua[14:0], 1'b0} ;	
    			                 data_ub <= {data_ub[14:0], 1'b0} ;
    			                 data_uc <= {data_uc[14:0], 1'b0} ;	
    			                 data_ux <= {data_ux[14:0], 1'b0} ;			    
    			                 data_ia <= {data_ia[14:0], 1'b0} ;	
    			                 data_ib <= {data_ib[14:0], 1'b0} ;			    
    			                 data_ic <= {data_ic[14:0], 1'b0} ;	
    			                 data_ix <= {data_ix[14:0], 1'b0} ; 
    			                 sclk16  <= 1'b0;	                
    	                       end              
                         else  begin                                        
                                  sclk16 <= 1'b1;  
                                  state  <= rd_dframe;
    		  	                 spi_cnt <= spi_cnt +1;	                                  
                                end      
                        end   
                        
   latch_before:    begin
                       load <= 0;
                       datain_reg <= d_seri16bit;                       
                       state   <= latch;
                      end    
                                         
        latch :     if(clk_cnt == 4'd15)       // latch the last word
                                begin
                                  state <= rst_sent_over;
                                  load  <= 1;
                                  clk_cnt <= 4'd0;
                                  sent_over <= 1;
                                end     
                          else  begin      
                                  clk_cnt <= clk_cnt + 1;
                                  datain_reg <= {datain_reg[14:0],1'b0};                                                         
                                end
                                
   rst_sent_over:      begin
                          state  <= idle;            
                          sent_over <= 0;    
                       end  								
								
								
        default :      begin
                          	load <= 1;
                           state <= idle;
                           clk_cnt <= 4'd0;
                           spi_cnt <= 5'd0;
                           sent_over <= 1'b0 ;                  
                          end                     
                       
              endcase
              
          assign    sclk = load ? 1'b0 : clkin;		
          assign    sdo = datain_reg[15];       

endmodule  //config2ser16bits
