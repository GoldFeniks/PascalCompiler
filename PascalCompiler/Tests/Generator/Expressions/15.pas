program test;
var 
    a: integer = 10;
    b: integer = 20;
    c: integer;
begin
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