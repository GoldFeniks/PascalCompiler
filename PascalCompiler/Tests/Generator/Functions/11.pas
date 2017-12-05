program test;

type
    arr = array [1..10] of integer;

function pick_if_divides(a: arr; m: integer) : arr;
var
    i: integer;
    j: integer;
begin
    j := 1;
    for i := 1 to 10 do
    begin
        Result[i] := 0;
        if (a[i] mod m) = 0 then
        begin
            Result[j] := a[i];
            j := j + 1;
        end;
    end;
end;

var
    a: arr;
    i: integer;

begin
    for i := 1 to 10 do
        a[i] := i;
    a := pick_if_divides(a, 2);
    for i := 1 to 10 do
        write(a[i]);
end.