program test;
type
    rec = record
        a: integer;
    end;
    arr = array [1..10] of rec;

function f(a: rec) : arr;
var
    b: integer = 10;
    t: arr;
begin
    a.a := b;
    t[0] := a;
    result := t;
end;

var 
    a: arr;

begin    
    f(a[0]);
end.