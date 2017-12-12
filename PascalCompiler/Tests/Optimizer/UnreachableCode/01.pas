program test;
var
    i: integer;
begin
    for i := 1 to 10 do
        write('1: ', i);
    for i := 10 to 1 do
        write('2: ', i);
    for i := 1 downto 10 do
        write('3: ', i);
    for i := 10 downto 1 do
        write('4: ', i);
end.