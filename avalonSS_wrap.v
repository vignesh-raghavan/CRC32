module avalonSS_wrap (
   input clock,
	input resetn,

	input chipselect,
	input [10:0] address,
	
	input read,
	output [31:0] readdata,
	
	input write,
	input [31:0] writedata,

	output waitrequest,
	
	output [31:0] Q_export
);


   reg [7:0] to_reg;
	reg [31:0] from_reg;

	reg [1:0] counter;
	reg [1:0] counter_next;

	wire [31:0] rdata;
	wire empty;
	wire full;
	wire busy;

	reg renable;
	reg enable;
	wire wenable;
	assign wenable = chipselect & write & (address == 11'h0);

	//CRC Registers
	wire [31:0] crc_value; //cleared by write 1 to crc_status register on next clock cycle.
	reg [31:0] crc_status; //w1c, Read computed final CRC value from this register.
	reg [31:0] crc_control; //Control register to configure CRC accelerator.

	wire crc_value_access;
	wire crc_status_access;
	wire crc_status_clear;
	wire crc_control_access;
	wire crc_control_write;
	wire crc_control_clear;

	assign crc_value_access = chipselect & read & (address == 11'h0);
	assign crc_status_access = chipselect & read & (address == 11'h1);
	assign crc_status_clear = chipselect & write & (address == 11'h1) & (writedata == 8'h1);
	assign crc_control_access = chipselect & read & (address == 11'h2);
	assign crc_control_write = chipselect & write & (address == 11'h2);
	assign crc_control_clear = empty & (counter == 2'b00) & (~waitrequest);
  
	
	synch_fifo FIFO0 (
		.clock(clock),
		.resetn(resetn),

		.wen(wenable),
		.ren(renable),
		.wdata(writedata),
		.rdata(rdata),

		.full(full),
		.empty(empty)
	);

	always @(posedge clock) begin
		if(!resetn) begin
			counter <= 2'b00;
		end
		else begin
			counter <= counter_next;
		end
	end

	always @(counter or empty or rdata or crc_control) begin
		case({empty, counter})
		3'b000 : begin
					counter_next = 2'b01;
					to_reg = rdata[7:0];
					enable = 1'b1;
					renable = 1'b0;
					end
		3'b001 : begin
					counter_next = 2'b10;
					to_reg = rdata[15:8];
					enable = 1'b1;
					renable = 1'b0;
					end
		3'b010 : begin
					counter_next = 2'b11;
					to_reg = rdata[23:16];
					enable = 1'b1;
					renable = 1'b0;
					end
		3'b011 : begin
					counter_next = 2'b00;
					to_reg = rdata[31:24];
					enable = 1'b1;
					renable = 1'b1;
					end
		3'b100 : begin
					counter_next = 2'b00;
					to_reg = crc_control[7:0];
					enable = crc_control[31];
					renable = 1'b0;
					end
		default: begin
					counter_next = 2'b00;
					to_reg = 8'h0;
					enable = 1'b0;
					renable = 1'b0;
					end
		endcase
	end

	assign waitrequest = full | busy;
	assign busy = (crc_control_write & ~empty);


	crc_acc crc0 (
		.clock(clock),
		.resetn( (resetn & (~crc_status_clear)) ),
		
		.enable(enable),
		
		.data(to_reg),

		.crc(crc_value)
	);

	always @(posedge clock) begin
		if(!resetn) begin
			crc_status <= 32'h0;
		end
		else if(crc_status_clear) begin
			crc_status <= 32'h0;
		end
		else begin
			crc_status <= (~crc_value);
		end
	end

	always @(posedge clock) begin
		if(!resetn) begin
			crc_control <= 32'h0;
		end
		else if(crc_control_write & crc_control_clear) begin
			crc_control <= writedata[31:0];
		end
		else if(crc_control_clear) begin
			crc_control <= 32'h0;
		end
	end


	always @(crc_value_access or crc_status_access or crc_control_access) begin
		if(crc_value_access) begin
			from_reg = crc_value;
		end
		else if(crc_status_access) begin
			from_reg = crc_status;
		end
		else if(crc_control_access) begin
			from_reg = crc_control;
		end
		else begin
			from_reg = 32'h0;
		end
	end
	 
   assign readdata = from_reg;
   assign Q_export = crc_value;

endmodule
