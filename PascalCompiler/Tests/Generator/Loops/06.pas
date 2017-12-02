program test;
var
    i: integer;
begin
    for i := 10 downto 1 do
    begin
        if i >= 5 then
            continue
        else if i >= 2 then
            write(i)
        else
            break;
    end;
end.