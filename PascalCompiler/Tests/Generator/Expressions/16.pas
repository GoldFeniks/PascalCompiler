program test;
var 
    a, b, c: integer;
begin
    a := 10;
    b := 20;
    write(a < b, a > b, a <= b, a >= b, a = b, a <> b, not (a = b));
end.