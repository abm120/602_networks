# ns2.tcl: tcp simulation based on ns-2 for Assignment #1

# Handle command line arguments.
proc usage {} {
    global argv0
    puts "Usage: ns ns2.tcl <TCP_flavor> <case_no>"
    puts ""
    puts "    TCP_flavor    One of VEGAS (default) and SACK"
    puts "    case_no       One of 1 (default), 2, 3"
    exit 0
}
if {$argc < 1} {
    set TCP_flavor "VEGAS"
} else {
    set TCP_flavor [lindex $argv 0]
}
# Allow abbreviated command line arguments.
# e.g. `ns ns2.tcl v` is the same as `ns ns2.tcl VEGAS`.
switch -glob -nocase $TCP_flavor {
    v* {
        set TCP_flavor "VEGAS"
    }
    s* {
        set TCP_flavor "SACK"
    }
    default {
        usage
    }
}
if {$argc < 2} {
    set case_no 1
} else {
    set case_no [lindex $argv 1]
}
# Create a debugging switch.
set DEBUG 0
proc debug {msg} {
    global DEBUG
    if {$DEBUG} {
        puts $msg
    }
}

debug "TCP $TCP_flavor, Case $case_no"

# Create a new Simulator object.
set ns [new Simulator]

# Color different flows.
$ns color 1 Blue
$ns color 2 Red

# Write trace to file for nam.
set fname "out.nam"
set nf [open $fname w]
$ns namtrace-all $nf

# Cleanup at finish.
proc finish {} {
    global ns nf fname
    $ns flush-trace
    close $nf
    #exec nam $fname &
    exit 0
}

# Topology definition.
# Two senders, two receivers, and two routers.
set src1 [$ns node]
set src2 [$ns node]
set rcv1 [$ns node]
set rcv2 [$ns node]
set R1 [$ns node]
set R2 [$ns node]
$R1 shape box
$R2 shape box

# Connections.
$ns duplex-link $src1 $R1 10Mb 5ms DropTail
$ns duplex-link-op $src1 $R1 orient right-down
$ns duplex-link $R1 $R2 1Mb 5ms DropTail
$ns duplex-link-op $R1 $R2 orient right
$ns duplex-link $R2 $rcv1 10Mb 5ms DropTail
$ns duplex-link-op $R2 $rcv1 orient right-up
switch $case_no {
    1 {
        # 12.5 ms end-2-end delay
        $ns duplex-link $src2 $R1 10Mb 12.5ms DropTail
        $ns duplex-link $R2 $rcv2 10Mb 12.5ms DropTail
    }
    2 {
        # 20 ms end-2-end delay
        $ns duplex-link $src2 $R1 10Mb 20ms DropTail
        $ns duplex-link $R2 $rcv2 10Mb 20ms DropTail
    }
    3 {
        # 27.5 ms end-2-end delay
        $ns duplex-link $src2 $R1 10Mb 27.5ms DropTail
        $ns duplex-link $R2 $rcv2 10Mb 27.5ms DropTail
    }
    default {
        usage
    }
}
$ns duplex-link-op $src2 $R1 orient right-up
$ns duplex-link-op $R2 $rcv2 orient right-down

# Agent definitions.
switch $TCP_flavor {
    VEGAS {
        set tcpsrc1 [new Agent/TCP/Vegas]
        set tcpsrc2 [new Agent/TCP/Vegas]
        set tcpsink1 [new Agent/TCPSink]
        set tcpsink2 [new Agent/TCPSink]
    }
    SACK {
        set tcpsrc1 [new Agent/TCP/Sack1]
        set tcpsrc2 [new Agent/TCP/Sack1]
        set tcpsink1 [new Agent/TCPSink/Sack1]
        set tcpsink2 [new Agent/TCPSink/Sack1]
    }
    default {
        usage
    }
}
$tcpsrc1 set class_ 1
$tcpsrc2 set class_ 2
set ftp1 [new Application/FTP]
set ftp2 [new Application/FTP]
$ns attach-agent $src1 $tcpsrc1
$ftp1 attach-agent $tcpsrc1
$ns attach-agent $src2 $tcpsrc2
$ftp2 attach-agent $tcpsrc2
$ns attach-agent $rcv1 $tcpsink1
$ns attach-agent $rcv2 $tcpsink2

$ns connect $tcpsrc1 $tcpsink1
$ns connect $tcpsrc2 $tcpsink2

# Event definitions.
# Ignore the first 100 seconds while measuring throughput.
set t_calc_start 100
set t_sim 400
# Time interval for calculating throughput.
set ti_calc [expr $t_sim - $t_calc_start]
set t_finish 401
proc calc {} {
    global tcpsink1 tcpsink2 ti_calc
    set bytes1 [$tcpsink1 set bytes_]
    set bytes2 [$tcpsink2 set bytes_]
    debug "bytes1: $bytes1, bytes2: $bytes2"
    # Throughput in Mbit/sec.
    set throughput1 [expr double($bytes1) / $ti_calc * 8 / 1000000]
    set throughput2 [expr double($bytes2) / $ti_calc * 8 / 1000000]
    set ratio [expr $throughput1 / $throughput2]
    puts "Throughput1\tThroughput2\tRatio"
    puts [format "%.3f\t%.3f\t%.3f" $throughput1 $throughput2 $ratio]
}
$ns at 0 "$ftp1 start"
$ns at 0 "$ftp2 start"
$ns at $t_calc_start "$tcpsink1 set bytes_ 0"
$ns at $t_calc_start "$tcpsink2 set bytes_ 0"
$ns at $t_sim "$ftp1 stop"
$ns at $t_sim "$ftp2 stop"
$ns at [expr $t_sim + 0.1] "calc"
$ns at $t_finish "finish"

# Run the simulation.
$ns run

# vim: set sw=4 et ai smarttab tw=80 cc=+1 isk+=-:
