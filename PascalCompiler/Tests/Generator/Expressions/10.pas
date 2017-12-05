program test;
var 
    a, b, c, d: integer;
begin
    a := 2;
    b := 5;
    c := 64;
    write(a << b, ' ', c >> b, ' ', a << 5, ' ', c >> 5, ' ', a and b, ' ', a or b, ' ', a xor b, ' ', not a);
end.