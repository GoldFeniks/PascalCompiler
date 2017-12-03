program test;
type
    arr = array [1..2] of integer;

function sum(a: arr) : integer;
var
    i: integer;
begin
    Result := 0;
    for i := 1 to 2 do
        Result += a[i];
end;

var
    b: arr;

begin
    b[1] := 10;
    b[2] := 20;
    write(sum(b));
end.