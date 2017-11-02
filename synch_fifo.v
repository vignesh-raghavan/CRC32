module synch_fifo (
	input clock,
	input resetn,

	input wen,
	input ren,
	input [31:0] wdata,
	output [31:0] rdata,

	output full,
	output empty
);

reg [5:0] wptr;
reg [5:0] rptr;

reg [31:0] fifofile [0:31]; // 32 Deep FIFO memory

reg [31:0] fout;

assign full = (wptr[4:0] == rptr[4:0]) & (wptr[5] != rptr[5]);
assign empty = (wptr == rptr);

always @(posedge clock) begin
	if(!resetn) begin
		wptr <= 6'h0;
		rptr <= 6'h0;
	end
	else  begin
		if(wen & ~full) begin
			fifofile[wptr[4:0]] <= wdata;
			wptr <= wptr + 6'h1;
		end
		if(ren & ~empty) begin
			rptr <= rptr + 6'h1;
		end
	end
end

assign rdata = fifofile[rptr[4:0]];

endmodule
