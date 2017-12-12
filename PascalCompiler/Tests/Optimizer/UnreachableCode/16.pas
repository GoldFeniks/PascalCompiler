program test;
var 
    i: integer;
    j: integer;
begin
    j := 2;
    while i < j do
    begin
        write(i);
        i += 1;
        break;
    end;
end.