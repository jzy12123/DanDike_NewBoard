`timescale 1 ns / 1 ps

module ad7606_axis#
(
    parameter integer WIDTH_OF_NUMBER_OF_OUTPUT_WORDS = 22
)
(
    input       bulk_start,      
    output      bulk_end,        
    input  [WIDTH_OF_NUMBER_OF_OUTPUT_WORDS-1 : 0] sample_points,  
    input  [15:0] sample_freq,   
    output yad_rst,
    output yad_cvn,
    output yad_cs,
    output yad_ck,
    input yad_sa,
    input yad_sb,
    input wire  M_AXIS_ACLK, 
    input wire  M_AXIS_ARESETN,
    output wire  M_AXIS_TVALID,
    output wire [127 : 0] M_AXIS_TDATA,
    output wire [15 : 0] M_AXIS_TSTRB,
    output wire  M_AXIS_TLAST,
    input wire  M_AXIS_TREADY
);

    localparam integer C_M_START_COUNT = 10;
    localparam [6:0] IDLE       = 7'b0000001,
                     WAIT_BEAT  = 7'b0000010,
                     START_HIGH = 7'b0000100,
                     ONE_ADC    = 7'b0001000,
                     IN_ADC     = 7'b0010000,
                     SEND_STREAM= 7'b0100000,
                     BULK_END   = 7'b1000000;
                     
    reg [6:0] mst_exec_state;
    reg [WIDTH_OF_NUMBER_OF_OUTPUT_WORDS-1 : 0] read_pointer;
    reg [3 : 0] count;
    wire axis_tvalid;
    reg axis_tvalid_delay;
    wire axis_tlast;
    reg axis_tlast_delay;
    reg [127 : 0] stream_data_out;
    wire tx_en;
    reg tx_done;
    wire [127:0] dout;
    reg conv_start;
    wire rd_enable;
    wire adc_point;
    reg clk_en;

    assign M_AXIS_TVALID = axis_tvalid_delay;
    assign M_AXIS_TDATA  = stream_data_out;
    assign M_AXIS_TLAST  = axis_tlast_delay;
    assign M_AXIS_TSTRB  = {16{1'b1}};
    assign bulk_end      = tx_done;

    // 状态机
    always @(posedge M_AXIS_ACLK)
    begin
      if (!M_AXIS_ARESETN) begin
          mst_exec_state <= IDLE;
          count          <= 0;
          conv_start     <= 1'b0;
          clk_en         <= 0;
      end else case (mst_exec_state)
          IDLE:
            if ( bulk_start ) begin
                mst_exec_state  <= WAIT_BEAT;
                clk_en   <= 1'b1;
            end else begin
                mst_exec_state  <= IDLE;
                count    <= 0;
                clk_en   <= 1'b0;
                conv_start     <= 1'b0;
            end
          WAIT_BEAT:
            if (adc_point) begin
                mst_exec_state  <= START_HIGH;
                conv_start <= 1'b1;
            end else begin
                mst_exec_state  <= WAIT_BEAT;
                conv_start      <= 1'b0;
            end
          START_HIGH:
            if ( count == C_M_START_COUNT - 1 ) begin
                mst_exec_state  <= ONE_ADC;
                count    <= 0;
            end else begin
                count <= count + 1;
                mst_exec_state  <= START_HIGH;
            end
          ONE_ADC :
            if (rd_enable == 1'b0) begin
                mst_exec_state  <= IN_ADC;
                conv_start      <= 1'b0;
            end else begin
                mst_exec_state  <= ONE_ADC;
            end
          IN_ADC :
            if (rd_enable == 1'b1) mst_exec_state  <= SEND_STREAM;
            else mst_exec_state  <= IN_ADC;
          SEND_STREAM:
            if (tx_en) mst_exec_state <= BULK_END;
            else mst_exec_state <= SEND_STREAM;
          BULK_END :
            if (read_pointer == sample_points) begin
                if (bulk_start) mst_exec_state <= WAIT_BEAT;
                else mst_exec_state <= IDLE;
            end else begin
                mst_exec_state  <= WAIT_BEAT;
            end
          default : mst_exec_state  <= IDLE;
        endcase
    end

    assign axis_tvalid = (mst_exec_state == SEND_STREAM);
    assign axis_tlast = ((mst_exec_state == SEND_STREAM) && (read_pointer == sample_points-1));

    always @(posedge M_AXIS_ACLK)
    begin
      if (!M_AXIS_ARESETN) begin
          axis_tvalid_delay <= 1'b0;
          axis_tlast_delay <= 1'b0;
      end else begin
          axis_tvalid_delay <= axis_tvalid;
          axis_tlast_delay  <= axis_tlast;
      end
    end

    // 【重要修改】read_pointer 复位逻辑
    always@(posedge M_AXIS_ACLK)
    begin
      // 必须加上 IDLE 状态判断，确保重启时从 0 开始
      if(!M_AXIS_ARESETN || (mst_exec_state == IDLE))
        begin
          read_pointer <= 0;
          tx_done <= 1'b0;
        end
      else
        if (read_pointer < sample_points)
          begin
            if (tx_en) begin
                read_pointer <= read_pointer + 1;
                tx_done <= 1'b0;
            end
          end
        else if (read_pointer == sample_points)
          begin
            read_pointer <= 0;
            tx_done <= 1'b1;
          end
    end

    assign tx_en = M_AXIS_TREADY && axis_tvalid;

    always @( posedge M_AXIS_ACLK )
    begin
      if(!M_AXIS_ARESETN) stream_data_out <= 0;
      else if (tx_en) stream_data_out <= dout;
    end

   // 12.5MHz Clock Gen
     reg [2:0] count12_5;
     wire      clk12_5mhz;
     always@(posedge M_AXIS_ACLK)
      if(!M_AXIS_ARESETN) count12_5 <= 3'b000;
      else count12_5 <= count12_5 + 1;
     assign clk12_5mhz = count12_5[2];

   // Sample Clock Gen
     reg [15:0] count50k;
     reg        clk50khz;
     always@(posedge M_AXIS_ACLK)
      if(!M_AXIS_ARESETN) begin
          count50k <= 16'b00;
          clk50khz <= 1'b0;
      end else if(clk_en) begin
          if(count50k == sample_freq-1) begin
              count50k  <= 16'b00 ;
              clk50khz  <= 1'b1;
          end else begin
              count50k <= count50k + 1;
              clk50khz <= 1'b0;
          end
      end
      assign adc_point = clk50khz;

    // Instantiation
    ad7606_ser2par128bit ad7606_inst
    (
        .clkin     (clk12_5mhz    ),
        .rst_n     (M_AXIS_ARESETN),
        .conv_start(conv_start    ),
        .rd_en     (rd_enable     ),
        .dout      (dout          ),
        .yad_rst (yad_rst   ),
        .yad_cvn (yad_cvn   ),
        .yad_cs  (yad_cs    ),
        .yad_ck  (yad_ck    ),
        .yad_sa  (yad_sa    ),
        .yad_sb  (yad_sb    )
    );
endmodule