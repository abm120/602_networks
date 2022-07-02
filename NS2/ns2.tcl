# ns2.tcl: tcp simulation based on ns-2 for Assignment #2

# Handle command line arguments.
proc usage {} {
    global argv0
    puts "Usage: ns ns2.tcl <queue_mechanism> <scenario_no>"
    puts ""
    puts "    queue_mechanism   One of DROPTAIL (default) and RED"
    puts "    scenario_no       One of 1 (default) and 2"
    exit 0
}
if {$argc < 1} {
    set queue_mechanism "DROPTAIL"
} else {
    set queue_mechanism [lindex $argv 0]
}
# Allow abbreviated case-insensitive command line arguments.
# e.g. `ns ns2.tcl d` is the same as `ns ns2.tcl DROPTAIL`.
switch -glob -nocase $queue_mechanism {
    d* {
        set queue_mechanism "DROPTAIL"
    }
    r* {
        set queue_mechanism "RED"
    }
    default {
        usage
    }
}
if {$argc < 2} {
    set scenario_no 1
} else {
    set scenario_no [lindex $argv 1]
    if {$scenario_no != 1 && $scenario_no != 2} {
        usage
    }
}
# Create a debugging switch.
set DEBUG 0
proc debug {msg} {
    global DEBUG
    if {$DEBUG} {
        puts $msg
    }
}

debug "$queue_mechanism Scenario $scenario_no"

# Create a new Simulator object.
set ns [new Simulator]

# Color different flows.
$ns color 1 Blue
$ns color 2 Red
$ns color 3 Green

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
# Scenario 1 has two senders, two receivers, and two routers.
# Scenario 2 has one more sender and one more receiver.
set src1 [$ns node]
set src2 [$ns node]
set rcv1 [$ns node]
set rcv2 [$ns node]
set R1 [$ns node]
set R2 [$ns node]
$src1 shape box
$rcv1 shape box
$src2 shape box
$rcv2 shape box
if {$scenario_no == 2} {
    set src3 [$ns node]
    set rcv3 [$ns node]
    $src3 shape box
    $rcv3 shape box
}

# Connections.
switch $queue_mechanism {
    DROPTAIL {
        set queue_type DropTail
    }
    RED {
        set queue_type RED
    }
    default {
        usage
    }
}

# Set default paramters of RED queue mechanism.
Queue/RED set thresh_           10
Queue/RED set maxthresh_        15
Queue/RED set linterm_          50

$ns duplex-link $src1 $R1 10Mb 1ms $queue_type
$ns duplex-link-op $src1 $R1 orient right-down
$ns duplex-link $R1 $R2 1Mb 10ms $queue_type
$ns duplex-link-op $R1 $R2 orient right
$ns queue-limit $R1 $R2 20
$ns duplex-link $R2 $rcv1 10Mb 1ms $queue_type
$ns duplex-link-op $R2 $rcv1 orient right-up
$ns duplex-link $src2 $R1 10Mb 1ms $queue_type
$ns duplex-link-op $src2 $R1 orient right-up
$ns duplex-link $R2 $rcv2 10Mb 1ms $queue_type
$ns duplex-link-op $R2 $rcv2 orient right-down
if {$scenario_no == 2} {
    $ns duplex-link $src3 $R1 10Mb 1ms $queue_type
    #$ns duplex-link-op $src3 $R1 orient right-up
    $ns duplex-link $R2 $rcv3 10Mb 1ms $queue_type
    #$ns duplex-link-op $R2 $rcv3 orient right-down
}

# Agent definitions.
set tcpsrc1 [new Agent/TCP/Reno]
set tcpsrc2 [new Agent/TCP/Reno]
set tcpsink1 [new Agent/TCPSink]
set tcpsink2 [new Agent/TCPSink]
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

if {$scenario_no == 2} {
    set udp [new Agent/UDP]
    set udpsink [new Agent/LossMonitor]
    $udp set class_ 3
    $ns attach-agent $src3 $udp
    $ns attach-agent $rcv3 $udpsink

    set cbr [new Application/Traffic/CBR]
    $cbr attach-agent $udp
    $cbr set type_ CBR
    $cbr set packet_size_ 100
    $cbr set interval_ [expr double(100) * 8 / 1000000]

    $ns connect $udp $udpsink
}

# Event definitions.
# Ignore the first several seconds while measuring throughput.
set t_calc_start 30
set t_sim 180
set t_finish 180.1
# Initize the counters for calculating average throughput.
set ti_calc 1
set cum_bytes1 0
set cum_bytes2 0
set cum_bytes3 0
proc calc {t} {
    global tcpsink1 tcpsink2 ti_calc cum_bytes1 cum_bytes2 cum_bytes3 scenario_no udpsink
    set bytes1 [$tcpsink1 set bytes_]
    set bytes2 [$tcpsink2 set bytes_]
    set cum_bytes1 [expr $cum_bytes1 + $bytes1]
    set cum_bytes2 [expr $cum_bytes2 + $bytes2]
    if {$scenario_no == 2} {
        set bytes3 [$udpsink set bytes_]
        set cum_bytes3 [expr $cum_bytes3 + $bytes3]
        debug "bytes3: $bytes3"
    }
    debug "bytes1: $bytes1, bytes2: $bytes2"
    # Throughput in Kbit/sec.
    set inst_throughput1 [expr double($bytes1) * 8 / 1000]
    set inst_throughput2 [expr double($bytes2) * 8 / 1000]
    set avg_throughput1 [expr double($cum_bytes1) / $ti_calc * 8 / 1000]
    set avg_throughput2 [expr double($cum_bytes2) / $ti_calc * 8 / 1000]
    if {$scenario_no == 2} {
        set inst_throughput3 [expr double($bytes3) * 8 / 1000]
        set avg_throughput3 [expr double($cum_bytes3) / $ti_calc * 8 / 1000]
    }
    incr ti_calc
    if {$scenario_no == 1} {
        puts [format "%d\t%.3f\t%.3f\t%.3f\t%.3f" \
            $t $inst_throughput1 $inst_throughput2 $avg_throughput1 $avg_throughput2]
    } elseif {$scenario_no == 2} {
        puts [format "%d\t%5.2f\t%5.2f\t%6.1f\t%.3f\t%.3f\t%.3f" \
            $t $inst_throughput1 $inst_throughput2 $inst_throughput3 $avg_throughput1 $avg_throughput2 $avg_throughput3]
    }
    $tcpsink1 set bytes_ 0
    $tcpsink2 set bytes_ 0
    if {$scenario_no == 2} {
        $udpsink set bytes_ 0
    }
}
$ns at 0 "$ftp1 start"
$ns at 0 "$ftp2 start"
$ns at $t_calc_start "$tcpsink1 set bytes_ 0"
$ns at $t_calc_start "$tcpsink2 set bytes_ 0"
$ns at $t_sim "$ftp1 stop"
$ns at $t_sim "$ftp2 stop"
if {$scenario_no == 2} {
    $ns at 0 "$cbr start"
    $ns at $t_calc_start "$udpsink set bytes_ 0"
    $ns at $t_sim "$cbr stop"
}
for {set t [expr $t_calc_start + 1]} {$t <= $t_sim} {incr t} {
    $ns at $t "calc $t"
}
$ns at $t_finish "finish"

# Run the simulation.
$ns run

# vim: set sw=4 et ai smarttab tw=80 cc=+1 isk+=-:
