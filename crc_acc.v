module crc_acc (
	input clock,
	input resetn,

	input enable,

	input [7:0] data,

	output reg [31:0] crc
);

reg [31:0] crc_next;
reg [31:0] crc_mem[0:255]; //CRC TABLE

initial begin
	$readmemh("CRC.hex", crc_mem);
end

always @(posedge clock)
begin
	if(!resetn) begin
		crc <= 32'h4E08BFB4;
	end
	else begin
		crc <= crc_next;
	end
end

always @(crc or enable or data) begin
	crc_next = crc;

	if(enable) begin
		crc_next = ( {crc[23:0], 8'h0} ) ^ ( crc_mem[ crc[31:24] ^ data ] );
	end
end

endmodule
