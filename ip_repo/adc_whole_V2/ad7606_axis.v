`timescale 1 ns / 1 ps

module ad7606_axis#
(
    // Users to add parameters here
    parameter integer WIDTH_OF_NUMBER_OF_OUTPUT_WORDS = 22
    // User parameters ends
)
(
    //FPGA interface
    input       bulk_start,      // 定义：高电平=使能连续采样，低电平=停止 为了支持录波
    output      bulk_end,        // 块传输完成信号（产生中断）
    input  [WIDTH_OF_NUMBER_OF_OUTPUT_WORDS-1 : 0] sample_points,  // Block Size (e.g. 1024*16)
    input  [15:0] sample_freq,   // Sample Frequency Divider

    //ADDC interface
    output yad_rst,
    output yad_cvn,
    output yad_cs,
    output yad_ck,
    input yad_sa,
    input yad_sb,

    // Global ports
    input wire  M_AXIS_ACLK, //100MHz
    input wire  M_AXIS_ARESETN,

    //AXIS interface
    output wire  M_AXIS_TVALID,
    output wire [127 : 0] M_AXIS_TDATA,
    output wire [15 : 0] M_AXIS_TSTRB,
    output wire  M_AXIS_TLAST,
    input wire  M_AXIS_TREADY
);

    // conv_start high state continued 10' 100MHz clock cycles
    localparam integer C_M_START_COUNT = 10;

    // Define the states of state machine
    localparam [6:0] IDLE       = 7'b0000001,
                     WAIT_BEAT  = 7'b0000010,
                     START_HIGH = 7'b0000100,
                     ONE_ADC    = 7'b0001000,
                     IN_ADC     = 7'b0010000,
                     SEND_STREAM= 7'b0100000,
                     BULK_END   = 7'b1000000;

    // State variable
    reg [6:0] mst_exec_state;
    // FIFO read pointer
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

    // I/O Connections assignments
    assign M_AXIS_TVALID = axis_tvalid_delay;
    assign M_AXIS_TDATA  = stream_data_out;
    assign M_AXIS_TLAST  = axis_tlast_delay;
    assign M_AXIS_TSTRB  = {16{1'b1}};
    assign bulk_end      = tx_done; // 每一块数据传完，这里会产生一个脉冲

    // 【修改点1】移除边沿检测逻辑
    // 我们不再需要捕捉 bulk_start 的上升沿，而是直接检测它的电平状态
    /*
    wire     bulk_start_flag;
    reg      start_d0;
    reg      start_d1;
    always @(posedge M_AXIS_ACLK) ...
    assign   bulk_start_flag = (~start_d1) & start_d0;
    */

    // Control state machine implementation
    always @(posedge M_AXIS_ACLK)
    begin
      if (!M_AXIS_ARESETN)
        begin
          mst_exec_state <= IDLE;
          count          <= 0;
          conv_start     <= 1'b0;
          clk_en         <= 0;
        end
      else
        case (mst_exec_state)
          IDLE:
            // 【修改点2】电平触发启动
            // 只要 bulk_start 为高（软件写1），就进入工作状态
            if ( bulk_start )
              begin
                mst_exec_state  <= WAIT_BEAT;
                clk_en   <= 1'b1; // 启动采样时钟计数器
              end
            else
              begin
                mst_exec_state  <= IDLE;
                count    <= 0;
                clk_en   <= 1'b0; // 停止采样时钟
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
                // 【修改点3】在等待期间，若软件拉低 bulk_start，则在本周期结束后停止
                // 这里为了逻辑简单，只在 BULK_END 检查停止条件，
                // 这样能保证至少传输完当前这个点，不会传输一半断掉。
                mst_exec_state  <= WAIT_BEAT;
                conv_start      <= 1'b0;
              end

          START_HIGH:
            if ( count == C_M_START_COUNT - 1 )
              begin
                mst_exec_state  <= ONE_ADC;
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
            // 当前 Block 传输完成
            if (read_pointer == sample_points)
              begin
                // 【修改点4】循环逻辑
                // 如果 bulk_start 依然为高，说明要继续录波，跳回 WAIT_BEAT
                // 此时 clk_en 保持为 1，采样计数器 continuous 工作，无缝衔接
                if (bulk_start)
                    mst_exec_state <= WAIT_BEAT;
                else
                    mst_exec_state <= IDLE; // 只有软件写 0 停止时，才回 IDLE
              end
            else
              begin
                mst_exec_state  <= WAIT_BEAT;
              end

          default :
                mst_exec_state  <= IDLE;

        endcase
    end


    // tvalid generation
    assign axis_tvalid = (mst_exec_state == SEND_STREAM);

    // AXI tlast generation
    // TLAST 信号非常重要，它告诉 DMA 这个 Packet 结束了，DMA 应该产生中断并切换 Buffer
    assign axis_tlast = ((mst_exec_state == SEND_STREAM) && (read_pointer == sample_points-1));


    // Delay signals
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


    // read_pointer and tx_done generation
    always@(posedge M_AXIS_ACLK)
    begin
      if(!M_AXIS_ARESETN)
        begin
          read_pointer <= 0;
          tx_done <= 1'b0;
        end
      else
        if (read_pointer < sample_points)
          begin
            if (tx_en)
              begin
                read_pointer <= read_pointer + 1;
                tx_done <= 1'b0;
              end
          end
        else if (read_pointer == sample_points)
          begin
            // 当一个 Block 传完，复位指针，准备下一次循环
            // tx_done 脉冲会触发一次中断，通知 CPU 处理 Ping/Pong Buffer
            read_pointer <= 0;
            tx_done <= 1'b1;
          end
    end


    // FIFO read enable generation
    assign tx_en = M_AXIS_TREADY && axis_tvalid;

    // Streaming output data
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

   // 12.5MHz Clock Gen
     reg [2:0]           count12_5;
     wire                 clk12_5mhz;

     always@(posedge M_AXIS_ACLK)
      if(!M_AXIS_ARESETN)
            count12_5   <= 3'b000;
        else
            count12_5 <= count12_5 + 1;

      assign clk12_5mhz = count12_5[2];

   // Sample Clock Gen (Continuous)
     reg [15:0]          count50k;
     reg                 clk50khz;

     always@(posedge M_AXIS_ACLK)
      if(!M_AXIS_ARESETN)
            begin
            count50k   <= 16'b00;
            clk50khz   <= 1'b0;
            end
        else if(clk_en) // clk_en 在 IDLE 状态下为 0，一旦 Start 后始终为 1
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

    // Instantiation
 ad7606_ser2par128bit ad7606_inst
    (
    //FPGA interface
    .clkin     (clk12_5mhz    ),
    .rst_n     (M_AXIS_ARESETN),
    .conv_start(conv_start    ),
    .rd_en     (rd_enable     ),
    .dout      (dout          ),

    //ADDC interface
    .yad_rst (yad_rst   ),
    .yad_cvn (yad_cvn   ),
    .yad_cs  (yad_cs    ),
    .yad_ck  (yad_ck    ),
    .yad_sa  (yad_sa    ),
    .yad_sb  (yad_sb    )

    );

endmodule