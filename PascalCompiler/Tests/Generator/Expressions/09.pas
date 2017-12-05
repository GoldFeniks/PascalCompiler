program test;
var 
    a, b, c, d: integer;
begin
    a := 2;
    b := 5;
    c := 64;
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