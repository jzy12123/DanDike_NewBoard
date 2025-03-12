
`timescale 1 ns / 1 ps
//-------------------------------------------------------------------------------------//
//
//  1. axis_data_FIFO is configured to no TSTRB and no TLAST. data width is 128bits.  //
//
//-------------------------------------------------------------------------------------//

	module dma_axis 
	(
		// Users to add ports here
		
		//FPGA control interface
		input wire        enable,          // The DMA_DAC works when enable high,stops when enable low
		input wire [15:0] freq_div,         // 1953,data sending speed to DAC chip 
		//
		output reg [127:0] stream_data_dma, //
		output 	reg        start_dma_dac,   // 
		
		//AXIS interface
		// AXI4Stream sink: Clock
		input wire  S_AXIS_ACLK,       //100MHz
		// AXI4Stream sink: Reset
		input wire  S_AXIS_ARESETN,    //active low
		
		// Ready to accept data in
		output wire  S_AXIS_TREADY,
		// Data in
		input wire [127 : 0] S_AXIS_TDATA,
		// Data is in valid
		input wire  S_AXIS_TVALID
	);

    //使能信号enable有效时，根据freq_div的值，生成周期性的FIFO读信号fifo_rden 
	reg [15:0]  frq_counter;
	reg         fifo_rden;
	
	always @(posedge S_AXIS_ACLK)	               
	    begin                                            
	      if(!S_AXIS_ARESETN)                            
	         begin                     
              frq_counter <= 16'd0;
              fifo_rden   <= 1'b0;
             end
           else if(enable) begin
                       if(frq_counter >= (freq_div-1)) begin
                             frq_counter <= 16'd0;
                             fifo_rden   <= 1'b1;
                             end
                             else begin
                                  frq_counter <= frq_counter + 1;
                                  fifo_rden   <= 1'b0;
                                  end
							end	
                  else begin
                        frq_counter <= 16'd0;
                        fifo_rden   <= 1'b0;
                        end
             end
	
	//fifo_rden有效时TREADY，然后从FIFO读取数据tream_data_dac同时生成fifo_rddone信号
	// Define the states of state machine
	// The control state machine oversees the writing of input streaming data to the DAC,
	localparam [1:0] IDLE       = 2'b00,        // This is the initial/idle state 
                     SHAKE_HAND = 2'b01,
	                 RD_FIFO    = 2'b10;        // In this state DAC is written with the
	                                           // input stream data S_AXIS_TDATA 
	
	wire  	axis_tready;
	wire    dac_wren;
	// State variable
	reg [1:0]   mst_exec_state;  
	reg         fifo_rddone;
	
    //
	assign S_AXIS_TREADY	= axis_tready;
	// Control state machine implementation
	always @(posedge S_AXIS_ACLK) 
	begin  
	  if (!S_AXIS_ARESETN) 
	    begin
	      mst_exec_state <= IDLE;
	    end  
	  else 
	    case (mst_exec_state)
	      IDLE : 
            if(fifo_rden) begin  
                    mst_exec_state <= SHAKE_HAND;
	            end
          SHAKE_HAND :
		  if (S_AXIS_TVALID)
	            begin
	              mst_exec_state <= RD_FIFO;
	            end
	          else
	            begin
	              mst_exec_state <= SHAKE_HAND;
	            end
	      RD_FIFO : 
	          begin
	            mst_exec_state <= IDLE;
	          end
	    endcase
	end
	
	// AXI Streaming Sink 
	assign axis_tready = (mst_exec_state == RD_FIFO);

	// DAC write enable generation
	assign dac_wren = S_AXIS_TVALID && axis_tready;
    reg    dac_wren_dly;
 	always @(posedge S_AXIS_ACLK) 
	begin  
	  if (!S_AXIS_ARESETN) 
              dac_wren_dly <= 0;
	     else    
             dac_wren_dly <= dac_wren;	     
	end      

	    // Streaming input data is stored in DAC
	    always @( posedge S_AXIS_ACLK )
	    begin
	      if (!S_AXIS_ARESETN) 
	        begin
	            stream_data_dma <= 128'd0;
			    fifo_rddone     <= 1'b0;
	        end  
		    else if (dac_wren_dly)// 
	            begin
	              stream_data_dma <= S_AXIS_TDATA;
		    	  fifo_rddone     <= 1'b1;
	            end  
		    	else
	              begin
	                stream_data_dma <= stream_data_dma;
		    	    fifo_rddone     <= 1'b0;
	              end  			
	    end  	
		
    //当fifo_rddone有效时，生成5个clock周期宽的start_dma_dac脉冲信号

	localparam     IDLE1 = 1'b0,  
	               WORK1 = 1'b1; 

	// State variable
	reg       state1;  
    reg [2:0] counter1;

	
	always @(posedge S_AXIS_ACLK) 
	begin  
	  if (!S_AXIS_ARESETN) 
	    begin
	      state1    <= IDLE1;
		  counter1  <= 3'd0;
		  start_dma_dac <= 1'b0;
	    end  
	  else
	    case (state1)
	      IDLE1: 
		          if (fifo_rddone)  
		     	      state1 <= WORK1;
	              else 
				  	  state1 <= IDLE1;
          
		  WORK1:  
		         if ( counter1 >= 3'd5 )          // 5 clock pulse             
	                 begin
	                   state1    <= IDLE1;
		               counter1  <= 3'd0;
		               start_dma_dac <= 1'b0;
	                 end                                          
	             else                                                              
 	                 begin
	                   state1    <= WORK1;
		               counter1  <= counter1 + 1;
		               start_dma_dac <= 1'b1;
	                 end                                                   
	    endcase
	end


	endmodule
