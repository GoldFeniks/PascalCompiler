program test;

type

    rec = record
        a: Real;
    end;

function f(a, b, c: Real) : rec;
begin
    Result.a := (a + b) / c;
end;

var
    r: rec;

begin
    r := f(1.0, 10.1, 3);
end.