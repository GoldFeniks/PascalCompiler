program test;
var
    i: integer;

procedure f();
begin
    for i := 1 to 10 do
    begin
        write('Once');
        exit;
    end;
end;

begin
    f;
end.