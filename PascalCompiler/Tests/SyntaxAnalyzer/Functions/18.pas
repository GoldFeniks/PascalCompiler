program test;

type

    rec = record
        a: array [1..2] of Integer;
    end;

function f(g: Integer = 10; a, b, c: Real) : rec;
begin
    Result.a[1] := 10;
end;

var
    a: Integer;

begin
    a := f(1.0, 10.1, 3).a[1];
end.