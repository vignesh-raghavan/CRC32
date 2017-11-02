module crc_ci (
	input clk,
	input reset,

	input [4:0] enable,
	input load,

	input [63:0] i_data,

	output reg [31:0] crc_o,
	output reg done
);

reg [31:0] crc_mem[0:255]; //CRC TABLE
reg [31:0] crc_o_next;

reg [7:0] bytedata;
reg evaluate;

reg [2:0] counter;
reg [2:0] counter_next;

initial begin
	$readmemh("CRC.hex", crc_mem);
end

always @(posedge clk) begin
	if(reset) begin
		crc_o <= 32'h4E08BFB4;
	end
	else begin
		crc_o <= crc_o_next;
	end
end

always @(crc_o or load or evaluate or i_data or bytedata) begin
	crc_o_next = crc_o;

	if(load) begin
		crc_o_next = i_data;
	end
	else if(evaluate) begin
		crc_o_next = ( { crc_o[23:0], 8'h0 } ^ crc_mem[ crc_o[31:24] ^ bytedata ] );
	end
end


always @(posedge clk) begin
	if(reset) begin
		counter <= 3'b000;
	end
	else begin
		counter <= counter_next;
	end
end


always @(counter or enable or i_data) begin
	counter_next = 3'b000;
	bytedata = 8'h0;
	evaluate  = 1'b0;
	done = 1'b1;

	case(counter)
		3'b000 : begin
						if(enable[0]) begin
						   bytedata = i_data[7:0];
						   evaluate = 1'b1;
						   if(enable[1]) begin
						  	 counter_next = 3'b001;
						  	 done = 1'b0;
						   end
						   else begin
						  	 done = 1'b1;
						   end
				   	end
				  end
		3'b001 : begin
				  		if(enable[1]) begin
							 bytedata = i_data[15:8];
							 evaluate = 1'b1;
							 if(enable[2]) begin
								 counter_next = 3'b010;
								 done = 1'b0;
							 end
							 else begin
								 done = 1'b1;
							 end
				   	end
				  end
		3'b010 : begin
				  		if(enable[2]) begin
							 bytedata = i_data[23:16];
							 evaluate = 1'b1;
							 if(enable[3]) begin
								 counter_next = 3'b011;
								 done = 1'b0;
							 end
							 else begin
								 done = 1'b1;
							 end
				   	end
				  end
		3'b011 : begin
				  		if(enable[3]) begin
							 bytedata = i_data[31:24];
							 evaluate = 1'b1;
							 if(enable[4]) begin
								 counter_next = 3'b100;
								 done =1'b0;
							 end
							 else begin
								 done = 1'b1;
							 end
				   	end
				  end
		3'b100 : begin
				  		if(enable[4]) begin
							 bytedata = i_data[39:32];
							 evaluate = 1'b1;
							 counter_next = 3'b101;
							 done =1'b0;
				   	end
				  end
		3'b101 : begin
				  		if(enable[4]) begin
							 bytedata = i_data[47:40];
							 evaluate = 1'b1;
							 counter_next = 3'b110;
							 done =1'b0;
				   	end
				  end
		3'b110 : begin
				  		if(enable[4]) begin
							 bytedata = i_data[55:48];
							 evaluate = 1'b1;
							 counter_next = 3'b111;
							 done =1'b0;
				   	end
				  end
		3'b111 : begin
				  		if(enable[4]) begin
							 bytedata = i_data[63:56];
							 evaluate = 1'b1;
							 counter_next = 3'b000;
							 done = 1'b1;
				   	end
				  end
	endcase
end

endmodule
