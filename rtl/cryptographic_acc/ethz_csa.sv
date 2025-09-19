// Copyright 2021 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// Author: Marco Bertuletti, ETH Zurich

module ethz_csa #(
  parameter int WIDTH = 32
)(
  input  logic [WIDTH-1:0] x_i,
  input  logic [WIDTH-1:0] y_i,
  input  logic [WIDTH-1:0] z_i,
  output logic [WIDTH-1:0] c_o,
  output logic [WIDTH-1:0] s_o
);

  logic [WIDTH:0] cfull_comb_o;

  // Sum calculation (XOR operation)
  assign s_o = x_i ^ y_i ^ z_i;

  // Carry calculation (AND-OR operation)
  assign cfull_comb_o = (x_i & y_i) | (x_i & z_i) | (y_i & z_i);
  
  // Assign the carry output
  assign c_o = {cfull_comb_o[WIDTH-1:0], 1'b0}; 
  
endmodule