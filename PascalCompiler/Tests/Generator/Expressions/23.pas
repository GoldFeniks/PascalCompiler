program test;
var 
    r: real;
    i: integer;
begin
    r := 123.45;
    i := 42;
    r := -r;
    i := -i;
    write(r, #10, i);
end.