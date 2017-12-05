program test;
var

    a: array [1..10] of integer;
    i, j: integer;

begin
    i := 2;
    j := 5;
    a[i * 2] := 10;
    a[(i - 1) * j] := 5;
    a[i] := 2;
    a[j mod i] := 1;
    write(a[i * 2], #10, a[(i - 1) * j], #10, a[i], #10, a[j mod i]);
end.