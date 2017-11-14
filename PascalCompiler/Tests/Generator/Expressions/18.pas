program test;
var 
    a: real = 10.5;
    b: real = 20.5;
    c: integer;
begin
    write(a < b, a > b, a <= b, a >= b, a = b, a <> b, not (a = b));
end.