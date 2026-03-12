#!/bin/sh
read cpu user nice system idle rest < /proc/stat
total=$((user+nice+system+idle))
busy=$((user+nice+system))
sleep 0.5
read cpu user2 nice2 system2 idle2 rest < /proc/stat
total2=$((user2+nice2+system2+idle2))
busy2=$((user2+nice2+system2))
dt=$((total2-total))
db=$((busy2-busy))
usage=$((100*db/dt))
echo " CPU ${usage}%"
