program test;

function f(a, b, c: Integer) : Real;
begin
    Result := (a + b) / c;
end;

var

    r: Real;

begin
    r := f();
end.