program test;
type
    arr = array [1..10] of integer;

function f(a: arr) : arr;
var
    b: integer = 10;
begin
    a[0] := b;
    exit(a);
end;

var 
    a: arr;

begin    
    f(a);
end.