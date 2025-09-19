// Copyright 2021 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// Author: Nikola Tesic, ETH Zurich     
    
    module input_handling
    import shapkg::*;
     (
        input   logic           clk_i,          
        input   logic           rst_ni,
        input   logic           wdata_acc_i,    
        output  sbr_obi_rsp_t   user_sbr_mem_rsp_o, 
        input   sbr_obi_req_t   user_sbr_mem_req_i,
        input   acc_hw_req_t    acc_hw_req_i,
        output  acc_hw_rsp_t    acc_hw_rsp_o
    );
    logic [31:0] mem_addr_q [0:3];
    logic [31:0] mem_addr_d [0:3]; 

    logic [31:0] wdata_croc;
    logic [31:0] rdata_croc;
    logic [31:0] rdata_acc;
    logic [31:0] addr_croc;
    logic [31:0] addr_acc;
    logic [2:0]  aid_croc;
    logic [4:0]  we_croc;
    logic [3:0]  be_croc;
    logic        req_croc;
    logic        gnt_croc;
    logic        gnt_acc;
    logic        err_croc;
    logic        rvalid_croc_q;
    logic        rvalid_croc_d;
    logic        check;
    logic [31:0] index_croc;
    logic [31:0] index_acc;
    logic        bits_for_rdata_q;
    logic        bits_for_rdata_d;
    logic        start_mem_addr;

    //
    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            bits_for_rdata_q <= 1'b0;
            rvalid_croc_q <= 1'b0;
        end else begin
            bits_for_rdata_q <= bits_for_rdata_d;
            rvalid_croc_q <= rvalid_croc_d;
        end
    end

    // Memory address
    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin 
            mem_addr_q[0] <= 32'b0;
            mem_addr_q[1] <= 32'b0;
            mem_addr_q[2] <= 32'b0;
            mem_addr_q[3] <= 32'b0;
        end else begin
            mem_addr_q <= mem_addr_d;
        end
    end

    //reset check
    always_comb begin
        if(start_mem_addr) begin
            check = 1'b0;
        end else begin
            check = mem_addr_q[2][0];
        end
    end 

    // Management of request and response signals
    always_comb begin
        bits_for_rdata_d = bits_for_rdata_q;
        //input croc
        wdata_croc = user_sbr_mem_req_i.a.wdata;
        we_croc = user_sbr_mem_req_i.a.we;
        be_croc = user_sbr_mem_req_i.a.be;
        addr_croc = user_sbr_mem_req_i.a.addr;
        aid_croc = user_sbr_mem_req_i.a.aid;
        req_croc = user_sbr_mem_req_i.req;

        //input acc
        addr_acc = acc_hw_req_i.addr;
        start_mem_addr = acc_hw_req_i.start_mem_addr;

        //error handling
        err_croc = req_croc && (be_croc != 4'b1111 || addr_croc < 32'h2000_0000 || addr_croc >= 32'h2000_1000);
        
        //gnt_mem and gnt_acc handling
        if(check == 0) begin
            gnt_croc = (1'b1 && req_croc);
            gnt_acc = 1'b0;
        end else if(check == 1)begin
            gnt_croc = 1'b0;
            gnt_acc = 1'b1;
        end else begin //for X o Z cases
            gnt_croc = (1'b1 && req_croc);
            gnt_acc = 1'b0;
        end 
        

        if ((we_croc == 0 && err_croc == 0 && addr_croc == 32'h2000_000C) || (bits_for_rdata_q && rvalid_croc_q)) begin
            rdata_croc = mem_addr_q[3];
            bits_for_rdata_d = 1'b1;
        end else begin
            rdata_croc = 32'b0;
            bits_for_rdata_d = 1'b0;
        end

        rvalid_croc_d = gnt_croc;

        //output croc
        user_sbr_mem_rsp_o.r.rdata = rdata_croc;
        user_sbr_mem_rsp_o.r.rid = aid_croc;
        user_sbr_mem_rsp_o.r.err = err_croc;
        user_sbr_mem_rsp_o.r.r_optional = 1'b0;
        user_sbr_mem_rsp_o.gnt = gnt_croc;
        user_sbr_mem_rsp_o.rvalid = rvalid_croc_q;

        //output acc
        acc_hw_rsp_o.rdata = rdata_acc;
        acc_hw_rsp_o.gnt = gnt_acc;

    end

    // Memory address managment
    always_comb begin
        mem_addr_d = mem_addr_q;
        if (start_mem_addr) begin 
            mem_addr_d[0] = 32'b0;
            mem_addr_d[1] = 32'b0;
            mem_addr_d[2] = 32'b0;
            index_croc = 0;
            index_acc = 0;
            rdata_acc = 0;
        end else begin 
           if(err_croc == 0 && check == 0 && req_croc == 1 && (addr_croc < 32'h2000_000C)) begin
                index_croc= (addr_croc - 32'h2000_0000) >> 2;
                mem_addr_d[index_croc] = wdata_croc;
           end else begin
                index_croc = 0;
                mem_addr_d = mem_addr_q;
           end
           if(check == 1) begin
                index_acc= (addr_acc - 32'h2000_0000) >> 2;
                rdata_acc = mem_addr_q[index_acc];
           end else begin
                index_acc = 0;
                rdata_acc = 32'b0;
           end
        end

        if(addr_acc == 32'h2000_000C && check == 0) begin
            mem_addr_d[3] = wdata_acc_i;
        end else if (we_croc == 1 && err_croc == 0 && addr_croc == 32'h2000_000C) begin
            mem_addr_d[3] = wdata_croc;  
        end

    end

endmodule