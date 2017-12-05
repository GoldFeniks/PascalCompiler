program test;
var
    i: integer;
begin
    for i := 1 to 10 do
    begin
        if (i > 5) and (i <= 7) then
            continue;
        write(i);
    end;
end.