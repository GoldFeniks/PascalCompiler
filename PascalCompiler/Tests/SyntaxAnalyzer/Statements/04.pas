program test;
var
    a: Integer;
begin
    if a < 15 then
        a := 10;
        write('Less', a)
    else
    begin
        a := 15;
        write('Greater', a);
    end;
end.