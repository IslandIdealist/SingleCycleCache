loop sw 1 0 6
     sw 1 0 7
     lw 2 0 two
     lw 3 0 tres
     lw 4 0 four
     lw 5 0 five
     add 6 6 2
     beq 4 6 end
     beq 0 0 loop
     two .fill 2
     tres .fill 3
     four .fill 4
     five .fill 5
