program test;
var
    a, b: array [1..2] of record
        i: integer;
        j: real;
    end;
    i: integer = 1;
begin
    a[1].j := 10.1;
    a[1].i := 10;
    a[2].j := 20.1;
    a[2].i := 20;
    repeat
    b[i] := a[i];
    i := i + 1;
    until i >= 3;
    write(b[1].i);
    write(b[2].j)
end.