program test;
var 
    a: integer = 10;
    b: integer = 20;
    c: integer;
begin
    write(a < b, a > b, a <= b, a >= b, a = b, a <> b, not (a = b));
end.