
## level1
phase 1 is just the string comparision

```
400ee4:	be 00 24 40 00       	mov    $0x402400,%esi
400ee9:	e8 4a 04 00 00       	callq  401338 <strings_not_equal>
400eee:	85 c0                	test   %eax,%eax
400ef0:	74 05                	je     400ef7 <phase_1+0x17>
```

The string is located at 0x402400, the value is
`Border relations with Canada have never been better.`

## level2

Analysis of `read_six_number`

```
000000000040145c <read_six_numbers>:                             # read six number on the rsi pointed stack by sequence
  40145c:	48 83 ec 18          	sub    $0x18,%rsp
  401460:	48 89 f2             	mov    %rsi,%rdx
  401463:	48 8d 4e 04          	lea    0x4(%rsi),%rcx
  401467:	48 8d 46 14          	lea    0x14(%rsi),%rax
  40146b:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  401470:	48 8d 46 10          	lea    0x10(%rsi),%rax
  401474:	48 89 04 24          	mov    %rax,(%rsp)
  401478:	4c 8d 4e 0c          	lea    0xc(%rsi),%r9
  40147c:	4c 8d 46 08          	lea    0x8(%rsi),%r8
  401480:	be c3 25 40 00       	mov    $0x4025c3,%esi
  401485:	b8 00 00 00 00       	mov    $0x0,%eax
  40148a:	e8 61 f7 ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  40148f:	83 f8 05             	cmp    $0x5,%eax
  401492:	7f 05                	jg     401499 <read_six_numbers+0x3d>
  401494:	e8 a1 ff ff ff       	callq  40143a <explode_bomb>
  401499:	48 83 c4 18          	add    $0x18,%rsp
  40149d:	c3                   	retq   
```

The `%rsi` points to the caller's `%rsp`

The parameters that passd to `sscanf` is as follows

| num | place   | value     |
|-----|---------|-----------|
| 1st | %rdx    | %rsi      |
| 2nd | %rcx    | %rsi + 4  |
| 3rd | %r8     | %rsi + 8  |
| 4th | %r9     | %rsi + 12 |
| 5th | (%rsp)  | %rsi + 16 |
| 6th | 8(%rsp) | %rsi + 20 |

Solve the phase 2
```
400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp)
400f0e:	74 20                	je     400f30 <phase_2+0x34>		# jump to L1
400f10:	e8 25 05 00 00       	callq  40143a <explode_bomb>
400f15:	eb 19                	jmp    400f30 <phase_2+0x34>
400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax          # .L2
400f1a:	01 c0                	add    %eax,%eax
400f1c:	39 03                	cmp    %eax,(%rbx)
400f1e:	74 05                	je     400f25 <phase_2+0x29>    # jump to L3
400f20:	e8 15 05 00 00       	callq  40143a <explode_bomb>
400f25:	48 83 c3 04          	add    $0x4,%rbx                # .L3
400f29:	48 39 eb             	cmp    %rbp,%rbx
400f2c:	75 e9                	jne    400f17 <phase_2+0x1b>   # jump to L2
400f2e:	eb 0c                	jmp    400f3c <phase_2+0x40>   # jump to END
400f30:	48 8d 5c 24 04       	lea    0x4(%rsp),%rbx           # .L1
400f35:	48 8d 6c 24 18       	lea    0x18(%rsp),%rbp
400f3a:	eb db                	jmp    400f17 <phase_2+0x1b>  # jump to L2
400f3c:	48 83 c4 28          	add    $0x28,%rsp              # END.
```

The core checker of phase 2 is as follows:
```
1st elem needs to be 1

eax = n-1 th elem
eax *= 2
required eax = n th elem
```

As a result, the answer should be: `1 2 4 8 16 32`

## level3

Level  3 is simple

