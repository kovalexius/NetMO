#!/bin/bash

ip link add veth0 type veth peer name veth1

ifconfig veth0 up
ifconfig veth1 up

ifconfig veth0 172.168.10.1 netmask 255.255.255.0

ip netns add test_ns

ip link set dev veth1 netns test_ns

ip netns exec test_ns bash

ifconfig veth1 172.168.10.5 netmask 255.255.255.0
