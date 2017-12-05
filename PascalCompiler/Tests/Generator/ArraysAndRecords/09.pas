program test;
var

    a, c: array [1..10] of integer;
    b: record
        i: integer;
        r: real;
        c: char;
    end;
    i, j: integer;

begin
    i := 2;
    j := 5;
    b.i := 10;
    b.r := 1.5;
    b.c := #10;
    a[b.i] := 318;
    a[integer(b.r)] := 221;
    a[integer(b.c)] := -9033;
    c := a;
    c[5] := 6;
    c[7] := 8;
    c[9] := -10;
    write(c[5], ' ', c[7], ' ', c[9]);
end.