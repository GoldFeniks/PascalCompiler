program test;

procedure p(a: Integer);

function f : Integer;
begin
    Result := 10;
end;

begin
    a += f;
end;

function f(a, b, c: Integer) : Real;
begin
    Result := (a + b) / c;
end;

begin
end.