program test;
var 
    a: Integer = 2;
    b: Integer = 5;
    c: Integer = 64;
    d: Integer;
begin
    write(a << b, c >> b, a << 5, c >> 5, a and b, a or b, a xor b, not a);
end.