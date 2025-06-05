//==================================================================================================//
//  Revision Time:                                                                                  //
//        2025年5月11日。                                                                           //
//  Founctions：                                                                                    //
//        软时钟+功放+开关量读写等；其功能的具体描述见对应.v文件的文件头。                          //
//--------------------------------------------------------------------------------------------------//
// 写寄存器的含义|slv_reg0 slv_reg1 slv_reg2 slv_reg3 slv_reg4 slv_reg5 slv_reg6 slv_reg7 备注      // 
//--------------------------------------------------------------------------------------------------//          
//	 pre_year    |[17:10]                                                                 BCD码     //
//   pre_month   |         [17:13]                                                        BCD码     //
//   pre_day     |         [12:7]                                                         BCD码     //
//   pre_week    |         [6:0]                                                         独热码     //
//   pre_yearday |[9:0]                                                                   BCD码     //
//	 pre_hour    |                  [19:14]                                               BCD码     //
//   pre_minute  |                  [13:7]                                                BCD码     //
//   pre_second  |                  [6:0]                                                 BCD码     //
//	 wr_date     |                           [0]                                        预置日期    //
//   wr_time     |                                   [0]                                 预置时间   //         
//   bm_encode_en|                                            [0]                     允许B码输出   //
//   bm_decode_en|                                                      [0]           启动B码校时   //
//   pps_clr_en  |                                                               [0] 亚秒计数器清零 //
//--------------------------------------------------------------------------------------------------//		
// 读寄存器的含义：                                                                                 //
//  slv_reg0： {14'd0, bm_year[7:0],   bm_yearday[9:0]};                                            //
//  slv_reg1： {12'd0, bm_hour[5:0],   bm_minute[6:0],     bm_second[6:0]};                         //
//  slv_reg2： {15'd0, bm_daysec[16:0]};                                                            //
//                                                                                                  //
//  slv_reg3： {14'd0, curr_year[7:0],  curr_yearday[9:0]};                                         //
//  slv_reg4： {14'd0, curr_month[4:0], curr_day[5:0],    curr_week[6:0]};                          //
//  slv_reg5： {12'd0, curr_hour[5:0],  curr_minute[6:0], curr_second[6:0]};                        //
//  slv_reg6： {15'd0, curr_daysec[16:0]};                                                          //
//  slv_reg7： {8'd0,  curr_subsec[23:0]};                                                          // 
//--------------------------------------------------------------------------------------------------//
//  写寄存器	        | slv_reg8	reg9	reg10	reg11	 reg12	  reg13     reg14     reg15     // 
//--------------------------------------------------------------------------------------------------//
//  onoff    byte_num   | [18:16]					                                                //
//	         rw_modes   | [31:30]                                                                   //	
//	         start	    | [24]	                                                                    //				
//           din	    |					                         [31:0]                         //
//                      |                                                                           //
//wrserial	ld1595_en   |                                                        [0]                //					
//       	ld595_en    |                                                        [1]                //					
// 	        start	    |                                                        [8]                //					
// 	        din0	    |          [31:0]                                                           //				
// 	        din1	    |	               [31:0]                                                   //			
// 	        din2	    |		                   [31:0]	                                        //	
// 	        din3	    |			                        [31:0]                                  //	
//                      |                                                                           //                
// rdserial	enable	    | 	                                                                [0]     //
//--------------------------------------------------------------------------------------------------//				
//读寄存器          	| slv_reg8	slv_reg9	slv_reg10	slv_reg11	slv_reg12	slv_reg13       //
//--------------------------------------------------------------------------------------------------//
//onoff	   onoff_done	|  [31]					                                                    //
//      	dout		|            [31:0]				                                            //
//         latch_hour   |                                   [19:14]                                 //
//         latch_minute |                                   [13:7]                                  //
//         latch_second |                                   [6:0]                                   //
//         latch_daysec |                                                 [16:0]                    //
//         latch_subsec |                                                             [23:0]        //
//wrserial wrserial_done|  [15]					                                                    //
//rdserial	dataout		|	                     [7:0]			                                    //
//--------------------------------------------------------------------------------------------------//
//  注：                                                                                            //
//     读8路故障的硬件电路中，没有设置读完成信号。从读使能=>读完成需要大约0.5us的时间。             //
//==================================================================================================//

