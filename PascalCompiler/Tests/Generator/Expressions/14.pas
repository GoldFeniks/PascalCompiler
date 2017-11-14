program test;
var 
    a, b, c, d: real;
begin
    a := 10.5;
    b := 20.5;
    c := 2;
    write(a + b, #10, a - b, #10, a / c, #10,  b * c, #10, -a);
end.