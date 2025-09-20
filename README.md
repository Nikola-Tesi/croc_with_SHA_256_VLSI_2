# Moloch - Part 2

This project is based on Moloch, with a few modifications made to meet the prerequisites of the VLSI 2 course at ETH. Small memories were added to the hardware accelerator containing our names, and the overall chip size was increased (2235 µm × 2235 µm).

The work was carried out in collaboration with another student. With additional time available, we extended the chip development and performed partial LVS and DRC verification. However, due to technical issues, a full verification could not be completed.

Except for these minor modifications, the rest of the project remains unchanged, and these changes do not affect the results. For further details, please refer to the [Moloch GitHub page](https://github.com/Nikola-Tesi/croc_with_SHA_256).

If you are curious, the full report submitted for VLSI 2 can be found here: [📄 Download VLSI 2 Report PDF](doc/VLSI2_Report.pdf).

The part I personally find most interesting relates to the DRC errors.

## Configuration

The file organization remains essentially the same. All RTL code for the hardware accelerator can be found in the `rtl/cryptographic_acc` directory. The files `user_domain.sv` and `user_pkg.sv` were modified, as before, to integrate the accelerator with the OBI bus.

On the software side, there are a few small modifications. Four C programs are provided: `sw/testbencher_croc.c`, used to verify the functionality of the new hardware implementation, and `sw/SHA256.c`, `sw/SHA256_1.c`, and `sw/SHA256_2.c`, which were used to compare performance across different reference implementations. The speedup is currently confirmed to be 54×.

As before, the file `openroad/floorplan.tcl` contains the necessary scripts for generating the physical implementation of the design.

