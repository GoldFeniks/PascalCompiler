program test;
var
    i: integer;
begin
    i := -10;
    repeat
    write(i);
    i -= 1;
    until i < 0;
end.