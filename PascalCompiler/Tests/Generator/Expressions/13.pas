program test;
var 
    a, b, c, d: real;
begin
    a := 10.5;
    b := 20.5;
    c := 2;
    d := a + b;
    write(d);
    d := a - b;
    write(d);
    d := a / c;
    write(d);
    d := b * c;
    write(d);
    d := -d;
    write(d);
end.