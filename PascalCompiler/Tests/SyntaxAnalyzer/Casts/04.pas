program test;
type
    a = array [1..10] of Integer;
var
    b: array [1..10] of Integer;
    c: a;
begin
    c := a(b);
end.