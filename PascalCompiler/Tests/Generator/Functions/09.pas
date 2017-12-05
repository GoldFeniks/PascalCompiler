program test;

function factorial(a: integer): integer;
begin
    if a < 0 then
    begin
        write('Wrong input');
        Result := -1;
    end
    else if a = 0 then
        Result := 1
    else 
        Result := a * factorial(a - 1);
end;

begin
    write(factorial(-1));
    write(factorial(8));
end.