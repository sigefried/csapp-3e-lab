define hook-stop
  printf "rip:\n"
  x/2i $pc
  printf "stack:\n"
  x/10xg $rsp
end



#b *0x401911
b *0x4018cf
