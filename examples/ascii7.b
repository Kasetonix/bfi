[ ascii7.b -- program displaying first 128 characters using the debug symbol #
(c) Igor Glajcher 2026 ]

Sets the loop counter (at index 0) to 64 minus 3 (the loop only modifies characters from index 2 onwards) and sets up third char 
--->++++++++[<++++++++>-]>++<<

Loop copying the previous character adding one and decrementing the counter located at index 1
[->>[>]<[>>+<<-]>>[<<+>+>-]<+[<]<]>+[>]#

Sets the first byte of the tape to 64 zeroing the second one in the process
<[<]>+++++++[<++++++++>-]<

Zeroes out the rest of the tape
>>[>]<[[-]<]<

Moves the character at index n to index n (plus) 2 and copies it back to indeces n and (n plus 1) 
[[->>+<<]>>[<<+>+>-]<+]#
