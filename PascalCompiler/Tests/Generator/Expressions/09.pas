program test;
var 
    a: Integer = 2;
    b: Integer = 5;
    c: Integer = 64;
    d: Integer;
begin
    d := a << b;
    write(d);
    d := c >> b;
    write(d);
    d := a << 5;
    write(d);
    d := c >> 5;
    write(d);
    d := a and b;
    write(d);
    d := a or b;
    write(d);
    d := a xor b;
    write(d);
    d := not a;
    write(d);
end.