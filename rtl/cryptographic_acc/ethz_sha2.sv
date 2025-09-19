// Copyright 2021 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// Author: Nikola Tesic, ETH Zurich
module ethz_sha2
import shapkg::*;
(
    input logic clk_i,          // Clock input
    input logic rst_ni,         // Reset input (active low)
    input sbr_obi_req_t user_sbr_obi_req_i, // Input message to hash or to save
    input mgr_obi_rsp_t user_mgr_obi_rsp_i, // Reacts to the processed signal or give the read value
    output logic irq,   // Interrupt request signal
    output sbr_obi_rsp_t user_sbr_obi_rsp_o, // Response to input messages
    output mgr_obi_req_t user_mgr_obi_req_o // We provide the processed signal or request to read the memory
);

    //For the management of the signal with the external interface
    logic [31:0] rdata_i;
    logic [4:0] rid_i;
    logic err_i;
    logic gnt_i;
    logic rvalid_i;
    logic [31:0] addr_o;
    logic we_o;
    logic req_o_q;
    logic req_o_d;
    logic read_q;
    logic read_d;

    //for hash
    logic [31:0] wkk_main_comb;
    logic [31:0] kkk_main_comb;

    logic [31:0] wkk15_comb , wkk2_comb, wkk16_comb, wkk7_comb, wkk_new_comb; 
    logic [31:0] words_q [0:15];
    logic [31:0] words_d [0:15];

    logic [31:0] ato_h_q [0:7];
    logic [31:0] ato_h_d [0:7];
    logic [31:0] ato_h_main_comb [0:7];
    logic [31:0] ato_h_new_comb [0:7]; 
    logic [31:0] next_chunk_comb [0:15];

    //for output
    logic [31:0] hout_q [0:7];
    logic [31:0] hout_d [0:7];
    logic [31:0] hout_o;
    
    // State machine states
    typedef enum logic [1:0] {
        Idle    = 2'b00,
        Hashing   = 2'b01,
        Chank_load = 2'b10,
        Output = 2'b11
    } state;
    
    state state_q, state_d;

    //counter for hash
    logic [8:0] coun_h_q;
    logic [8:0] coun_h_d;

    //counter for input and output
    logic [5:0] coun_io_q;
    logic [5:0] coun_io_d;

    ////////////////////For the management of the signal, memory address, or hash////////////////////////
    acc_hw_req_t acc_hw_req_i;
    acc_hw_rsp_t acc_hw_rsp_o;
    
    logic [31:0] addr_inp_hand;
    logic [31:0] rdata_inp_hand; 
    logic gnt_inp_hand; 

    logic start_mem_addr; // the signal for memory address
    logic wdata_acc_i; // for polling

    assign acc_hw_req_i.addr = addr_inp_hand;
    assign acc_hw_req_i.start_mem_addr = start_mem_addr;

    assign rdata_inp_hand = acc_hw_rsp_o.rdata;
    assign gnt_inp_hand = acc_hw_rsp_o.gnt;

    assign rdata_i = user_mgr_obi_rsp_i.r.rdata;
    assign rid_i = user_mgr_obi_rsp_i.r.rid;
    assign err_i = user_mgr_obi_rsp_i.r.err;
    assign gnt_i = user_mgr_obi_rsp_i.gnt;
    assign rvalid_i = user_mgr_obi_rsp_i.rvalid;

    assign user_mgr_obi_req_o.a.addr = addr_o; 
    assign user_mgr_obi_req_o.a.we = we_o;
    assign user_mgr_obi_req_o.a.be = 4'b1111;
    assign user_mgr_obi_req_o.a.wdata = hout_o;
    assign user_mgr_obi_req_o.a.aid = 1'b0;
    assign user_mgr_obi_req_o.a.a_optional = 0;
    assign user_mgr_obi_req_o.req = req_o_q;


    input_handling input_handling (
        .rst_ni             ( rst_ni             ),
        .clk_i              ( clk_i              ),
        .wdata_acc_i        ( wdata_acc_i        ),
        .user_sbr_mem_req_i ( user_sbr_obi_req_i ),
        .user_sbr_mem_rsp_o ( user_sbr_obi_rsp_o ),
        .acc_hw_req_i       ( acc_hw_req_i       ),
        .acc_hw_rsp_o       ( acc_hw_rsp_o       )
    );
    //////////////////////////////////UP TO HERE//////////////////////////////////

    // The main memory (Words memory, Hash value, Output memory, counter memory and some minor memory)
    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            read_q <= 1'b0;
            req_o_q <= 1'b0;
            coun_h_q <= 0;
            coun_io_q <= 0;
            state_q <= Idle;
            for(int i =0; i < 16; i++) begin
                words_q[i] <= 0;
            end

            ato_h_q <= SHA_IV;

            for(int i = 0; i < 8; i++) begin
                hout_q[i] <= 0;
            end
        end else begin

            //some values useful for input and output 
            read_q <= read_d;
            req_o_q <= req_o_d;

            //counter
            coun_h_q <= coun_h_d;
            coun_io_q <= coun_io_d;

            //state
            state_q <= state_d;

            //Words, H-value, e Output
            words_q <= words_d;
            ato_h_q <= ato_h_d;
            hout_q  <= hout_d;
        end
    end

    // Create new words for hashing
    MessageExpansion msg_expansion (
        .wkk15_i    ( wkk15_comb   ),
        .wkk2_i     ( wkk2_comb    ),
        .wkk16_i    ( wkk16_comb   ),
        .wkk7_i     ( wkk7_comb    ),
        .wkk_o      ( wkk_new_comb )
    );

    // Hash function
    MainLoop main_loop (
        .kkk_i      ( kkk_main_comb   ),
        .wkk_i      ( wkk_main_comb   ),
        .ato_h_i    ( ato_h_main_comb ),
        .ato_h_o    ( ato_h_new_comb  )
    );

    // For next chank
    always_comb begin
        for (int i = 0; i < 16; i++) begin
            if(i==0) begin 
                next_chunk_comb[i] = 32'b10000000000000000000000000000000;
            end else if(i==15) begin
                next_chunk_comb[i] = 32'b00000000000000000000001000000000;
            end else begin
                next_chunk_comb[i] = 32'b000000000000000000000000000000;
            end
        end
    end  

    // State transition and control logic
    always_comb begin
        state_d = state_q;
        req_o_d = req_o_q;
        ato_h_d = ato_h_q;  
        coun_h_d = coun_h_q;
        coun_io_d = coun_io_q; 
        read_d = read_q;
        words_d = words_q;
        hout_d = hout_q ;
        irq = 1'b0;
        we_o = 1'b0;  
        hout_o = 32'b0;
        start_mem_addr = 1'b0;
        addr_inp_hand = 32'b0;
        addr_o = 32'b0;
        wkk15_comb = 0;
        wkk2_comb = 0;
        wkk16_comb = 0;
        wkk7_comb = 0;
        wkk_main_comb = 0;
        wdata_acc_i = 0;
        kkk_main_comb = 0;
        ato_h_main_comb = ato_h_q;

        case (state_q)
            Idle: begin
                hout_d = ato_h_q;
                if ((rvalid_i == 1 && err_i == 0 && coun_io_q < 16)) begin
                    words_d[coun_io_q] = rdata_i; 
                end 
                addr_inp_hand = 32'h2000_0000;
                if((gnt_inp_hand == 1 || read_q == 1)) begin
                    hout_o = 0;
                    if((rvalid_i == 1) && (err_i == 0) && (rid_i == 0) && (req_o_q == 1)) begin // rid_i == 0 if aid == 0
                        coun_io_d = coun_io_q + 1;
                        req_o_d = 1'b0;
                    end else if((rvalid_i == 0) && (err_i == 0) && (rid_i == 5'b11111) && (gnt_i == 0) && (req_o_q == 1)) begin
                        coun_io_d = coun_io_q;
                        req_o_d = 1'b0;
                    end else begin
                        coun_io_d = coun_io_q;
                        req_o_d = 1'b1;
                    end 
                    addr_o = rdata_inp_hand + (coun_io_q)*4;
                    if(addr_inp_hand == 32'h2000_000C) begin
                        wdata_acc_i = 0;
                    end
                end else begin
                    coun_io_d = coun_io_q;
                end
                if(coun_io_q==17) begin
                    state_d = Hashing;
                end
            end
            Hashing: begin
                req_o_d = 1'b0;
                read_d = 0;
                coun_io_d= 0;
                coun_h_d = coun_h_q + 1;  
                if (((coun_h_q) % 65)> 15) begin
                    wkk15_comb = words_q[1];
                    wkk2_comb = words_q[14];
                    wkk16_comb = words_q[0];
                    wkk7_comb = words_q[9];
                    wkk_main_comb = wkk_new_comb;
                end else if (((coun_h_q) % 65) <= 15) begin
                    wkk_main_comb = words_q[(coun_h_q) % 65];
                end
                if(((coun_h_q) % 65)>15) begin
                    words_d[0:14] = words_q[1:15];
                    words_d[15] = wkk_new_comb;
                end
                kkk_main_comb = K_INITIAL[(coun_h_q)%65];
                ato_h_main_comb = ato_h_q;
                ato_h_d = ato_h_new_comb;                 
                if(coun_h_q == 64 || coun_h_q == 129) begin
                    for(int i = 0; i<8; i++) begin
                        hout_d[i] = ato_h_q[i] + hout_q[i];
                    end
                end
                if(coun_h_q == 64) begin
                    state_d = Chank_load;
                end else if(coun_h_q == 130) begin
                    state_d = Output;
                end
            end
            Chank_load: begin
                words_d = next_chunk_comb;
                ato_h_d = hout_q;
                state_d = Hashing;
            end

            Output: begin
                we_o = 1'b1;
                ato_h_d = ato_h_new_comb;
                hout_o = hout_q[coun_io_q];
                addr_inp_hand = 32'h2000_0004;
                addr_o = rdata_inp_hand + (coun_io_q)*4;
                if((rvalid_i == 1) && (err_i == 0) && (rid_i == 0) && (req_o_q == 1)) begin // rid_i == 0 if aid == 0
                    coun_io_d = coun_io_q + 1;
                    req_o_d = 1'b0;
                end else if((rvalid_i == 0) && (err_i == 0) && (rid_i == 5'b11111) && (gnt_i == 0) && (req_o_q == 1)) begin
                    coun_io_d = coun_io_q;
                    req_o_d = 1'b0;
                end else begin
                    coun_io_d = coun_io_q;
                    req_o_d = 1'b1;
                end 
                if(coun_io_q == 8) begin
                    start_mem_addr = 1'b1;
                    addr_inp_hand = 32'h2000_000C;
                    coun_io_d = 0;
                    coun_h_d = 0;
                    req_o_d = 1'b0;
                    irq = 1; //for wfi()
                    wdata_acc_i = 1'b1;
                    ato_h_d = SHA_IV;
                    state_d = Idle;
                end
            end
        endcase
    end

endmodule