```
400f5b:	e8 90 fc ff ff       	callq  400bf0 <__isoc99_sscanf@plt> # read one int
400f60:	83 f8 01             	cmp    $0x1,%eax
400f63:	7f 05                	jg     400f6a <phase_3+0x27>     # if > 1 jump to L2
400f65:	e8 d0 04 00 00       	callq  40143a <explode_bomb>
400f6a:	83 7c 24 08 07       	cmpl   $0x7,0x8(%rsp)             # .L2
400f6f:	77 3c                	ja     400fad <phase_3+0x6a>      # if > 0x7 jump to bomb
400f71:	8b 44 24 08          	mov    0x8(%rsp),%eax
400f75:	ff 24 c5 70 24 40 00 	jmpq   *0x402470(,%rax,8)         # jump to 0x402470 + rax * 8
400f7c:	b8 cf 00 00 00       	mov    $0xcf,%eax
400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b>       # jump to L1
400f83:	b8 c3 02 00 00       	mov    $0x2c3,%eax
400f88:	eb 34                	jmp    400fbe <phase_3+0x7b>			# jump to L1
400f8a:	b8 00 01 00 00       	mov    $0x100,%eax
400f8f:	eb 2d                	jmp    400fbe <phase_3+0x7b>			# jump to L1
400f91:	b8 85 01 00 00       	mov    $0x185,%eax
400f96:	eb 26                	jmp    400fbe <phase_3+0x7b>			# jump to L1
400f98:	b8 ce 00 00 00       	mov    $0xce,%eax
400f9d:	eb 1f                	jmp    400fbe <phase_3+0x7b>			# jump to L1
400f9f:	b8 aa 02 00 00       	mov    $0x2aa,%eax
400fa4:	eb 18                	jmp    400fbe <phase_3+0x7b>				# jump to L1
400fa6:	b8 47 01 00 00       	mov    $0x147,%eax
400fab:	eb 11                	jmp    400fbe <phase_3+0x7b>       # jump to L1
400fad:	e8 88 04 00 00       	callq  40143a <explode_bomb>       # BOMB
400fb2:	b8 00 00 00 00       	mov    $0x0,%eax
400fb7:	eb 05                	jmp    400fbe <phase_3+0x7b>        # jump to L1
400fb9:	b8 37 01 00 00       	mov    $0x137,%eax
400fbe:	3b 44 24 0c          	cmp    0xc(%rsp),%eax        # .L1
400fc2:	74 05                	je     400fc9 <phase_3+0x86>       # jump to end
400fc4:	e8 71 04 00 00       	callq  40143a <explode_bomb>
400fc9:	48 83 c4 18          	add    $0x18,%rsp
400fcd:	c3                   	retq   
```

The phase 3 takes two argument. The first is an index in a switch statement,
and the second is the value. It checks the value is equal to the certain value
in index_i of the switch statement. My answer is `1 314`

## level4

Just the simple recursion function call.
Takes two args `a b`, `b` must be `0`.
`a` is the arg passed to `func4`. The `func4` is simple recursion function. `a`
must `be` than `0xe`
call likes `func(a, 0 , 0xe)`. We need the `func` return 0. We can reverse the
function and also can go over the function just try a input, I just find 1 will
work so I does not reverse the function (actually there are only 15 tries, we
can use gdb `call` to go over 15 guess. My answer is `1 0`.

## level5

There is a strings at the location `0x40245e` which is `flyers`. The input of
phase5 should be 6 byte length string and each bytes is the index in the
dictonary string (which is at the location `0x4024b0`. We need to reconstruct
the string `flyers` with the inputted indexes. The indexers should be `0x9 0xf 0xe
0x5 0x6 0x7`. As a result, the input string should he the ascii decoded of the index.


## level6

The level6 is non-trival. It read 6 integers. For each input `i`, set `j = 7 - i`. The `j` is
reordering index for a linked list at `0x6032d0`. We need reorder the linked list
as descending order. The linked list is as follows:

| addr     | uint |
|----------|------|
| 0x6032d0 | 332  |
| 0x6032e0 | 168  |
| 0x6032f0 | 924  |
| 0x603300 | 691  |
| 0x603310 | 477  |
| 0x603320 | 443  |

The reoridering index should be `3 4 5 6 1 2`, so the input should be `4 3 2 1 6 5`

## secret phase.

When defuse all 6 phases, we can enter the secret phase. The entry point of
secret phase is as follows:

```
4015f0:	be 19 26 40 00       	mov    $0x402619,%esi
4015f5:	bf 70 38 60 00       	mov    $0x603870,%edi
4015fa:	e8 f1 f5 ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
4015ff:	83 f8 03             	cmp    $0x3,%eax
401602:	75 31                	jne    401635 <phase_defused+0x71>
401604:	be 22 26 40 00       	mov    $0x402622,%esi
401609:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi
40160e:	e8 25 fd ff ff       	callq  401338 <strings_not_equal>
401613:	85 c0                	test   %eax,%eax
401615:	75 1e                	jne    401635 <phase_defused+0x71>
401617:	bf f8 24 40 00       	mov    $0x4024f8,%edi
40161c:	e8 ef f4 ff ff       	callq  400b10 <puts@plt>
401621:	bf 20 25 40 00       	mov    $0x402520,%edi
401626:	e8 e5 f4 ff ff       	callq  400b10 <puts@plt>
40162b:	b8 00 00 00 00       	mov    $0x0,%eax
401630:	e8 0d fc ff ff       	callq  401242 <secret_phase>
```

The `0x402619` stores to string `%d %d %s` and the `0x603879` stores the input
strings of phase4. It means we need to append a string to phase4's input, and
the string should be equal to what is stored in `0x402622`, which is `DrEvil`.
The secret phase takes 1 input which is value of the `root -> go left -> go right` node
of a BST which start at `0x6030f0` and the value is `22`.
