program test;

type

    rec = record
        a: array [1..2] of Integer;
    end;

function f(g: Integer = 10; a: Real = 10) : rec;
begin
    Result.a[1] := 10;
end;

var
    a: Integer;

begin
    a := f.a[1];
end.