module CRC_custom_combinational(dataa, datab, result);

input [31:0] dataa; //32-bit CRC
input [7:0] datab; //8-bit Data

output [31:0] result; //32-bit CRC (Updated)

reg [31:0] crc_mem[0:255]; //CRC TABLE

wire [31:0] crc_lookup;


initial begin
	$readmemh("CRC.hex", crc_mem);
end

assign crc_lookup = crc_mem[ dataa[31:24] ^ datab[7:0] ];

assign result = { (dataa[23:0] ^ crc_lookup[31:8]), crc_lookup[7:0] };


endmodule
