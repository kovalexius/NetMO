#!/bin/bash

ip link delete veth0

ip netns delete test_ns
