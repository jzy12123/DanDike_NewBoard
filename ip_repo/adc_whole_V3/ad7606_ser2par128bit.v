`timescale 1ns / 10ps

module ad7606_ser2par128bit
(
    input           clkin,           
    input           rst_n,
    input           conv_start,      
    output  reg     rd_en, 
    output [127:0]  dout,            
    output     yad_rst,
    output reg yad_cvn,
    output reg yad_cs,
    output     yad_ck,
    input      yad_sa,
    input      yad_sb
);
   
    parameter  idle  = 2'd0;
    parameter  s1    = 2'd1;
    parameter  s2    = 2'd2;
    parameter  s3    = 2'd3;

    reg    [1:0]  state;
    reg    [5:0]  count2;
    reg           count2_end;
    reg    [5:0]  count1;
    reg           count1_end;
    reg    [63:0] data_a, data_b;  
	
    // 【修改】同步 conv_start
    reg conv_start_d0, conv_start_d1;
    wire conv_start_sync = conv_start_d1;

    always @(posedge clkin or negedge rst_n) begin
        if (!rst_n) begin
            conv_start_d0 <= 1'b0;
            conv_start_d1 <= 1'b0;
        end else begin
            conv_start_d0 <= conv_start;
            conv_start_d1 <= conv_start_d0;
        end
    end

    always@(posedge clkin or negedge rst_n )
    if(!rst_n) begin
        state    <= idle;
        yad_cs   <= 1'b1;
        yad_cvn  <= 1'b0;
        rd_en    <= 0;
    end
    else
    case(state)
        idle :  if(conv_start_sync)    begin 
                    state <= s1;
                    yad_cvn <= 1; rd_en<=0; 
                end
        s1 :    if (count2_end)   begin
                    state <= s2;
                    yad_cvn <= 0;  
                end  
        s2 :	 begin state <= s3; yad_cs <= 0; end 	 
        s3 :    if(count1_end)     begin 
                    state <= idle;
                    yad_cs <= 1; rd_en <=1; 
                end
    endcase

    always@(posedge clkin or negedge rst_n)
    if(!rst_n) begin
        count2      <= 6'd0;
        count2_end  <= 1'b0; 
    end
    else if(yad_cvn) begin
        if (count2==6'd51)begin
            count2     <= 6'd0;
            count2_end <= 1'b1; 
        end
        else begin
            count2     <= count2 + 1;
            count2_end <= 1'b0;  
        end	 
    end

    assign yad_ck = yad_cs?1'b1:clkin;

    // 【修改】改为 posedge 计数
    always@(posedge yad_ck or negedge rst_n)
    if(!rst_n) begin
        count1      <= 6'd0;
        count1_end  <= 1'b0; 
    end
    else if(count1==6'd63)begin
        count1 <= 6'd0;
        count1_end <= 1'b1; 
    end
    else begin
        count1 <= count1 + 1;
        count1_end <= 1'b0;  
    end	 
        
    // 【修改】改为 posedge 采样
    always@(posedge yad_ck or negedge rst_n)
    if(!rst_n) begin
        data_a <= 64'd0;
        data_b <= 64'd0;
    end
    else begin
        data_a <= {data_a[62:0],yad_sa};
        data_b <= {data_b[62:0],yad_sb};
    end
    
	assign  dout = {data_a[63:32],data_a[31:0],data_b[63:32],data_b[31:0]};
    assign yad_rst = ~rst_n;

endmodule