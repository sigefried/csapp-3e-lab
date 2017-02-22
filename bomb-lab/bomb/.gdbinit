define hook-stop
  printf "rip:\n"
  x/i $pc
  printf "stack:\n"
  x/6xw $rsp
end


define print_args
  display /x $rdi
  display /x $rsi
  display /x $rdx
end

define print_six_node
  set $np = $arg0
  printf "head is %p\n", $np
  set $i = 1
  while $i < 7
    set $next = (void *) *((long *) $np + 1)
    printf "node: %i at: %p: { val:%u, next %p }\n", $i, $np, *(unsigned int *) $np, $next
    set $np = $next
    set $i = $i + 1
  end
end


b fun7
commands
printf "===enter=====\n"
printf "----rax: %d \n\n", $rax
printf "rsi: %d, rdi: %p, \n", $rsi, $rdi
printf "node: { val: %d, left: %p, right: %p }\n" , *$rdi, *($rdi + 0x8), *($rdi+0x10)
printf "=============\n"
end

b *0x401241
commands
printf "===return=====\n"
printf "----rax: %d \n\n", $rax
printf "rsi: %d, rdi: %p, \n", $rsi, $rdi
printf "node: { val: %d, left: %p, right: %p }\n" , *$rdi, *($rdi + 0x8), *($rdi+0x10)
printf "==============\n"
end


