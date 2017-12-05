program test;
var
    i: integer;
begin
    i := 10;
    while i >= 0 do
    begin
        i := i - 1;
        if i mod 2 = 0 then
            continue;
        write(i);
    end;
end.