program test;
var 
    i: integer;
    j: integer;
begin
    j := 2;
    for i := 1 to j do
    begin
        write(j);
        break;
    end;
end.