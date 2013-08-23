target remote localhost:3333
monitor soft_reset_halt
monitor sleep 1000
load gcc/rollo.axf
break main
c
d 1
