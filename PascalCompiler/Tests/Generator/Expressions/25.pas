program test;
var 
    r, r1, b1: real;
    i, j, b: integer;
    c, c1: char;
begin
    r := 123.45;
    r1 := -100.5;
    i := 48;
    j := -23;
    c := '0';
    c1 := #247;
    b1 := (i + 2 * j - 1) << 6 - r + r1;
    write(b1);
    b := integer((integer(real(c1) - integer(c) + 257) div 2 - i - j - 11) >> 6 * (r - r1));
    write(b);
end.