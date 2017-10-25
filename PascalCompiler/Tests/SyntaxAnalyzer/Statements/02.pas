program test;
var
    a: Integer;
begin
    if 10 < 15 then
    begin
        a := 10;
        write('Less', a)
    end
    else
    begin
        a := 15;
        write('Greater', a);
    end;
end.