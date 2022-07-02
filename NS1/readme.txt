# tcp-sim

This is the TCP Simulation based on NS-2 created by Tao Zhao and Anil
Balasubramanya Murthy (Team 20) for the first network simulation assignment
of TAMU ECEN 602 in 2016 fall.

## Architecture

The code is in `ns2.tcl`, which takes care of handling command line arguments
as well as defining nodes, connections, and events (both for packet
transmission and performance measurement).

## Build & Installation

You can run the script `ns2.tcl` directly following the usage below.
No additional build and installation is needed.

## Usage

The invocation syntax is:

    ns ns2.tcl <TCP_flavor> <case_no>

where `TCP_flavor` is one of VEGAS and SACK, and `case_no` is one of 1, 2, 3.
For example,

    ns ns2.tcl VEGAS 2

will run the simulation with TCP VEGAS over Case 2.

## Errata

None.
