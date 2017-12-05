program test;
var 
    a, b: real;
    c: integer;
begin
    a := 10.5;
    b := 20.5;
    c := a < b;
    write(c);
    c := a > b;
    write(c);
    c := a <= b;
    write(c);
    c := a >= b;
    write(c);
    c := a <= b - 10;
    write(c);
    c := a + 11 >= b;
    write(c);
    c := a = b;
    write(c);
    c := a <> b;
    write(c);
    c := not (a = b);
    write(c);
end.