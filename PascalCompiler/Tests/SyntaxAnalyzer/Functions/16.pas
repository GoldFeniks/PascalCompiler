program test;

type

    rec = record
        a: Real;
    end;

function f(a, b, c: Real; d: rec) : rec;
begin
    d.a := (a + b) / c;
    Result := d;
end;

var
    r: rec;

begin
    r := f(1.0, 10.1, 3, r);
end.