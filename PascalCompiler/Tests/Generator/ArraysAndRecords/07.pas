program test;
var

    a: array [1..10] of integer;
    b: array [1..1000] of integer;
    i, j: integer;

begin
    i := 2;
    j := 5;
    a[i * 2] := 10;
    a[(i - 1) * j] := 5;
    a[i] := 2;
    a[j mod i] := 1;
    b[a[i * 2] * a[(i - 1) * j]] := 100;
    b[a[i] * a[j mod i] * 100] := -100;
    write(b[a[i * 2] * a[(i - 1) * j]], ' ', b[a[i] * a[j mod i] * 100]);
end.