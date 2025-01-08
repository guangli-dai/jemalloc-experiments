#!/bin/sh
g++ -o generateAllocs generateAllocs.cpp
#sample usage
./generateAllocs 2000000 booting_sim.log booting_stats.log
