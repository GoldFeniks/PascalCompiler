program test;
var

    a: array [1..10] of integer;
    b: record
        i: integer;
        r: real;
        c: char;
    end;
    i, j: integer;

begin
    i := 2;
    j := 5;
    b.i := 3;
    b.r := 1.5;
    b.c := #10;
    a[b.i] := 318;
    a[integer(b.r)] := 221;
    a[integer(b.c)] := -9033;
    write(a[b.i], b.c, a[integer(b.r)], b.c, a[integer(b.c)]);
end.