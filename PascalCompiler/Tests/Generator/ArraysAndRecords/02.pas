program test;
var
    a: array [1..3] of integer;
    b: array [-1..1] of real;
    c: array [0..1] of char;
begin
    a[1] := 10;
    a[3] := -100;
    b[-1] := 123.45;
    b[0] := -123.45;
    c[1] := 'c';
    c[0] := 'b';
    write(c[0], ' ', b[0], ' ', a[3], ' ', c[1], ' ', b[-1], ' ', a[3], ' ', b[-1], ' ', c[0], ' ', a[1]);
end.