module MainLoop 
import shapkg::*;
(
    input  logic [31:0] kkk_i,
    input  logic [31:0] wkk_i,
    input  logic [31:0] ato_h_i [0:7],  
    output logic [31:0] ato_h_o [0:7] 
);
    logic [31:0] s_0_comb;
    logic [31:0] maj_comb;
    logic [31:0] ch_comb;
    logic [31:0] s_1_comb;
    
    // Carry Save Adder signals
    logic [31:0] csac_1_comb;
    logic [31:0] csas_1_comb;
    logic [31:0] csac_2_comb;
    logic [31:0] csas_2_comb;
    logic [31:0] csac_3_comb;
    logic [31:0] csas_3_comb;
    logic [31:0] csac_4_comb;
    logic [31:0] csas_4_comb;
    logic [31:0] csac_5_comb;
    logic [31:0] csas_5_comb;
    logic [31:0] csac_6_comb;
    logic [31:0] csas_6_comb;

    //
    assign s_0_comb = ({ato_h_i[0][21:0], ato_h_i[0][31:22]} ^ ({ato_h_i[0][12:0], ato_h_i[0][31:13]} ^ {ato_h_i[0][1:0], ato_h_i[0][31:2]}));
    assign maj_comb = ((ato_h_i[0] & ato_h_i[1]) ^ (ato_h_i[0] & ato_h_i[2]) ^ (ato_h_i[1] & ato_h_i[2]));
    assign ch_comb  = (ato_h_i[4] & ato_h_i[5]) ^ (~ato_h_i[4] & ato_h_i[6]);
    assign s_1_comb = ({ato_h_i[4][24:0], ato_h_i[4][31:25]} ^ ({ato_h_i[4][10:0], ato_h_i[4][31:11]} ^ {ato_h_i[4][5:0], ato_h_i[4][31:6]}));
    
    ethz_csa #(.WIDTH(32)) CSA_1 (
        .x_i ( kkk_i       ),
        .y_i ( wkk_i       ),
        .z_i ( ato_h_i[7]  ),
        .c_o ( csac_1_comb ),
        .s_o ( csas_1_comb )
    );

    ethz_csa #(.WIDTH(32)) CSA_2 (
        .x_i ( csac_1_comb ),
        .y_i ( csas_1_comb ),
        .z_i ( s_1_comb    ),
        .c_o ( csac_2_comb ),
        .s_o ( csas_2_comb )
    );

    ethz_csa #(.WIDTH(32)) CSA_3 (
        .x_i ( csac_2_comb ),
        .y_i ( csas_2_comb ),
        .z_i ( ch_comb     ),
        .c_o ( csac_3_comb ),
        .s_o ( csas_3_comb )
    );

    ethz_csa #(.WIDTH(32)) CSA_4 (
        .x_i ( csac_3_comb ),
        .y_i ( csas_3_comb ), 
        .z_i ( ato_h_i[3]  ),
        .c_o ( csac_4_comb ),
        .s_o ( csas_4_comb )
    );

    // Assign the result of CSA_4
    assign ato_h_o[4] = csac_4_comb + csas_4_comb;

    //fa temp1+temp2

    ethz_csa #(.WIDTH(32)) CSA_5 (
        .x_i ( csac_3_comb ),
        .y_i ( csas_3_comb ),
        .z_i ( s_0_comb    ),
        .c_o ( csac_5_comb ),
        .s_o ( csas_5_comb )
    );

    ethz_csa #(.WIDTH(32)) CSA_6 (
        .x_i ( csac_5_comb ),
        .y_i ( csas_5_comb ),
        .z_i ( maj_comb    ),
        .c_o ( csac_6_comb ),
        .s_o ( csas_6_comb )
    );

    // Assign the result of CSA_6
    assign ato_h_o[0] = csac_6_comb + csas_6_comb;

    assign ato_h_o[7] = ato_h_i[6];
    assign ato_h_o[6] = ato_h_i[5];
    assign ato_h_o[5] = ato_h_i[4];
    assign ato_h_o[3] = ato_h_i[2];
    assign ato_h_o[2] = ato_h_i[1];
    assign ato_h_o[1] = ato_h_i[0];
    
    

endmodule

