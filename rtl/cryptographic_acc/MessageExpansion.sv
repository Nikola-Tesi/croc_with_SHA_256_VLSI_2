// Copyright 2021 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// Author: Nikola Tesic, ETH Zurich

module MessageExpansion (
    input  logic [31:0] wkk15_i,
    input  logic [31:0] wkk2_i,
    input  logic [31:0] wkk16_i,
    input  logic [31:0] wkk7_i,
    output logic [31:0] wkk_o 
);

    logic [31:0] csac_1_comb;
    logic [31:0] csas_1_comb;
    logic [31:0] csac_2_comb;
    logic [31:0] csas_2_comb;
    logic [31:0] d0_comb;
    logic [31:0] d1_comb;
    logic [31:0] wkk_new_comb;

    assign d0_comb =    ((wkk15_i >> 7)  | (wkk15_i << (32-7)))      // Rotate right 7
                      ^ ((wkk15_i >> 18) | (wkk15_i << (32-18)))     // Rotate right 18
                      ^ ( wkk15_i >> 3);                             // Shift right 3 (no rotate)
    assign d1_comb =    ((wkk2_i >> 17)  | (wkk2_i << (32-17)))      // Rotate right 17
                      ^ ((wkk2_i >> 19)  | (wkk2_i << (32-19)))      // Rotate right 19
                      ^ ( wkk2_i >> 10);                             // Shift right 10 (no rotate)

    ethz_csa #(.WIDTH(32)) CSA_1 (
        .x_i ( wkk16_i     ),
        .y_i ( wkk7_i      ),
        .z_i ( d0_comb     ),
        .c_o ( csac_1_comb ),
        .s_o ( csas_1_comb )
    );

    ethz_csa #(.WIDTH(32)) CSA_2 (
        .x_i ( csac_1_comb ),
        .y_i ( csas_1_comb ), 
        .z_i ( d1_comb     ),
        .c_o ( csac_2_comb ),
        .s_o ( csas_2_comb )
    );  

    assign wkk_new_comb = csac_2_comb + csas_2_comb;

    assign wkk_o = wkk_new_comb;

endmodule