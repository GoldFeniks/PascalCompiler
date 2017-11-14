program test;
var 
    r: real = 123.5;
    r1: real = -100.5;
    i: integer = 48;
    j: integer = -23;
    c: char = '0';
    c1: char = #247;
    b: integer;
    b1: real;
begin
    b1 := (i + 2 * j - 1) << 6 - r + r1;
    write(b1);
    b := integer((integer(real(c1) - integer(c) + 257) div 2 - i - j - 11) >> 6 * (r - r1));
    write(b);
end.