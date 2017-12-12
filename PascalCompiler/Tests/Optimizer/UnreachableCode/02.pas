program test;
var
    i: integer;
begin
    i := 1;
    while 10 < 1 do
    begin
        write('Unreacheable');
        i := 10;
        i := i + 1;
    end;
    while i < 10 do
    begin
        write('Reachable');
        i := i + 1;
    end;
end.