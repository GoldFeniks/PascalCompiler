program test;
type
    arr = array [1..10] of integer;

function f(const a: integer = 10) : arr;
var
    b: integer = 10;
begin
    a += b;
end;

begin    
end.