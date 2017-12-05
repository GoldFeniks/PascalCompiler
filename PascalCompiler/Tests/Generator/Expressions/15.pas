program test;
var 
    a, b, c: integer;
begin
    a := 10;
    b := 20;
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