program test;
var 
    r: real = 123.45;
    r1: real = -100.5;
    i: integer = 48;
    j: integer = -23;
    c: char = '0';
    c1: char = #247;
    b: integer;
begin
    b := (c > c1) and (r1 < r);
    write(b);
    b := (i > j) or (r1 > r) xor (c1 <> c);
    write(b);
    b := not ((char(i) = '0') and (c1 <= c)) xor ((j <= i) or (r1 = r));
    write(b);
end.