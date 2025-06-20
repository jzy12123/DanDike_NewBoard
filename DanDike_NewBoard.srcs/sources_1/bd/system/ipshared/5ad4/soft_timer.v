`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                      //
//1.本模块的输入时钟来自恒温晶振，10MHz；复位信号rstn借用系统的，所以需要异步复位。                     //
//                                                                                                      //
//2.亚秒的计数脉冲周期为0.1us；亚秒计数器计时长度为10M个周期，需要24bits。                              //
//                                                                                                      //
//3.CPU可以读到的时钟数据源：GPS、RTC、BM和软时钟。CPU可以写入时间信息的地方：RTC和软时钟。该工程中，   //
//  软时钟是应用核心，RTC用来备份时钟数据；因此，软时钟每次修改后，要立即更新RTC。                      //
//                                                                                                      //
//4.亚秒数据的清零信号来源于三个PPS信号：GPS、BM和硬件信号pps_in。PPS信号不修改秒及其以上的时间数据。   //
//  秒计数器的计数脉冲来源于亚秒计数器，所以，秒计数值可能会有±1秒的误差。要保证时钟的绝对精准，可以快速//
//  响应PPS中断信号，通过CPU比对软计数器在PPS脉冲附近与外部接收数据是否一致，来确认是否存在1秒误差。    //
//                                                                                                      //
//5.三个PPS信号共用一个使能信号pps_clr_en；亚秒数据同步/清零功能由CPU的软件来控制。                     //
//                                                                                                      //
//6.CPU对timer和dater预置数的使能脉冲wr_time和wr_date要及时复位，否则，B码无法自动同步最新数据。        //
//                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////
	module soft_timer #
	(  
	    parameter COUNTER_WIDTH = 24,            // 亚秒计数器位宽     // 4
		parameter MAX_COUNT_VALUE = 10_000_000 , // 对应于10MHz时钟    // 10
        parameter MAX_10MS = 99_999,             // BM编解码计数器位宽 // 99
        parameter MAX_8MS  = 79_999,             // BM编解码计数器位宽 // 79
        parameter MAX_5MS  = 49_999,             // BM编解码计数器位宽 // 49
        parameter MAX_2MS  = 19_999,             //	BM编解码计数器位宽 // 19	
        parameter MAX_6MS  = 59_999,             // BM编解码计数器位宽 // 59
        parameter MAX_3MS  = 29_999	             // BM编解码计数器位宽 // 29			
    )			
	(
       // system clock and reset signal   
    	input  wire   clkin,          // 10MHz
		input  wire   rst_n,          // 低电平有效，异步复位
		
	   // FPGA 外部端口	   
		input  wire   pps_gps   ,     // 来自GPS的秒同步信号，脉宽300ms
		input  wire   pps_in    ,     // 辅助对时，只要求秒同步
		input  wire   irig_b_in ,     // B码输入信号。用于设备同步，要求显示对时过程及对时成功反馈
		output wire   irig_b_out,     // B码输出，一直发   
		output wire   pps_50    ,     // pps脉冲输出，50%占空比，用于检测内部时钟稳定性
				
	   // FPGA内部端口
		input  wire  [7:0]  pre_year   ,  // 预置日期值,BCD码
        input  wire  [4:0]  pre_month  ,  // BCD码 
        input  wire  [5:0]  pre_day    ,  // BCD码 
        input  wire  [6:0]  pre_week   ,  // 独热码
        input  wire  [9:0]  pre_yearday,  // BCD码 
 		input  wire  [5:0]  pre_hour   ,  // 预置时间值，BCD码
        input  wire  [6:0]  pre_minute ,  // BCD码 
        input  wire  [6:0]  pre_second ,  // BCD码 
		input  wire   wr_date ,           // CPU脉冲，预置与日期相关的数据   
        input  wire   wr_time ,           // CPU脉冲，预置与时间相关的数据             
        input  wire   bm_encode_en,
        input  wire   bm_decode_en,       // 启动B码校时，校时成功后硬件关闭？
        input  wire   pps_clr_en  ,       // 亚秒计数器清零使能信号 
		
	   // 当前BM解码的值
		output  wire  [7:0]  bm_year   ,    // 当前日期值,BCD码
        output  wire  [9:0]  bm_yearday,    // BCD码 
 		output  wire  [5:0]  bm_hour   ,    // 当前时间值，BCD码
        output  wire  [6:0]  bm_minute ,    // BCD码 
        output  wire  [6:0]  bm_second ,    // BCD码
		output  wire  [16:0] bm_daysec ,    // 日秒，二进制 	   
	   
       //当前软时钟的值
		output  wire  [7:0]  curr_year   ,   // 当前日期值,BCD码
        output  wire  [4:0]  curr_month  ,   // BCD码 
        output  wire  [5:0]  curr_day    ,   // BCD码 
        output  wire  [6:0]  curr_week   ,   // 独热码
        output  wire  [9:0]  curr_yearday,   // BCD码 
 		output  wire  [5:0]  curr_hour   ,   // 当前时间值，BCD码
        output  wire  [6:0]  curr_minute ,   // BCD码 
        output  wire  [6:0]  curr_second ,   // BCD码
		output  wire  [16:0] curr_daysec ,   // 日秒，二进制 
        output  wire  [23:0] curr_subsec ,   // 亚秒，二进制	
		
       // CPU中断请求信号
	    output  wire   pps_gps2cpu,       // 3个pps信号，用于软件精确对时
	    output  wire   pps_in2cpu ,
        output  wire   pps_bm2cpu ,
	    output  reg    bm_syn_end ,       // B码解码成功，并完成软时钟的同步	
		output  wire   date_update        // 00:00:00到了，请求CPU更行日期数据  		
	);
	//  
      wire         pps_bm ;
      wire         carry_bit;                // 秒脉冲
	  reg  [1:0]   pps_r ;
	  wire         pps_posedge;
	  
	assign  pps_bm2cpu  = pps_bm  && pps_clr_en;
	assign  pps_in2cpu  = pps_in  && pps_clr_en; 
	assign  pps_gps2cpu = pps_gps && pps_clr_en; 
		  
   // subsecond	   
    always@(posedge clkin or negedge rst_n)
	   if(~rst_n)
	          pps_r <= 2'b0;
        else if(pps_clr_en)
              pps_r <= {pps_r[0],(pps_gps || pps_in || pps_bm)};
        else 
              pps_r <= 2'b0;		
    assign  pps_posedge = ~pps_r[1] && pps_r[0];
    //
    subsecond #   (  
	                .COUNTER_WIDTH(COUNTER_WIDTH),
	            	.MAX_COUNT_VALUE(MAX_COUNT_VALUE)					
                )
     subsec_inst(
        .rstn            (rst_n      ),        //low active
        .clkin           (clkin      ),        //The freqency is 10MHz
		.pps_in          (pps_posedge),        //同步清零，一个clk周期		
        .dout            (curr_subsec),    
        .pps_out         (pps_50     ),
        .carry_bit       (carry_bit  )          
        );   
   
   //-------------------------------- B码同步软时钟,开始 ------------------------------------//
      reg  [2:0] bm_state;
      reg        bm_syn_dataswitch;    // 切换数据
      reg        bm_syn_en;            // 切换使能信号
      reg  [1:0] bm_decode_en_r;
      reg        bm_decode_start;      // decoder模块的使能信号
	  wire       bm_decode_en_posedge;
      
    //延迟一拍，用来启动并捕捉上升沿
    always @(posedge clkin or negedge rst_n) begin
        if (~rst_n) begin
            bm_decode_en_r <= 2'b0;
            end
          else
            bm_decode_en_r <= {bm_decode_en_r[0],bm_decode_en};
        end   
    assign bm_decode_en_posedge = bm_decode_en_r[0] & ~bm_decode_en_r[1];//上升沿       	  
    //
    always @(posedge clkin or negedge rst_n) begin
        if (~rst_n) begin
                  bm_state <= 3'd0;
			      bm_syn_dataswitch <= 1'b0;
                  bm_syn_en  <= 1'b0;	
                  bm_decode_start <= 1'b0;
                  bm_syn_end  <= 1'b0;				  
                  end
          else begin
     		case(bm_state)
             3'd0 : if(bm_decode_en_posedge) begin 
                        bm_state <= 3'd1;			              
                        bm_decode_start <= 1'b1;
                        bm_syn_end  <= 1'b0;
                      end
                    else begin
                        bm_state <= 3'd0;
			            bm_syn_dataswitch <= 1'b0;
                        bm_syn_en  <= 1'b0;                        					
                      end
					  
             3'd1 : if(pps_bm) begin
                        bm_state <= 3'd2;			              
                        bm_decode_start <= 1'b0; 
                      end 	
					  
             3'd2 : begin
                        bm_state <= 3'd3;			              
                        bm_syn_dataswitch <= 1'b1; 
                      end 
					  
             3'd3 : begin
                        bm_state <= 3'd4;			              
                        bm_syn_en <= 1'b1; 
                      end 
					  
             3'd4 : begin
                        bm_state <= 3'd5;			              
                      end 					  
					  
             3'd5 : begin
                        bm_state <= 3'd6;			              
                      end 					  
					  
             3'd6 : begin
                        bm_state <= 3'd0;			              
                        bm_syn_end <= 1'b1; 
	                    bm_syn_en  <= 1'b0; 
                        bm_syn_dataswitch <= 1'b0; 						
                      end 

             3'd7 : begin
                        bm_state <= 3'd0;			              
                      end 
            endcase
         end
      end
    //
       wire   [6:0]  pre_sec_2    ;
       wire   [6:0]  pre_min_2    ;
       wire   [5:0]  pre_hur_2    ;
       wire   [9:0]  pre_yearday_2;
       wire   [7:0]  pre_year_2   ;
	   wire          wr_date_2    ;
	   wire          wr_time_2    ;

       assign  pre_sec_2     = bm_syn_dataswitch ? bm_second  : pre_second ;
       assign  pre_min_2     = bm_syn_dataswitch ? bm_minute  : pre_minute ;	   
       assign  pre_hur_2     = bm_syn_dataswitch ? bm_hour    : pre_hour   ;	
       assign  pre_year_2    = bm_syn_dataswitch ? bm_year    : pre_year   ;	
       assign  pre_yearday_2 = bm_syn_dataswitch ? bm_yearday : pre_yearday;
	   
	   assign  wr_date_2 = bm_syn_en ? 1 : wr_date;
       assign  wr_time_2 = bm_syn_en ? 1 : wr_time;	   
   //-------------------------------- B码同步软时钟,结束 ------------------------------------//
   
   // time_counter
    wire   [19:0]   curr_time;
    assign curr_second = curr_time[6:0]  ;
    assign curr_minute = curr_time[13:7] ; 
    assign curr_hour   = curr_time[19:14];	
	
    time_counter timer_inst (
        .rstn        (rst_n      ),        // low active
        .clkin       (clkin      ),        // The freqency is 10MHz
        .count_en    (carry_bit  ),        // 计数秒脉冲，持续一个clk周期		
		.load_en     (wr_time_2  ),        // 预置数据;须从100MHz时钟域同步到10MHz时钟域		
        .pre_time    ({pre_hur_2, pre_min_2, pre_sec_2}), // 预置值，时、分、秒；20bit BCD码  		
        .current_time( curr_time ),        // 结构同pre_time
		.day_end     (date_update),        // 一天结束时的进位脉冲，持续一个clk周期 
		.day_sec     (curr_daysec)         // 本日内的秒数，二进制编码  
        );  
		
   // data_latch
    wire   [35:0]   curr_date;
    assign curr_yearday = curr_date[9:0]  ;
    assign curr_week    = curr_date[16:10]; 
    assign curr_day     = curr_date[22:17];	    
    assign curr_month   = curr_date[27:23]; 
    assign curr_year    = curr_date[35:28];   
	
   date_latch dater_inst(
		.clkin   (clkin    ),                      // 10MHz,专用精确时钟。
		.rstn    (rst_n    ),                      // 系统复位信号,异步
		.latch_en(wr_date_2),                      // 来自CPU的数据锁存信号，需要提取一个clk周期的上边沿使能信号
		.pre_date({pre_year_2, pre_month, pre_day, pre_week, pre_yearday_2}),   //预置数据
		                                         // 年(8bit)、月(5bit)、日(6bit)、周(7bit)、年日(10bit)
		.curr_date(curr_date)	      
	    );        

	// b_encoder	
    b_encoder #(
        .MAX_10MS(MAX_10MS),
        .MAX_8MS (MAX_8MS ),
        .MAX_5MS (MAX_5MS ),
        .MAX_2MS (MAX_2MS ) 
       )
       encoder_inst(
        .clk       (clkin       ),   //10MHz
        .rst_n     (rst_n       ),	
    	.pps       (carry_bit   ),	
    	.encode_en (bm_encode_en),
        .sec       (curr_second ),                // 2位BCD码
        .min       (curr_minute ),                // 2位BCD码
        .hur       (curr_hour   ),                // 2位BCD码
        .year_day  (curr_yearday),                // 3位BCD码；本年的第几天
        .year      (curr_year   ),                // 2位BCD码
        .day_sec   (curr_daysec ),
    	//	
        .irig_b    (irig_b_out  )
     );
    
	// b_decoder
    b_decoder #(
        .MAX_6MS (MAX_6MS),
        .MAX_3MS (MAX_3MS)
    	)
    	decoder_inst (
        .clk   (clkin            ),      // 10MHz     endmodule
        .rst_n (rst_n            ),      
        .irig_b(irig_b_in        ),      
    	.decode_en(bm_decode_start),      // 启动解码，高电平有效；低电平禁止解码
        .sec      (bm_second     ),      // 2位BCD码
        .min      (bm_minute     ),      // 2位BCD码
        .hur      (bm_hour       ),      // 2位BCD码
        .year_day (bm_yearday    ),      // 3位BCD码；本年的第几天
        .year     (bm_year       ),      // 2位BCD码
        .day_sec  (bm_daysec     ),      // 16为binary码，本日的第几秒
    	.pps      (pps_bm        )       // 比Pr上升沿晚2个clk周期的时间，脉宽100ms 
        );

	endmodule //soft_timer
	
    //-----------------------------------------------------------------------------------------------------------//
	module subsecond #
	(  
	    parameter COUNTER_WIDTH = 24,
		parameter MAX_COUNT_VALUE = 10_000_000  //对应于10MHz时钟
    )			
	(
		input wire  clkin,                      //10MHz,专用精确时钟。
		input wire  rstn,                       //系统复位信号,异步
		input wire  pps_in,                     //上升沿同步信号，已由前级电路调整到一个clkin周期
		output  reg [COUNTER_WIDTH-1:0] dout,
		output  reg pps_out,	                //占空比50%的内部时钟检测信号	
		output  wire   carry_bit                //占空比只有1clock的秒脉冲，用于B码发送的同步信号 
	);	
   //	
   assign carry_bit = (dout == MAX_COUNT_VALUE-1) ? 1'b1 :1'b0;
   
   //
   always @(posedge clkin or negedge rstn)
     begin      
	  if (!rstn) 
            dout <= {COUNTER_WIDTH{1'b0}};
			
         else if (pps_in)
            dout <= {COUNTER_WIDTH{1'b0}};	
			
	         else if (carry_bit)
	              dout <= {COUNTER_WIDTH{1'b0}};
				  
                else 
                  dout   <= dout + 1'b1;	
     end
	 
	 //
   always @(posedge clkin or negedge rstn)
     begin  
	  if (!rstn) 
            pps_out <= 1'b0;
			
         else if (carry_bit)	 
            pps_out <= 1'b1;
			
		   else if (dout == (MAX_COUNT_VALUE >>1)-1)	 
              pps_out <= 1'b0;	
             			
             else
                pps_out <= pps_out;		  
	 end
	endmodule //subsecond


//-------------------------------------------------------------------------------------------------------//	
    module time_counter(
        input  wire        rstn,        // low active
        input  wire        clkin,       // The freqency is 10MHz
        input  wire        count_en,    // 计数秒脉冲，持续一个clk周期		
		input  wire        load_en,     // 预置数据;须从100MHz时钟域同步到10MHz时钟域		
        input  wire [19:0] pre_time,    // 预置值
		                                // 2+4+3+4+3+4bits,对应于hr10+hr0+min10+min0+sec10+sec0   		
        output wire [19:0] current_time,// 结构同pre_time
		output wire        day_end,     // 一天结束时的进位脉冲，持续一个clk周期 
		output wire [16:0] day_sec      // 本日内的秒数，二进制编码  
        );
     //
        wire       sec10_en, min0_en, min10_en, hur0_en;
	    wire [3:0] sec0, min0;
        wire [2:0] sec10, min10;
		reg  [3:0]  hur0 ;
        reg  [1:0]  hur10;
        reg  [1:0]  load_r;
        wire        load_posedge;
		
	    wire [4:0] pre_hur, curr_hur; 
	    reg  [4:0] curr_hur_tmp;
        assign pre_hur = pre_time[17:14]+pre_time[19:18]*10;			
	    //
        assign current_time = {hur10,hur0,min10, min0,sec10,sec0};
		//
	    assign day_sec = sec0 + sec10*10 + min0*60 + min10*600 + curr_hur*3600; 		 
    
	//延迟一拍，用来启动并捕捉上升沿
        always @(posedge clkin or negedge rstn) begin
            if (~rstn) begin
                load_r <= 2'b0;
            end
            else if(load_en)begin
                load_r <= {load_r[0],load_en};
                end
        	else
                load_r <= 2'b0;   		
        end
        
        assign load_posedge = load_r[0] & ~load_r[1];//上升沿
	
    // 				
    clk_counter #
	             (  
	               .COUNTER_WIDTH(4),
	            	.MAX_COUNT_VALUE(9)
                 )	sec0_inst		
	            (
	            	.clkin    (clkin   ),       
	            	.rstn     (rstn    ),       
	            	.clk_en   (count_en),       
	            	.din      (pre_time[3:0]),  
	            	.load_en  (load_posedge ),                                
	            	.dout     (sec0    ),          
	            	.carry_bit(sec10_en)		
	             );  //seccond个位
				 
    clk_counter #
	             (  
	               .COUNTER_WIDTH(3),
	            	.MAX_COUNT_VALUE(5)
                 )	sec10_inst		
	            (
	            	.clkin    (clkin   ),       
	            	.rstn     (rstn    ),       
	            	.clk_en   (sec10_en),       
	            	.din      (pre_time[6:4]),  
	            	.load_en  (load_posedge ),                                
	            	.dout     (sec10   ),          
	            	.carry_bit(min0_en )		
	             );  //seccond十位				 

    clk_counter #
	             (  
	               .COUNTER_WIDTH(4),
	            	.MAX_COUNT_VALUE(9)
                 )	min0_inst		
	            (
	            	.clkin    (clkin   ),       
	            	.rstn     (rstn    ),       
	            	.clk_en   (min0_en&&sec10_en),       
	            	.din      (pre_time[10:7]   ),  
	            	.load_en  (load_posedge     ),                                
	            	.dout     (min0    ),          
	            	.carry_bit(min10_en)		
	             );  //minute个位
				 
    clk_counter #
	             (  
	               .COUNTER_WIDTH(3),
	            	.MAX_COUNT_VALUE(5)
                 )	min10_inst		
	            (
	            	.clkin    (clkin   ),       
	            	.rstn     (rstn    ),       
	            	.clk_en   (min10_en),       
	            	.din      (pre_time[13:11]),  
	            	.load_en  (load_posedge   ),                                
	            	.dout     (min10   ),          
	            	.carry_bit(hur0_en )		
	             );  //minute十位				 
				 				 
    // 

     clk_counter #
	             (  
	               .COUNTER_WIDTH(5),
	            	.MAX_COUNT_VALUE(23)
                 )	hur_inst		
	            (
	            	.clkin    (clkin   ),       
	            	.rstn     (rstn    ),       
	            	.clk_en   (hur0_en&&min10_en),       
	            	.din      (pre_hur  ),  
	            	.load_en  (load_posedge),                                
	            	.dout     (curr_hur    ),          
	            	.carry_bit(day_end     )		
	             );  //hour十六进制				 

    always@(*)
       begin
	      if(curr_hur>=5'd20)begin
	                           hur10 = 2'd2;
                               curr_hur_tmp = curr_hur-5'd20;
							   hur0  = curr_hur_tmp[3:0];
                             end							   
	      else  if(curr_hur>=5'd10)begin
	                           hur10 = 2'd1;
                               curr_hur_tmp = curr_hur-5'd10;
							   hur0  = curr_hur_tmp[3:0];
                             end	
              else          begin
	                           hur10 = 2'd0;
							   hur0  = curr_hur[3:0];
                             end		   	   
        end	   
				 
    endmodule //time_counter
	
	//	 
	module clk_counter #
	  (  
	    parameter COUNTER_WIDTH = 4,
		parameter MAX_COUNT_VALUE = 9
      )			
	 (
		input   clkin,                        //10MHz,专用精确时钟。   
		input   rstn,                         //系统复位信号,异步
		input   clk_en,                       //低位来的计数脉冲，以时钟使能信号形式出现
		input  [COUNTER_WIDTH-1:0] din,       //CPU送来的预置数据
		input  load_en,                       //CPU送来的预置数据使能脉冲。
		                                      //前级须用100MHz时钟同步到10MHz时钟域
		output reg [COUNTER_WIDTH-1:0] dout,  //计数时钟输出
		output wire   carry_bit		
	  );
 
	   //
       always @(posedge clkin or negedge rstn)
         if (!rstn)
                   dout <= {COUNTER_WIDTH{1'b0}};	 
          else if (load_en) 
                          dout <= din;	 				  
	         else if (clk_en) begin
                          if (dout >= MAX_COUNT_VALUE) 
                                 dout <= {COUNTER_WIDTH{1'b0}};	  			
                              else
                                 dout <= dout + 1'b1;
                          end 
	   //				   
       assign carry_bit = (dout == MAX_COUNT_VALUE) && clk_en;	
       
	   //
    endmodule //clk_counter
	

//-------------------------------------------------------------------------------------------------------//	
	module date_latch(
		input wire  clkin,                      //10MHz,专用精确时钟。
		input wire  rstn,                       //系统复位信号,异步
		input wire  latch_en,                   //来自CPU的数据锁存信号，需要提取一个clk周期的上边沿使能信号
		input wire  [35:0] pre_date,            //预置数据
		                                        //年(8bit)、月(5bit)、日(6bit)、周(7bit)、年日(10bit)
		output  reg [35:0] curr_date	      
	    );
    //
      reg [1:0]   latch_en_r;
      wire        latch_en_posedge;

   //延迟一拍，用来启动并捕捉上升沿
   always @(posedge clkin or negedge rstn) begin
       if (~rstn) begin
           latch_en_r <= 2'b0;   end   
          else 
           latch_en_r <= {latch_en_r[0],latch_en};
         end
   
   assign latch_en_posedge = latch_en_r[0] & ~latch_en_r[1];//上升沿
   
   //
   always @(posedge clkin or negedge rstn)
     begin  
	  if (!rstn) 
            curr_date <= 36'b0;
		else if (latch_en_posedge)	 
              curr_date <= pre_date;            			
           else
              curr_date <= curr_date;		  
	 end
	endmodule //date_latch	
	
    //---------------------------------------------------------------------------------------------------------//
    //	1. current time_r为待发送的时间数据，分别对应于B码的90位数据的帧结构。
    //  2. 单一主时钟clk，10MHz，接外部恒温晶振。
    //  3. 为了加快仿真，只需修改MAX_2MS、_5MS、_8MS和、10MS。
    //  4. 提取的数据随着接收到的码元逐组（Pn）即时更新。
    //  5. pps信号比Pr晚2个clk周期。
    //  6. decode_en为编码使能信号，高电平启动，低电平停止。
    //  7. 综合后结果：46个LUT，106个FF。
    //---------------------------------------------------------------------------------------------------------//
    module b_decoder #(  
        parameter MAX_6MS  = 59_999,
        parameter MAX_3MS  = 29_999
       )
      (
        input  wire clk,   //10MHz     
        input  wire rst_n,
        input  wire irig_b,
    	input  wire decode_en,        // 启动解码，高电平有效；低电平禁止解码
        output reg [6:0] sec ,        //2位BCD码
        output reg [6:0] min ,        //2位BCD码
        output reg [5:0] hur ,        //2位BCD码
        output reg [9:0] year_day,    //3位BCD码；本年的第几天
        output reg [7:0] year,        //2位BCD码
        output reg [16:0] day_sec,	  //16为binary码，本日的第几秒
    	output reg  pps     //比Pr上升沿晚2个clk周期的时间，脉宽100ms 
        );
        
    reg [1:0]   irig_b_r;
    wire        irig_b_posedge;
    wire        irig_b_negedge;
    
    //延迟一拍，用来启动并捕捉上升沿和下降沿
    always @(posedge clk or negedge rst_n) begin
        if (~rst_n) begin
            irig_b_r <= 2'b0;
        end
        else if(decode_en) begin
            irig_b_r <= {irig_b_r[0],irig_b};
            end
    		else
            irig_b_r <= 2'b0;		
    end
    
    assign irig_b_posedge = irig_b_r[0] & ~irig_b_r[1];//上升沿
    assign irig_b_negedge = irig_b_r[1] & ~irig_b_r[0];//下降沿
    
    //
    reg [16:0]  cnt_pos;   //10ms delay counter, 17bits
    reg [1:0]   decode ;   
    reg [8:0]  curr_time;  //10 x 9bit码元
    reg [3:0]   state  ;
    localparam  IDLE = 4'b0000;
    localparam  PR   = 4'b0001;
    localparam  SEC  = 4'b0010;
    localparam  MIN  = 4'b0011;
    localparam  HOUR = 4'b0100;
    localparam  DAY1 = 4'b0101;
    localparam  DAY2 = 4'b0110;
    localparam  YEAR = 4'b0111;
    localparam  S0   = 4'b1000;
    localparam  S1   = 4'b1001;
    localparam  S2   = 4'b1010;
    localparam  S3   = 4'b1011;
    
    //对b码的高电平进行计数
    always @(posedge clk or negedge rst_n) begin
        if (~rst_n) begin
            cnt_pos <= 17'd0;
        end
        else if (irig_b_posedge) begin
            cnt_pos <= 17'd1;
            end
           else if (irig_b) begin
               cnt_pos <= cnt_pos + 17'b1;
               end
              else begin
                 cnt_pos <= cnt_pos;
                end
         end
    
    //在下降沿对波形进行编码
    always @(posedge clk or negedge rst_n) begin
        if (~rst_n) begin
                decode <= 2'b00;
             end
        else if (irig_b_negedge) begin
    	        decode <= (cnt_pos >= MAX_6MS)? 2'b11 :  // p码
    	                  (cnt_pos >= MAX_3MS)? 2'b01 :  // 1码
    	                                        2'b00 ;  // 0码 
                 end
             else
                decode <= decode;
        end
    
    //主状态机
    always @(posedge clk or negedge rst_n) begin
        if (~rst_n) begin
                   state  <= IDLE;
                   pps    <= 1'b0;	
                   curr_time <= 9'd0;		   
                 end
        else if (irig_b_posedge) begin
    	 case (state)
            IDLE : begin
                if (decode[1] == 1'b1) begin
                    state <= PR;
                end
                else begin
                   state <= state;
                   pps    <= 1'b0;					   
                end
            end
            PR : begin
                if (decode[1] == 1'b1) begin
                    state  <= SEC;
                end
                else if (decode[1] == 1'b0) begin
                    state <= IDLE;
                end
                else begin
                    state  <= state;	
                end
            end
            SEC : begin
                if (decode[1] == 1'b1) begin
                    state <= MIN;
                    pps   <= 1'b0;				
                end
                else begin
                    state  <= state;					
    				curr_time <= {decode[0],curr_time[8:1]};
                end
            end
            MIN : begin
                if (decode[1] == 1'b1) begin
                    state <= HOUR;
                end
                else begin
                    state <= state;
    				curr_time <= {decode[0],curr_time[8:1]};			
                end
            end
            HOUR : begin
                if (decode[1] == 1'b1) begin
                    state <= DAY1;
                end
                else begin
                    state <= state;
    				curr_time <= {decode[0],curr_time[8:1]};			
                end
            end
            DAY1 : begin
                if (decode[1] == 1'b1) begin
                    state <= DAY2;
                end
                else begin
                    state <= state;
    				curr_time <= {decode[0],curr_time[8:1]};			
               end
            end
            DAY2 : begin
                if (decode[1] == 1'b1) begin
                    state <= YEAR;
                end
                else begin
                    state <= state;	
    				curr_time <= {decode[0],curr_time[8:1]};				
                end
            end
            YEAR : begin
                if (decode[1] == 1'b1) begin
                    state <= S0;
                end
                else begin
                    state <= state;
    				curr_time <= {decode[0],curr_time[8:1]};			
                end
            end
            S0 : begin
                if (decode[1] == 1'b1) begin
                    state <= S1;
                end
                 else begin
                    state <= state;
    				curr_time <= {decode[0],curr_time[8:1]};					
                end
            end
            S1 : begin
                if (decode[1] == 1'b1) begin
                    state <= S2;
                end
                else begin
                    state <= state;
    				curr_time <= {decode[0],curr_time[8:1]};					
                end
            end
            S2 : begin
                if (decode[1] == 1'b1) begin
                    state <= S3;
                end
                else begin
                    state <= state;
    				curr_time <= {decode[0],curr_time[8:1]};				
                end
            end
            S3 : begin
                if (decode[1] == 1'b1) begin
                    state  <= PR;				
    				pps    <= 1'b1; //发送同步脉冲	
                end
                else begin
                    state <= state;
    				curr_time <= {decode[0],curr_time[8:1]};				
                end
            end
    		
            default: state <= IDLE;
        endcase
      end
    end
    
    //在下降沿锁定输出时间
       reg [8:0] day1_reg;
       reg [8:0] sbs1_reg;
    always @(posedge clk or negedge rst_n) begin
        if (~rst_n) begin    
                 sec      <= 7'd0;
                 min      <= 7'd0;
                 hur      <= 6'd0;
                 year_day <= 10'd0;             
                 year     <= 8'd0;
                 day_sec  <= 17'd0;    
                 day1_reg <= 9'd0;
                 sbs1_reg <= 9'd0;		 
              end
        else if (irig_b_negedge && (cnt_pos>=MAX_6MS)) begin
                case(state)
                  SEC   : sec  <= {curr_time[8:6],curr_time[4:1]};        //2位BCD码    
                  MIN   : min  <= {curr_time[7:5],curr_time[3:0]};  
                  HOUR  : hur  <= {curr_time[6:5],curr_time[3:0]}; 
                  DAY1  : day1_reg <= curr_time; 
                  DAY2  : year_day <= {curr_time[1:0],day1_reg[8:5],day1_reg[3:0]}; //3位BCD码；本年的第几天
                  YEAR  : year <= {curr_time[7:5],curr_time[3:0]};  
                  S0    :  ;
                  S1    :  ;
                  S2    : sbs1_reg <= curr_time;
                  S3    : day_sec <= {curr_time[7:0],sbs1_reg}; 	  //16为binary码，本日的第几秒	
                 endcase     
               end
            end
    
    endmodule //b_decoder	
    	
   //-----------------------------------------------------------------------------------------------------------//
   //	1. current time_r为待发送的时间数据，分别对应于B码的90位数据的帧结构。
   //  2. 单一主时钟clk，10MHz，接外部恒温晶振。
   //  3. 为了加快仿真，只需修改MAX_2MS、_5MS、_8MS和、10MS。
   //  4. IRIG-B码波形比pps信号晚1个clk周期。
   //  5. encode_en为编码使能信号，高电平启动，低电平禁止。
   //  6. 综合后结果：89个LUT，87个FF。
   //-----------------------------------------------------------------------------------------------------------// 
   module b_encoder #(
       parameter MAX_10MS = 99_999,
       parameter MAX_8MS  = 79_999,
       parameter MAX_5MS  = 49_999,
       parameter MAX_2MS  = 19_999
      )
      (
       input wire clk,    //10MHz
       input wire rst_n,	
   	input wire pps,	   //来自软时钟亚秒的进位脉冲，脉宽一个clk周期
   	input wire encode_en,         //来自CPU的使能开关   
       input wire [6:0] sec ,        //2位BCD码
       input wire [6:0] min ,        //2位BCD码
       input wire [5:0] hur ,        //2位BCD码
       input wire [9:0] year_day,    //3位BCD码；本年的第几天
       input wire [7:0] year,        //2位BCD码
       input wire [16:0] day_sec,	  //17位binary码，本日的第几秒	
   	//	
       output reg irig_b
   );
       
   //
   reg [89:0]  curr_time_r;
   //锁存当前时间，并按irig_b码序排列
   always @(posedge clk or negedge rst_n) 
   begin
       if (~rst_n)
           curr_time_r <= 90'd0;
       else if(pps&&encode_en) 
           curr_time_r <= { 
            	  1'b0, day_sec,   // Straight Binary Seconds
   			  18'b0,           // Control Functions
   		      year[7:4],1'b0,year[3:0],         // year		
   		      7'b0,year_day[9:8],               // day of year		
   		      year_day[7:4],1'b0,year_day[3:0], // day of year, BCD coding	
   		      2'b0, hur[5:4], 1'b0, hur[3:0],   // hours
   		      1'b0, min[6:4], 1'b0, min[3:0],   // minutes
   		      sec[6:4], 1'b0, sec[3:0], 1'b0    // seconds
   		    };
   end
   	
   //
   reg [6:0] cnt_1s;     //1秒内要发送100个码元（含90bit）
   reg [16:0]cnt_10ms;   //10ms delay counter, 17bits
   reg [3:0] cnt_1p;     //1个Pn组内要发送9位码元
   
   reg [2:0]  state;
   localparam IDLE      = 3'd0;
   localparam SEND_PR   = 3'd1;
   localparam SEND_DATA = 3'd2;
   localparam WORD_END  = 3'd3;
   localparam FRAME_END = 3'd4;
   localparam SEND_PN   = 3'd5;
   
   always @(posedge clk or negedge rst_n) begin
       if (~rst_n) begin
           state    <= IDLE;
           cnt_10ms <= 17'd0;
           cnt_1s   <= 7'd1;
           cnt_1p   <= 4'd1;	
           irig_b   <= 1'b0;		
       end
       else case (state)
           IDLE : begin
                  if(pps&&encode_en) begin           
   			      state  <= SEND_PR;
                     irig_b <= 1'b1;	
                     end  			  
				else begin
                      state    <= IDLE;
                      cnt_10ms <= 17'd0;
                      cnt_1s   <= 7'd1;
                      cnt_1p   <= 4'd1;	
                      irig_b   <= 1'b0;					  
                  end 
			   end
		
		SEND_PR : begin           //发送Pr
            if (cnt_10ms == MAX_10MS-1) begin
                state <= SEND_DATA;
                cnt_10ms <= 17'd0;
            end
            else  begin
                cnt_10ms <= cnt_10ms + 17'd1;
            end

            if(cnt_10ms <= MAX_8MS-1)begin
                irig_b <= 1'b1;
            end
            else begin
                irig_b <= 1'b0;
            end
        end
		
        SEND_DATA : begin         //发送1位数据
            if (cnt_10ms == MAX_10MS - 1) begin
                    state    <= WORD_END;
                    cnt_10ms <= 17'd0;
                    cnt_1s   <= cnt_1s + 7'd1;	
                    cnt_1p   <= cnt_1p + 1;					
                end
            else  begin
                   cnt_10ms <= cnt_10ms + 17'd1;
                end

            if(cnt_10ms <= (curr_time_r[cnt_1s] ? MAX_5MS : MAX_2MS))begin
                irig_b <= 1'b1;
            end
            else begin
                irig_b <= 1'b0;
            end
        end
		
        WORD_END : begin          //判断一组数据（9个码元）是否已经发送完毕
            if (cnt_1p == 7'd9) begin
                      state  <= SEND_PN;				  
                      cnt_1p <= 4'd0;						
                     end
            else  begin
                    state  <= SEND_DATA;					
                  end
         end

		SEND_PN : begin           //发送Pn
            if (cnt_10ms == MAX_10MS - 10) begin
                state <= FRAME_END ;
                cnt_10ms <= 17'd0;
            end
            else  begin
                cnt_10ms <= cnt_10ms + 17'd1;
            end

            if(cnt_10ms <= MAX_8MS)begin
                irig_b <= 1'b1;
            end
            else begin
                irig_b <= 1'b0;
            end
        end		
	
        FRAME_END : begin         //判断一帧数据（90个码元）是否发送完
            if (cnt_1s == 7'd90) begin
                      state    <= IDLE;
                      cnt_10ms <= 17'd0;
                      cnt_1s   <= 7'd1;
                      cnt_1p   <= 4'd1;	
                      irig_b   <= 1'b0;							   
		   	  	   end
               else  begin
		   	      state <= SEND_DATA;                
               end
             end
             
       default :  state <= IDLE; 
    endcase
  end

  endmodule //b_encoder   	
	
//---------------------------------------------------the last line-------------------------------------------------------//	