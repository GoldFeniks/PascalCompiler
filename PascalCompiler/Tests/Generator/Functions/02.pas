program test;

function sum(a, b: real) : real;
begin
    Result := a + b;
end;

var 

    a, b: real;
    c: array [1..10] of real;
    d: record
        i: real;
        j: real;
    end;

begin
    a := 6.5;
    b := 7.5;
    c[5] := 23.5;
    c[3] := 1.5;
    d.i := 10.5;
    d.j := -100.5;
    write(sum(6.5, 7.5));
    write(sum(a, b));
    write(sum(c[5], c[3]));
    write(sum(d.i, d.j));
end.