# net-sim

This is the Network Simulation based on NS-2 created by Tao Zhao and Anil
Balasubramanya Murthy (Team 20) for the second network simulation assignment
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

    ns ns2.tcl <queue_mechanism> <scenario_no>

where `queue_mechanism` is one of DROPTAIL and RED, and `scenario_no` is one
of 1 and 2.

For example,

    ns ns2.tcl RED 2

will run the simulation with RED queue mechanism over Scenario 2.

The instantaneous and average throughput measurements are written to the
standard output. Each line of output has the following ordered fields:

- timestamp
- instantaneous throughput of TCP link 1 (H1 to H3)
- instantaneous throughput of TCP link 2 (H2 to H4)
- instantaneous throughput of UDP link (Scenario 2 only)
- average throughput of TCP link 1
- average throughput of TCP link 2
- average throughput of UDP link (Scenario 2 only)

## Errata

None.
