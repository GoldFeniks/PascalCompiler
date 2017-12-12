program test;
var
    i: integer;

function foo() : integer;
begin
    exit(10);
end;

procedure f();
begin
    write(foo);
end;

begin
    f;
end.