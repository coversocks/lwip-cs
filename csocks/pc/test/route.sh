#!/bin/bash
ip route add 0.0.0.0/1 via 192.168.100.200
ip route add 128.0.0.0/1 via 192.168.100.200
ip route add 192.168.100.1/32 via 192.168.100.200
ip route add 192.168.65.1/32 via 192.168.100.200
# ip route add 216.58.199.4 via 192.168.100.200
