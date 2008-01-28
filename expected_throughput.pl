#!/usr/bin/perl
# work out the expected throughput of a network filesystem without overlapped
# reads
# tridge@samba.org January 2008

# Note: to get network latency, try ping -s 64 <DEST>

use strict;

if ($#ARGV < 3) {
	print "
Usage: expected_throughput.pl <server_throughput> <network_throughput> <network_latency> <io_size>
  Latencies in Microseconds
  Throughputs in MByte/sec
  IO Size in Bytes
";
	exit(0);
}

my $server_throughput = shift;
my $network_throughput = shift;
my $network_latency = shift;
my $io_size = shift;

# scale to seconds and bytes
$server_throughput *= 1.0e6;
$network_throughput *= 1.0e6;
$network_latency *= 1.0e-6;

my $total_latency = $network_latency;

$total_latency += (1.0/$server_throughput) * $io_size;
$total_latency += (1.0/$network_throughput) * $io_size;

my $expected_throughput = $io_size/$total_latency;

printf "With server throughput  :  %7.1f MByte/s\n", $server_throughput * 1.0e-6;
printf "With network throughput :  %7.1f MByte/s\n", $network_throughput * 1.0e-6;
printf "With network latency    :  %7.1f usec\n", $network_latency * 1.0e6;
printf "With network IO size    :  %7.0f bytes\n", $io_size;
printf ">> Expected throughput is %.2f MByte/sec\n", $expected_throughput * 1.0e-6;

