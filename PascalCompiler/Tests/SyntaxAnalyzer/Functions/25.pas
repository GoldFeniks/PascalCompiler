program test;
type
    arr = array [1..10] of integer;

function f(var a, b: integer;) : arr;
begin
    if (a < b) then
        exit(a)
    else
        exit(b);
end;

begin    
end.