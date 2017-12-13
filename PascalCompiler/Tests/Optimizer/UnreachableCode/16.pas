program test;
var 
    i: integer;
    j: integer;
begin
    j := 2;
    i := 0;
    while i < j do
    begin
        write(i);
        i += 1;
        break;
    end;
end.