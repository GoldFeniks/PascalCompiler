program test;
var
    i: integer;
begin
    i := 3;
    repeat
    write('Once');
    i := i - 1;
    break;
    until i = 0;
    repeat
    write('Twice');
    i := i - 1;
    until i = 0;
end.