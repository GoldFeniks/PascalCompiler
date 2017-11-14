program test;
var 
    r: real = 123.45;
    i: integer = 42;
begin
    r := -r;
    i := -i;
    write(r, #10, i);
end.