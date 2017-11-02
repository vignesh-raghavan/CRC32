module CRC_custom_multicycle(clk, clk_en, reset, start, done, dataa, datab, n, result);

input clk;
input clk_en;
input reset;

input start;
output done;

input [31:0] dataa;
input [31:0] datab;

// n can take the following values
// 0 - (Wait : 1 cycle) Load 32-bit CRC value from dataa
// 1 - (Wait : 1 cycle) Compute CRC on Lower 8 bits of dataa
// 2 - (Wait : 2 cycle) Compute CRC on Lower 16 bits of dataa
// 3 - (Wait : 3 cycle) Compute CRC on Lower 24 bits of dataa
// 4 - (Wait : 4 cycle) Compute CRC on all 32 bits of dataa
// 5 - (Wait : 8 cycle) Compute CRC on all 32 bits of dataa, followed by all 32 bits of datab
// 6 - (Wait : 1 cycle) Read out 32-bit CRC value through result
// 7 - Reserved
input [2:0] n;

output [31:0] result;


//Internal Signalling
wire [31:0] crc;

wire load;
reg [4:0] enable;


assign load = clk_en & start & (n == 3'h0);

always @(clk_en or n) begin
	enable = 5'b00000;

	if(clk_en) begin
		if(n == 3'h1)      enable = 5'b00001;
		else if(n == 3'h2) enable = 5'b00011;
		else if(n == 3'h3) enable = 5'b00111;
		else if(n == 3'h4) enable = 5'b01111;
		else if(n == 3'h5) enable = 5'b11111;
		else               enable = 5'b00000; 
	end
end


crc_ci crc1 (.clk(clk), .reset(reset), .enable(enable), .load(load), .i_data( {datab, dataa} ), .crc_o(crc), .done(done));


//assign result = ((n == 3'h6) & clk_en & done) ? crc : 32'hA5A5C3C3;
assign result = (clk_en & done) ? crc : 32'hA5A5C3C3; //Output the CRC value for all accesses to this custom instruction. Helps in Software Debug purposes.

endmodule