`timescale 1 ns / 1 ps

	module stimer_onoff_pa_rw_AXI #
	(
		// Users to add parameters here

		// User parameters ends
		// Do not modify the parameters beyond this line

		// Width of S_AXI data bus
		parameter integer C_S_AXI_DATA_WIDTH	= 32,
		// Width of S_AXI address bus
		parameter integer C_S_AXI_ADDR_WIDTH	= 6
	)
	(
		// Users to add ports here
        // FPGA interface
    	input  wire   clk_10MHz,      // 接10MHz的恒温晶振
		input wire   CLK25MHz,        // 接25MHz时钟
		input wire   CLK25MHz_RSTN,	  // 25MHz时钟的地
				
	   // FPGA 外部端口	 
       // soft timer	   
		input  wire   pps_gps   ,     // 来自GPS的秒同步信号，脉宽300ms
		input  wire   pps_in    ,     // 辅助对时，只要求秒同步
		input  wire   irig_b_in ,     // B码输入信号。用于设备同步，要求显示对时过程及对时成功反馈
		output wire   irig_b_out,     // B码输出，一直在发送   
		output wire   pps_50    ,     // pps脉冲输出，50%占空比，用于检测内部时钟稳定性	
		
		//onoff control interface
		output  wire onoff_cs,
		output  wire onoff_sclk,
		output  wire onoff_sdo,		
		input   wire onoff_sdi,
		output  wire onoff_done,       // 中断信号
		
		//power amplifier config interface
		output  wire wrserial_load,
		output  wire wrserial_sclk,
		output  wire wrserial_sdo,
		output  wire wrserial_done,     //中断信号
		
		//read 8' fail-signals
		output  wire rdserial_load,    
		output  wire rdserial_sclk,
		input   wire rdserial_sdi,

       // 软时钟对应的中断请求信号
	    output  wire   pps_gps2cpu,   // 3个pps信号，用于软件精确对时
	    output  wire   pps_in2cpu ,   // 来自外部引脚pps_in的辅助对时信号
        output  wire   pps_bm2cpu ,   // B码解码得到的pps信号
	    output  wire   bm_syn_end ,   // B码解码成功，并自动完成了软时钟的同步	
		output  wire   date_update,   // 00:00:00到了，请求CPU更行日期数据		
		
		// User ports ends
		// Do not modify the ports beyond this line

		// Global Clock Signal
		input wire  S_AXI_ACLK,       //100MHz
		// Global Reset Signal. This Signal is Active LOW
		input wire  S_AXI_ARESETN,    //100MHz时钟的地
		// Write address (issued by master, acceped by Slave)
		input wire [C_S_AXI_ADDR_WIDTH-1 : 0] S_AXI_AWADDR,
		// Write channel Protection type. This signal indicates the
    		// privilege and security level of the transaction, and whether
    		// the transaction is a data access or an instruction access.
		input wire [2 : 0] S_AXI_AWPROT,
		// Write address valid. This signal indicates that the master signaling
    		// valid write address and control information.
		input wire  S_AXI_AWVALID,
		// Write address ready. This signal indicates that the slave is ready
    		// to accept an address and associated control signals.
		output wire  S_AXI_AWREADY,
		// Write data (issued by master, acceped by Slave) 
		input wire [C_S_AXI_DATA_WIDTH-1 : 0] S_AXI_WDATA,
		// Write strobes. This signal indicates which byte lanes hold
    		// valid data. There is one write strobe bit for each eight
    		// bits of the write data bus.    
		input wire [(C_S_AXI_DATA_WIDTH/8)-1 : 0] S_AXI_WSTRB,
		// Write valid. This signal indicates that valid write
    		// data and strobes are available.
		input wire  S_AXI_WVALID,
		// Write ready. This signal indicates that the slave
    		// can accept the write data.
		output wire  S_AXI_WREADY,
		// Write response. This signal indicates the status
    		// of the write transaction.
		output wire [1 : 0] S_AXI_BRESP,
		// Write response valid. This signal indicates that the channel
    		// is signaling a valid write response.
		output wire  S_AXI_BVALID,
		// Response ready. This signal indicates that the master
    		// can accept a write response.
		input wire  S_AXI_BREADY,
		// Read address (issued by master, acceped by Slave)
		input wire [C_S_AXI_ADDR_WIDTH-1 : 0] S_AXI_ARADDR,
		// Protection type. This signal indicates the privilege
    		// and security level of the transaction, and whether the
    		// transaction is a data access or an instruction access.
		input wire [2 : 0] S_AXI_ARPROT,
		// Read address valid. This signal indicates that the channel
    		// is signaling valid read address and control information.
		input wire  S_AXI_ARVALID,
		// Read address ready. This signal indicates that the slave is
    		// ready to accept an address and associated control signals.
		output wire  S_AXI_ARREADY,
		// Read data (issued by slave)
		output wire [C_S_AXI_DATA_WIDTH-1 : 0] S_AXI_RDATA,
		// Read response. This signal indicates the status of the
    		// read transfer.
		output wire [1 : 0] S_AXI_RRESP,
		// Read valid. This signal indicates that the channel is
    		// signaling the required read data.
		output wire  S_AXI_RVALID,
		// Read ready. This signal indicates that the master can
    		// accept the read data and response information.
		input wire  S_AXI_RREADY
	);

	// AXI4LITE signals
	reg [C_S_AXI_ADDR_WIDTH-1 : 0] 	axi_awaddr;
	reg  	axi_awready;
	reg  	axi_wready;
	reg [1 : 0] 	axi_bresp;
	reg  	axi_bvalid;
	reg [C_S_AXI_ADDR_WIDTH-1 : 0] 	axi_araddr;
	reg  	axi_arready;
	reg [C_S_AXI_DATA_WIDTH-1 : 0] 	axi_rdata;
	reg [1 : 0] 	axi_rresp;
	reg  	axi_rvalid;

	// Example-specific design signals
	// local parameter for addressing 32 bit / 64 bit C_S_AXI_DATA_WIDTH
	// ADDR_LSB is used for addressing 32/64 bit registers/memories
	// ADDR_LSB = 2 for 32 bits (n downto 2)
	// ADDR_LSB = 3 for 64 bits (n downto 3)
	localparam integer ADDR_LSB = (C_S_AXI_DATA_WIDTH/32) + 1;
	localparam integer OPT_MEM_ADDR_BITS = 3;
	//----------------------------------------------
	//-- Signals for user logic register space example
	//------------------------------------------------
	//-- Number of Slave Registers 8
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg0;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg1;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg2;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg3;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg4;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg5;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg6;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg7;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg8;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg9;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg10;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg11;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg12;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg13;	
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg14;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg15;	
	wire	 slv_reg_rden;
	wire	 slv_reg_wren;
	reg [C_S_AXI_DATA_WIDTH-1:0]    reg_data_out;
	integer	 byte_index;
	reg	 aw_en;

	// I/O Connections assignments

	assign S_AXI_AWREADY	= axi_awready;
	assign S_AXI_WREADY	= axi_wready;
	assign S_AXI_BRESP	= axi_bresp;
	assign S_AXI_BVALID	= axi_bvalid;
	assign S_AXI_ARREADY	= axi_arready;
	assign S_AXI_RDATA	= axi_rdata;
	assign S_AXI_RRESP	= axi_rresp;
	assign S_AXI_RVALID	= axi_rvalid;
	// Implement axi_awready generation
	// axi_awready is asserted for one S_AXI_ACLK clock cycle when both
	// S_AXI_AWVALID and S_AXI_WVALID are asserted. axi_awready is
	// de-asserted when reset is low.

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_awready <= 1'b0;
	      aw_en <= 1'b1;
	    end 
	  else
	    begin    
	      if (~axi_awready && S_AXI_AWVALID && S_AXI_WVALID && aw_en)
	        begin
	          // slave is ready to accept write address when 
	          // there is a valid write address and write data
	          // on the write address and data bus. This design 
	          // expects no outstanding transactions. 
	          axi_awready <= 1'b1;
	          aw_en <= 1'b0;
	        end
	        else if (S_AXI_BREADY && axi_bvalid)
	            begin
	              aw_en <= 1'b1;
	              axi_awready <= 1'b0;
	            end
	      else           
	        begin
	          axi_awready <= 1'b0;
	        end
	    end 
	end       

	// Implement axi_awaddr latching
	// This process is used to latch the address when both 
	// S_AXI_AWVALID and S_AXI_WVALID are valid. 

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_awaddr <= 0;
	    end 
	  else
	    begin    
	      if (~axi_awready && S_AXI_AWVALID && S_AXI_WVALID && aw_en)
	        begin
	          // Write Address latching 
	          axi_awaddr <= S_AXI_AWADDR;
	        end
	    end 
	end       

	// Implement axi_wready generation
	// axi_wready is asserted for one S_AXI_ACLK clock cycle when both
	// S_AXI_AWVALID and S_AXI_WVALID are asserted. axi_wready is 
	// de-asserted when reset is low. 

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_wready <= 1'b0;
	    end 
	  else
	    begin    
	      if (~axi_wready && S_AXI_WVALID && S_AXI_AWVALID && aw_en )
	        begin
	          // slave is ready to accept write data when 
	          // there is a valid write address and write data
	          // on the write address and data bus. This design 
	          // expects no outstanding transactions. 
	          axi_wready <= 1'b1;
	        end
	      else
	        begin
	          axi_wready <= 1'b0;
	        end
	    end 
	end       

	// Implement memory mapped register select and write logic generation
	// The write data is accepted and written to memory mapped registers when
	// axi_awready, S_AXI_WVALID, axi_wready and S_AXI_WVALID are asserted. Write strobes are used to
	// select byte enables of slave registers while writing.
	// These registers are cleared when reset (active low) is applied.
	// Slave register write enable is asserted when valid address and data are available
	// and the slave is ready to accept the write address and write data.
	assign slv_reg_wren = axi_wready && S_AXI_WVALID && axi_awready && S_AXI_AWVALID;

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      slv_reg0 <= 0;
	      slv_reg1 <= 0;
	      slv_reg2 <= 0;
	      slv_reg3 <= 0;
	      slv_reg4 <= 0;
	      slv_reg5 <= 0;
	      slv_reg6 <= 0;
	      slv_reg7 <= 0;
	      slv_reg8 <= 0;
	      slv_reg9 <= 0;
	      slv_reg10<= 0;
	      slv_reg11<= 0;
	      slv_reg12<= 0;
	      slv_reg13<= 0;	
	      slv_reg14<= 0;
	      slv_reg15<= 0;		  
	    end 
	  else begin
	    if (slv_reg_wren)
	      begin
	        case ( axi_awaddr[ADDR_LSB+OPT_MEM_ADDR_BITS:ADDR_LSB] )
	          4'h0:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 0
	                slv_reg0[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h1:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 1
	                slv_reg1[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h2:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 2
	                slv_reg2[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h3:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 3
	                slv_reg3[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h4:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 4
	                slv_reg4[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h5:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 5
	                slv_reg5[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h6:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 6
	                slv_reg6[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h7:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 7
	                slv_reg7[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h8:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 8
	                slv_reg8[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end 
	          4'h9:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 9
	                slv_reg9[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end 
	          4'd10:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 10
	                slv_reg10[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end 
	          4'd11:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 11
	                slv_reg11[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end 
	          4'd12:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 12
	                slv_reg12[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end 
	          4'd13:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 13
	                slv_reg13[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end 
	          4'd14:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 14
	                slv_reg14[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end 
	          4'd15:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 15
	                slv_reg15[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end 				  
				  
	          default : begin
	                      slv_reg0 <= slv_reg0;
	                      slv_reg1 <= slv_reg1;
	                      slv_reg2 <= slv_reg2;
	                      slv_reg3 <= slv_reg3;
	                      slv_reg4 <= slv_reg4;
	                      slv_reg5 <= slv_reg5;
	                      slv_reg6 <= slv_reg6;
	                      slv_reg7 <= slv_reg7;
	                      slv_reg8 <= slv_reg8;
	                      slv_reg9 <= slv_reg9;						  
	                      slv_reg10 <= slv_reg10;
	                      slv_reg11 <= slv_reg11;
	                      slv_reg12 <= slv_reg12;
	                      slv_reg13 <= slv_reg13;
	                      slv_reg14 <= slv_reg14;
	                      slv_reg15 <= slv_reg15;						  
	                    end
	        endcase
	      end
	  end
	end    

	// Implement write response logic generation
	// The write response and response valid signals are asserted by the slave 
	// when axi_wready, S_AXI_WVALID, axi_wready and S_AXI_WVALID are asserted.  
	// This marks the acceptance of address and indicates the status of 
	// write transaction.

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_bvalid  <= 0;
	      axi_bresp   <= 2'b0;
	    end 
	  else
	    begin    
	      if (axi_awready && S_AXI_AWVALID && ~axi_bvalid && axi_wready && S_AXI_WVALID)
	        begin
	          // indicates a valid write response is available
	          axi_bvalid <= 1'b1;
	          axi_bresp  <= 2'b0; // 'OKAY' response 
	        end                   // work error responses in future
	      else
	        begin
	          if (S_AXI_BREADY && axi_bvalid) 
	            //check if bready is asserted while bvalid is high) 
	            //(there is a possibility that bready is always asserted high)   
	            begin
	              axi_bvalid <= 1'b0; 
	            end  
	        end
	    end
	end   

	// Implement axi_arready generation
	// axi_arready is asserted for one S_AXI_ACLK clock cycle when
	// S_AXI_ARVALID is asserted. axi_awready is 
	// de-asserted when reset (active low) is asserted. 
	// The read address is also latched when S_AXI_ARVALID is 
	// asserted. axi_araddr is reset to zero on reset assertion.

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_arready <= 1'b0;
	      axi_araddr  <= 32'b0;
	    end 
	  else
	    begin    
	      if (~axi_arready && S_AXI_ARVALID)
	        begin
	          // indicates that the slave has acceped the valid read address
	          axi_arready <= 1'b1;
	          // Read address latching
	          axi_araddr  <= S_AXI_ARADDR;
	        end
	      else
	        begin
	          axi_arready <= 1'b0;
	        end
	    end 
	end       

	// Implement axi_arvalid generation
	// axi_rvalid is asserted for one S_AXI_ACLK clock cycle when both 
	// S_AXI_ARVALID and axi_arready are asserted. The slave registers 
	// data are available on the axi_rdata bus at this instance. The 
	// assertion of axi_rvalid marks the validity of read data on the 
	// bus and axi_rresp indicates the status of read transaction.axi_rvalid 
	// is deasserted on reset (active low). axi_rresp and axi_rdata are 
	// cleared to zero on reset (active low).  
	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_rvalid <= 0;
	      axi_rresp  <= 0;
	    end 
	  else
	    begin    
	      if (axi_arready && S_AXI_ARVALID && ~axi_rvalid)
	        begin
	          // Valid read data is available at the read data bus
	          axi_rvalid <= 1'b1;
	          axi_rresp  <= 2'b0; // 'OKAY' response
	        end   
	      else if (axi_rvalid && S_AXI_RREADY)
	        begin
	          // Read data is accepted by the master
	          axi_rvalid <= 1'b0;
	        end                
	    end
	end  
	
	   // 变量声明
		wire  [7:0]  bm_year   ;    // 当前日期值,BCD码
        wire  [9:0]  bm_yearday;    // BCD码 
 		wire  [5:0]  bm_hour   ;    // 当前时间值，BCD码
        wire  [6:0]  bm_minute ;    // BCD码 
        wire  [6:0]  bm_second ;    // BCD码
		wire  [16:0] bm_daysec ;    // 日秒，二进制 	   

		wire  [7:0]  curr_year   ;   // 当前日期值,BCD码
        wire  [4:0]  curr_month  ;   // BCD码 
        wire  [5:0]  curr_day    ;   // BCD码 
        wire  [6:0]  curr_week   ;   // 独热码
        wire  [9:0]  curr_yearday;   // BCD码 
 		wire  [5:0]  curr_hour   ;   // 当前时间值，BCD码
        wire  [6:0]  curr_minute ;   // BCD码 
        wire  [6:0]  curr_second ;   // BCD码
		wire  [16:0] curr_daysec ;   // 日秒，二进制 
        wire  [23:0] curr_subsec ;   // 亚秒，二进制	
		
	    wire  [31:0] onoff_dout;     // 开入数据
        wire         onoff_latch;	 // 上升沿打时戳	
	    wire  [7:0]  data_rdserial;  // 故障数据
		
	   //时戳数据	
 		reg   [5:0]  latch_hour   ;   // BCD码
        reg   [6:0]  latch_minute ;   // BCD码 
        reg   [6:0]  latch_second ;   // BCD码
		reg   [16:0] latch_daysec ;   // 日秒，二进制 
        reg   [23:0] latch_subsec ;   // 亚秒，二进制		

	// Implement memory mapped register select and read logic generation
	// Slave register read enable is asserted when valid address is available
	// and the slave is ready to accept the read address.
	assign slv_reg_rden = axi_arready & S_AXI_ARVALID & ~axi_rvalid;
	always @(*)
	begin
	      // Address decoding for reading registers
	      case ( axi_araddr[ADDR_LSB+OPT_MEM_ADDR_BITS:ADDR_LSB] )  
		  //当前B码解码的时间数据
	        4'h0   : reg_data_out <= {14'd0, bm_year, bm_yearday};
	        4'h1   : reg_data_out <= {12'd0, bm_hour, bm_minute, bm_second};
	        4'h2   : reg_data_out <= {15'd0, bm_daysec};
		  //软时钟的实时时间数据
	        4'h3   : reg_data_out <= {14'd0, curr_year, curr_yearday};
	        4'h4   : reg_data_out <= {14'd0, curr_month, curr_day, curr_week};
	        4'h5   : reg_data_out <= {12'd0, curr_hour, curr_minute, curr_second};
	        4'h6   : reg_data_out <= {15'd0, curr_daysec};
	        4'h7   : reg_data_out <= {8'd0,  curr_subsec};
		  //开关量读写、二级功放配置及故障读出	
	        4'h8   : reg_data_out <= {onoff_done,15'd0,wrserial_done,15'd0};
	        4'h9   : reg_data_out <= onoff_dout;   //datas read from 74HC165 		
	        4'd10  : reg_data_out <= {24'd0,data_rdserial};  //fail signals
		  //时戳数据	
	        4'd11  : reg_data_out <= {12'd0, latch_hour, latch_minute, latch_second};			
	        4'd12  : reg_data_out <= {15'd0, latch_daysec};
	        4'd13  : reg_data_out <= {8'd0,  latch_subsec};						
	        default: reg_data_out <= 0;
	      endcase
	end

	// Output register or memory read data
	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_rdata  <= 0;
	    end 
	  else
	    begin    
	      // When there is a valid read address (S_AXI_ARVALID) with 
	      // acceptance of read address by the slave (axi_arready), 
	      // output the read dada 
	      if (slv_reg_rden)
	        begin
	          axi_rdata <= reg_data_out;     // register read data
	        end   
	    end
	end    

	// Add user logic here
    soft_timer #
    	  (      	
          )			
	   soft_timer_inst(
       // system clock and reset signal   
    	.clkin(clk_10MHz     ),          // 10MHz
		.rst_n(CLK25MHz_RSTN ),          // 借用25MHz地，异步复位
		
	   // FPGA 外部端口	  
		.pps_gps   (pps_gps   ),         // 来自GPS的秒同步信号，脉宽300ms
		.pps_in    (pps_in    ),         // 辅助对时，只要求秒同步
		.irig_b_in (irig_b_in ),         // B码输入信号。用于设备同步，要求显示对时过程及对时成功反馈
		.irig_b_out(irig_b_out),         // B码输出，一直发   
		.pps_50    (pps_50    ),         // pps脉冲输出，50%占空比，用于检测内部时钟稳定性
				
	   // FPGA内部端口
		.pre_year   (slv_reg0[17:10]),  // 预置日期值,BCD码
        .pre_month  (slv_reg1[17:13]),  // BCD码 
        .pre_day    (slv_reg1[12:7] ),  // BCD码 
        .pre_week   (slv_reg1[6:0]  ),  // 独热码
        .pre_yearday(slv_reg0[9:0]  ),  // BCD码 
 		.pre_hour   (slv_reg2[19:14]),  // 预置时间值，BCD码
        .pre_minute (slv_reg2[13:7] ),  // BCD码 
        .pre_second (slv_reg2[6:0]  ),  // BCD码 
		.wr_date     (slv_reg3[0]   ),  // CPU脉冲，预置与日期相关的数据   
        .wr_time     (slv_reg4[0]   ),  // CPU脉冲，预置与时间相关的数据             
        .bm_encode_en(slv_reg5[0]   ),  // B码编码输出使能信号
        .bm_decode_en(slv_reg6[0]   ),  // 启动B码校时，校时成功后自动关闭硬件
        .pps_clr_en  (slv_reg7[0]   ),  // 亚秒计数器清零使能信号 
		                                
	   // 当前BM解码的值
		.bm_year   (bm_year   ),    // 当前日期值,BCD码
        .bm_yearday(bm_yearday),    // BCD码 
 		.bm_hour   (bm_hour   ),    // 当前时间值，BCD码
        .bm_minute (bm_minute ),    // BCD码 
        .bm_second (bm_second ),    // BCD码
		.bm_daysec (bm_daysec ),    // 日秒，二进制 	   
	   
       //当前软时钟的值
		.curr_year   (curr_year    ),   // 当前日期值,BCD码
        .curr_month  (curr_month   ),   // BCD码 
        .curr_day    (curr_day     ),   // BCD码 
        .curr_week   (curr_week    ),   // 独热码
        .curr_yearday(curr_yearday ),   // BCD码 
 		.curr_hour   (curr_hour    ),   // 当前时间值，BCD码
        .curr_minute (curr_minute  ),   // BCD码 
        .curr_second (curr_second  ),   // BCD码
		.curr_daysec (curr_daysec  ),   // 日秒，二进制 
        .curr_subsec (curr_subsec  ),   // 亚秒，二进制	
		
       // CPU中断请求信号
	    .pps_gps2cpu(pps_gps2cpu),     // 3个pps信号，用于软件精确对时
	    .pps_in2cpu (pps_in2cpu ),
        .pps_bm2cpu (pps_bm2cpu ),
	    .bm_syn_end (bm_syn_end ),     // B码解码成功，并完成软时钟的同步	
		.date_update(date_update)      // 00:00:00到了，请求CPU更行日期数据
	); 
	
    onoff_io onoff_inst(
          //开关量配置       
		  //FPGA interface
           .clkin   (CLK25MHz       ),       //25MHz  main clock
           .rst_n   (CLK25MHz_RSTN  ),       // low actively
           .start   (slv_reg8[24]   ),       //start one write&read or reset last status
           .byte_num(slv_reg8[18:16]),       //1,2,3,4 refer to 8,16,24,32 of the io bits respectively
           .rw_modes(slv_reg8[31:30]),
           .din     (slv_reg13      ),       //datas writted to TPIC6B595
           .dout    (onoff_dout     ),       //datas read from 74HC165   
           .done    (onoff_done     ),       //feedback to CPU 
		   .latch_time(onoff_latch  ),
           //board interface      
           .cs   (onoff_cs  ),		   
           .sclk (onoff_sclk),         
           .sdo  (onoff_sdo ),
           .sdi  (onoff_sdi )            
         );     
		 
    config2ser16bits wrserial_inst(
         //二级DAC及功放配置?
         //FPGA interface
          .clkin    (CLK25MHz     ),           //25MHz  main clock
          .rst_n    (CLK25MHz_RSTN),
          .start    (slv_reg14[8] ),             //start one congfiguration
          .ld1595_en(slv_reg14[0] ),
          .ld595_en (slv_reg14[1] ),
          .din0     (slv_reg9     ),        //ub + ua  
          .din1     (slv_reg10    ),        //ux + uc  
          .din2     (slv_reg11    ),        //ib + ia  
          .din3     (slv_reg12    ),        //ix + ic  
          .config_done(wrserial_done),      //feedback to CPU 
          //ADDA board interface
          .load(wrserial_load),           //LD_W            
          .sclk(wrserial_sclk),           //CLK_W 
          .sdo (wrserial_sdo )            //DATA_W
          );		 
	
   rd_from_serial8bit  rd_serial_inst(
          //8路功放信号失效反馈
	      //FPGA interface
          .clkin      (CLK25MHz     ),         //25MHz  main clock
          .rst_n      (CLK25MHz_RSTN),
          .enable     (slv_reg15[0] ),
          .dataout    (data_rdserial),         //
	      //ADDA board interface
          .load_n     (rdserial_load),         //LD_R
          .sclk       (rdserial_sclk),         //CLK_R
          .sdi        (rdserial_sdi )	         //DATA_R
          );	
	
     //////////////////////////////////////////////////////////////////
	   reg   [1:0]  latch_dly;
       wire         latch_posedge;
	   
 	  always @( posedge S_AXI_ACLK )
	     begin
	       if ( S_AXI_ARESETN == 1'b0 )
	          latch_dly <= 2'd0; 
	       else      
		      latch_dly <= {latch_dly[0],onoff_latch};		  
          end
       assign latch_posedge = ~latch_dly[1] && latch_dly[0];
	
	//
	  always @( posedge S_AXI_ACLK )
	  begin
	    if ( S_AXI_ARESETN == 1'b0 )
	      begin
             latch_hour   <= 6'd0;  
             latch_minute <= 7'd0;
             latch_second <= 7'd0;
             latch_daysec <= 17'd0;	      
             latch_subsec <= 24'd0;		  		  
	      end 
	    else if( latch_posedge )
	      begin 	
             latch_hour   <= curr_hour  ; 
             latch_minute <= curr_minute; 
             latch_second <= curr_second; 
             latch_daysec <= curr_daysec;	
             latch_subsec <= curr_subsec;	
          end
      end  
	// User logic ends
   
	endmodule

