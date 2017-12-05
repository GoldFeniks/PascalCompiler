program test;

type

    arr = array [1..10] of integer;


procedure sum(a: arr; var sum: integer);
var
    i: integer;
begin
    sum := 0;
    for i := 1 to 10 do
        sum := sum + a[i];
end;

var 

    a: arr;
    i, ssum: integer;

begin
    for i := 1 to 10 do
        a[i] := i * 2;
    sum(a, ssum);
    write(ssum);
end.