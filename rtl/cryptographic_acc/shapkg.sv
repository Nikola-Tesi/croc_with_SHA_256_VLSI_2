// Copyright 2021 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// Author: Marco Bertuletti, ETH Zurich

package shapkg;

    // Type declarations
    typedef logic [31:0] h_arr [0:7];  // Array to store the H(0..7) words and AtoH(0..7)
    typedef logic [31:0] k_arr [0:63];  // Array to store the initial K values

    // Constant values
    // Initial values for H array (H_INITIAL)
    localparam h_arr H_INITIAL = {
        32'h6a09e667, 32'hbb67ae85, 32'h3c6ef372, 32'ha54ff53a,
        32'h510e527f, 32'h9b05688c, 32'h1f83d9ab, 32'h5be0cd19
    };

    // Initial values for K array (K_INITIAL)
    localparam k_arr K_INITIAL = {
        32'h428a2f98, 32'h71374491, 32'hb5c0fbcf, 32'he9b5dba5,
        32'h3956c25b, 32'h59f111f1, 32'h923f82a4, 32'hab1c5ed5,
        32'hd807aa98, 32'h12835b01, 32'h243185be, 32'h550c7dc3,
        32'h72be5d74, 32'h80deb1fe, 32'h9bdc06a7, 32'hc19bf174,
        32'he49b69c1, 32'hefbe4786, 32'h0fc19dc6, 32'h240ca1cc,
        32'h2de92c6f, 32'h4a7484aa, 32'h5cb0a9dc, 32'h76f988da,
        32'h983e5152, 32'ha831c66d, 32'hb00327c8, 32'hbf597fc7,
        32'hc6e00bf3, 32'hd5a79147, 32'h06ca6351, 32'h14292967,
        32'h27b70a85, 32'h2e1b2138, 32'h4d2c6dfc, 32'h53380d13,
        32'h650a7354, 32'h766a0abb, 32'h81c2c92e, 32'h92722c85,
        32'ha2bfe8a1, 32'ha81a664b, 32'hc24b8b70, 32'hc76c51a3,
        32'hd192e819, 32'hd6990624, 32'hf40e3585, 32'h106aa070,
        32'h19a4c116, 32'h1e376c08, 32'h2748774c, 32'h34b0bcb5,
        32'h391c0cb3, 32'h4ed8aa4a, 32'h5b9cca4f, 32'h682e6ff3,
        32'h748f82ee, 32'h78a5636f, 32'h84c87814, 32'h8cc70208,
        32'h90befffa, 32'ha4506ceb, 32'hbef9a3f7, 32'hc67178f2
    };

    // SHA initial vector (SHA_IV)
    localparam logic [31:0] SHA_IV [0:7] = {
        32'h6a09e667 ,  32'hbb67ae85 ,  32'h3c6ef372 ,  32'ha54ff53a ,  32'h510e527f ,  32'h9b05688c ,  32'h1f83d9ab , 32'h5be0cd19
    };

    // Width of hash
    localparam integer HWIDTH = 256;


    typedef struct packed {
        logic        UseRReady;    
        logic        CombGnt;       
        int unsigned AddrWidth;     
        int unsigned DataWidth;     
        int unsigned IdWidth;       
        logic        Integrity;     
        logic        BeFull;        
        logic [3:0]  OptionalCfg;   
    } obi_cfg_t;

    localparam obi_cfg_t MgrObiCfg = '{
        UseRReady:   1'b0,
        CombGnt:     1'b0,
        AddrWidth:     32,
        DataWidth:     32,
        IdWidth:        1,
        Integrity:   1'b0,
        BeFull:      1'b1,
        OptionalCfg:  '0
    };

  localparam obi_cfg_t SbrObiCfg = '{
                                  UseRReady:   1'b0,
                                  CombGnt:     1'b0,
                                  AddrWidth:     32,
                                  DataWidth:     32,
                                  IdWidth:        1 + cf_math_pkg::idx_width(4),
                                  Integrity:   1'b0,
                                  BeFull:      1'b1,
                                  OptionalCfg:  '0
                                };

    typedef struct packed {
        logic [  MgrObiCfg.AddrWidth-1:0] addr;
        logic                             we;
        logic [MgrObiCfg.DataWidth/8-1:0] be;
        logic [  MgrObiCfg.DataWidth-1:0] wdata;
        logic [    MgrObiCfg.IdWidth-1:0] aid;
        logic                             a_optional; 
    } mgr_obi_a_chan_t;

    typedef struct packed {
        mgr_obi_a_chan_t a;
        logic            req;
    } mgr_obi_req_t;

    typedef struct packed {
        logic [SbrObiCfg.DataWidth-1:0] rdata;
        logic [  SbrObiCfg.IdWidth-1:0] rid;
        logic                           err;
        logic                           r_optional;
    } sbr_obi_r_chan_t;

    typedef struct packed {
        sbr_obi_r_chan_t r;
        logic            gnt;
        logic            rvalid;
    } sbr_obi_rsp_t;


    typedef struct packed {
        logic [  SbrObiCfg.AddrWidth-1:0] addr;
        logic                             we;
        logic [SbrObiCfg.DataWidth/8-1:0] be;
        logic [  SbrObiCfg.DataWidth-1:0] wdata;
        logic [    SbrObiCfg.IdWidth-1:0] aid;
        logic                             a_optional;
    } sbr_obi_a_chan_t;

    typedef struct packed {
        sbr_obi_a_chan_t a;
        logic            req;
    } sbr_obi_req_t;

    typedef struct packed {
        logic [MgrObiCfg.DataWidth-1:0] rdata;
        logic [  MgrObiCfg.IdWidth-1:0] rid;
        logic                           err;
        logic                           r_optional;
    } mgr_obi_r_chan_t;

    typedef struct packed {
        mgr_obi_r_chan_t r;
        logic            gnt;
        logic            rvalid;
    } mgr_obi_rsp_t;

    
//To define the input of acc_hw

    typedef struct packed {
        logic [  SbrObiCfg.AddrWidth-1:0] addr;
        logic                             start_mem_addr;
    } acc_hw_req_t;


//To define the output of acc_hw

    typedef struct packed {
        logic [SbrObiCfg.DataWidth-1:0] rdata;
        logic                           gnt;
    } acc_hw_rsp_t;

endpackage