`timescale 1 ns / 1 ps

	module ad7606_axis#
	(
		// Users to add parameters here
        parameter integer WIDTH_OF_NUMBER_OF_OUTPUT_WORDS = 22
		// User parameters ends
	)
	(
	    //FPGA interface
        input       bulk_start,
        output      bulk_end, 
        input  [WIDTH_OF_NUMBER_OF_OUTPUT_WORDS-1 : 0] sample_points,  //number of sample points,maxium number is 60s*50K
                                      //1024 for one 50Hz signal cycle
        input  [15:0] sample_freq,    //number divived of sample freqency
		                              //1953 for 50*1024Hz of sample freqency	
		
	    //ADDC interface
        output [1:0] yos,
        output yad_rst,
        output yad_cvn,
        output yad_cs,
        output yad_ck,
        input yad_sa,
        input yad_sb,
        input yad_busy,

		// Global ports
		input wire  M_AXIS_ACLK, //100MHz
		input wire  M_AXIS_ARESETN,
		
		//AXIS interface
		// Master Stream Ports. TVALID indicates that the master is driving a valid transfer, A transfer takes place when both TVALID and TREADY are asserted. 
		output wire  M_AXIS_TVALID,
		// TDATA is the primary payload that is used to provide the data that is passing across the interface from the master.
		output wire [127 : 0] M_AXIS_TDATA,
		// TSTRB is the byte qualifier that indicates whether the content of the associated byte of TDATA is processed as a data byte or a position byte.
		output wire [15 : 0] M_AXIS_TSTRB,
		// TLAST indicates the boundary of a packet.
		output wire  M_AXIS_TLAST,
		// TREADY indicates that the slave can accept a transfer in the current cycle.
		input wire  M_AXIS_TREADY
	);
	// conv_start' high state is 6 clock cycles = one 16.7MHz clock cycle.
		localparam integer C_M_START_COUNT	= 6;   

	// Define the states of state machine                                                                                    
	localparam [6:0] IDLE       = 7'b0000001,      // This is the initial/idle state               	                                                                                     
	                 WAIT_BEAT  = 7'b0000010,      //  wait for a sample clock pulse      
	                 START_HIGH = 7'b0000100,      //  
	                 ONE_ADC    = 7'b0001000,
	                 IN_ADC     = 7'b0010000,	                                    
	                 SEND_STREAM= 7'b0100000, // In this state the                       
	                                     // stream data is output through M_AXIS_TDATA   
	                 BULK_END   = 7'b1000000;                    
	// State variable                                                                    
	reg [6:0] mst_exec_state;                                                            
	// Example design FIFO read pointer                                                  
	reg [WIDTH_OF_NUMBER_OF_OUTPUT_WORDS-1 : 0] read_pointer;                                                      

	// AXI Stream internal signals  wait counter.
	// The master waits for the user defined number of clock cycles before initiating a transfer.
	reg [2 : 0] 	count;
	//streaming data valid
	wire  	axis_tvalid;
	//streaming data valid delayed by one clock cycle
	reg  	axis_tvalid_delay;
	//Last of the streaming data 
	wire  	axis_tlast;
	//Last of the streaming data delayed by one clock cycle
	reg  	axis_tlast_delay;
	//FIFO implementation signals
	reg [127 : 0] 	stream_data_out;
	wire  	tx_en;
	//The master has issued all the streaming data stored in FIFO
	reg  	tx_done;
    //user defined
	wire [127:0]  dout;    //从高位到低位分别是第1通道~第8通道                  
	reg           conv_start;                                     
	wire          rd_enable;                                                                         
    wire          adc_point;
    reg           clk_en;	
       
	// I/O Connections assignments

	assign M_AXIS_TVALID = axis_tvalid_delay;
	assign M_AXIS_TDATA	= stream_data_out;
	assign M_AXIS_TLAST	= axis_tlast_delay;
	assign M_AXIS_TSTRB	= {16{1'b1}};
	assign bulk_end  = tx_done;
//
    //捕捉边沿，生成启动转换脉冲，以防止启动一次，采样多次的情况发生。	
       wire     bulk_start_flag;
       reg      start_d0;
	   reg      start_d1;	   
	always @(posedge M_AXIS_ACLK)                                             
	begin                                                                     
	  if (!M_AXIS_ARESETN)                                                                                         
	    begin                                                                 
	      start_d0 <= 1'b0;                                             	
          start_d1 <= 1'b0;		  
	    end                                                                   
	  else 	   
 	    begin                                                                 
	      start_d0 <= bulk_start;                                             	
          start_d1 <= start_d0;		  
	    end              
	end    
	assign   bulk_start_flag = (~start_d1) & start_d0;	
	
	// Control state machine implementation                             
	always @(posedge M_AXIS_ACLK)                                             
	begin                                                                     
	  if (!M_AXIS_ARESETN)                                                    
	  // Synchronous reset (active low)                                       
	    begin                                                                 
	      mst_exec_state <= IDLE;                                             
	      count          <= 0; 
          conv_start     <= 1'b0; 
          clk_en         <= 0; 		  
	    end                                                                   
	  else                                                                    
	    case (mst_exec_state) 		
	      IDLE:                                                                                            
	        if ( bulk_start_flag )                                                 
	          begin                                                           
	            mst_exec_state  <= WAIT_BEAT;
                clk_en   <= 1'b1; 				
	          end                                                             
	        else                                                              
	          begin                                                           
	            mst_exec_state  <= IDLE; 
	            count    <= 0; 
                clk_en   <= 1'b0; 
                conv_start     <= 1'b0; 				
	          end                                                             
	                                                                          
	      WAIT_BEAT: 
	        if (adc_point) 		  
	          begin                                                           
	            mst_exec_state  <= START_HIGH; 
                conv_start <= 1'b1;				                 				
	          end                                                             
	        else                                                              
	          begin                                                                                                    
	            mst_exec_state  <= WAIT_BEAT; 
                conv_start      <= 1'b0;	                                         
	          end                                                             

	      START_HIGH: 
          //delay  to sure conv_start valid		  
	        if ( count == C_M_START_COUNT - 1 )                               
	          begin                                                           
	            mst_exec_state  <=ONE_ADC;  
	            count    <= 0;                  				
	          end                                                             
	        else                                                              
	          begin                                                           
	            count <= count + 1;                                           
	            mst_exec_state  <= START_HIGH; 	                                         
	          end                     

	                
          ONE_ADC :
	        if (rd_enable == 1'b0)                                                 
	          begin                                                           
	            mst_exec_state  <= IN_ADC; 
                conv_start      <= 1'b0;				
	          end                                                             
	        else                                                              
	          begin                                                           
	            mst_exec_state  <= ONE_ADC;				
	          end                                   		  

          IN_ADC :
	        if (rd_enable == 1'b1)                                                 
	          begin                                                           
	            mst_exec_state  <= SEND_STREAM;  	                                         
	          end                                                             
	        else                                                              
	          begin                                                           
	            mst_exec_state  <= IN_ADC;                                      
	          end                                   		  
			  
	      SEND_STREAM:                                                                                
	        if (tx_en)                                                      
	          begin                                                           
	            mst_exec_state <= BULK_END;  				
	          end                                                             
	        else                                                              
	          begin                                                           
	            mst_exec_state <= SEND_STREAM;                                
	          end 
    
	      BULK_END :
	        if (read_pointer == sample_points)                                                 
	          begin                                                           
	            mst_exec_state  <= IDLE; 
	          end                                                             
	        else                                                              
	          begin                                                           
	            mst_exec_state  <= WAIT_BEAT;                 				
	          end                   		  

		  default :                                                                                                                                           
	            mst_exec_state  <= IDLE;  
				
	    endcase                                                               
	end                                                                       


	//tvalid generation
	//axis_tvalid is asserted when the control state machine's state is SEND_STREAM and
	//number of output streaming data is less than the NUMBER_OF_OUTPUT_WORDS.
	assign axis_tvalid = (mst_exec_state == SEND_STREAM); // && (read_pointer < sample_points));
	                                                                                               
	// AXI tlast generation                                                                        
	// axis_tlast is asserted number of output streaming data is NUMBER_OF_OUTPUT_WORDS-1          
	// (0 to NUMBER_OF_OUTPUT_WORDS-1)                                                             
	assign axis_tlast = ((mst_exec_state == SEND_STREAM) && (read_pointer == sample_points-1));                               
	                                                                                               
	                                                                                               
	// Delay the axis_tvalid and axis_tlast signal by one clock cycle                              
	// to match the latency of M_AXIS_TDATA                                                        
	always @(posedge M_AXIS_ACLK)                                                                  
	begin                                                                                          
	  if (!M_AXIS_ARESETN)                                                                         
	    begin                                                                                      
	      axis_tvalid_delay <= 1'b0;                                                               
	      axis_tlast_delay <= 1'b0;                                                                
	    end                                                                                        
	  else                                                                                         
	    begin                                                                                      
	      axis_tvalid_delay <= axis_tvalid;                                                        
	      axis_tlast_delay  <= axis_tlast;                                                          
	    end                                                                                        
	end                                                                                            


	//read_pointer pointer

	always@(posedge M_AXIS_ACLK)                                               
	begin                                                                            
	  if(!M_AXIS_ARESETN)                                                            
	    begin                                                                        
	      read_pointer <= 0;                                                         
	      tx_done <= 1'b0;                                                           
	    end                                                                          
	  else                                                                           
	    if (read_pointer <sample_points)                                
	      begin                                                                      
	        if (tx_en)                                                                                                 
	          begin                                                                  
	            read_pointer <= read_pointer + 1;                                    
	            tx_done <= 1'b0;                                                     
	          end                                                                    
	      end                                                                        
	    else if (read_pointer == sample_points)                             
	      begin                                                                      
	        // tx_done is asserted when NUMBER_OF_OUTPUT_WORDS numbers of streaming data
	        // has been out. 
	        read_pointer <= 0;  	                                                                
	        tx_done <= 1'b1;                                                         
	      end                                                                        
	end                                                                              


	//FIFO read enable generation 

	assign tx_en = M_AXIS_TREADY && axis_tvalid;   
	                                                     
	    // Streaming output data is read from FIFO       
	    always @( posedge M_AXIS_ACLK )                  
	    begin                                            
	      if(!M_AXIS_ARESETN)                            
	        begin                                        
	          stream_data_out <= 1;                      
	        end                                          
	      else if (tx_en)
	        begin                                        
	          stream_data_out <= dout;   
	        end                                          
	    end                                              

   //generate a clock of 16.7MHz
     reg [1:0]           count17m;
     reg                 clk17mhz; 
 
     always@(posedge M_AXIS_ACLK)                                            
	  if(!M_AXIS_ARESETN)                            
	        begin
            count17m   <= 2'b00;
            clk17mhz   <= 1'b0; 
            end
        else
		   begin		
		     if(count17m == 2'b10)begin
                           count17m    <= 2'b00 ;
                           clk17mhz <= ~clk17mhz;    
						   end
                      else begin
                           count17m <= count17m + 1;
                                end 
            end
   //generate a sample clock who's freqency is about 50x1024Hz
     reg [15:0]          count50k;
     reg                 clk50khz; 
 
     always@(posedge M_AXIS_ACLK)                                            
	  if(!M_AXIS_ARESETN)                            
	        begin
            count50k   <= 16'b00;
            clk50khz   <= 1'b0; 
            end
        else if(clk_en)
		   begin		
		     if(count50k == sample_freq-1)begin
                           count50k  <= 16'b00 ;
                           clk50khz  <= 1'b1;    
						   end
                      else begin
                           count50k <= count50k + 1;
                           clk50khz   <= 1'b0; 	
                                end 
            end
	  assign adc_point = clk50khz;
	  
    //
 ad7606_ser2par128bit ad7606_inst
    (
    //FPGA侧	
    .clkin     (clk17mhz  ),           //main clock,16.7MHz
    .rst_n     (M_AXIS_ARESETN  ),
    .conv_start(conv_start  ),
    .rd_en     (rd_enable  ),
    .dout      (dout  ),     //从高位到低位分别是第1通道~第8通道
	//ADDC侧
    .yos     (yos       ),
    .yad_rst (yad_rst   ),
    .yad_cvn (yad_cvn   ),
    .yad_cs  (yad_cs    ),
    .yad_ck  (yad_ck    ),
    .yad_sa  (yad_sa    ),
    .yad_sb  (yad_sb    ),
    .yad_busy(yad_busy  )
    );


	endmodule
