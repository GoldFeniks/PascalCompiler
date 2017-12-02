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
    while i <= 2 do
    begin
        b[i] := a[i];
        write(i);
        i := i + 1;
    end;
    write(b[1].i);
    write(b[2].j)
end.