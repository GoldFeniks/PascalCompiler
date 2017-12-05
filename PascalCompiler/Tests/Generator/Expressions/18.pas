program test;
var 
    a, b: real;
    c: integer;
begin
    a := 10.5;
    b := 20.5;
    write(a < b, a > b, a <= b, a >= b, a = b, a <> b, not (a = b));
end.