program test;
type
    arr = array [1..10] of integer;

function f(var a: integer = 10) : arr;
var
    b: integer = 10;
begin
    if (a < b) then
        exit(a)
    else
        exit(b);
end;

begin    
end.