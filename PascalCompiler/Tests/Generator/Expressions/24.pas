program test;
var 
    r, r1: real;
    i, j, b: integer;
    c, c1: char;
begin
    r := 123.45;
    r1 := -100.5;
    i := 48;
    j := -23;
    c := '0';
    c1 := #247;
    b := (c > c1) and (r1 < r);
    write(b);
    b := (i > j) or (r1 > r) xor (c1 <> c);
    write(b);
    b := not ((char(i) = '0') and (c1 <= c)) xor ((j <= i) or (r1 = r));
    write(b);
end.