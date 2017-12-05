program test;

type

    arr = array [1..10] of integer;


procedure sum(var a: arr; var sum: integer);
var
    i: integer;
    temp: integer;
begin
    sum := 0;
    for i := 1 to 10 do
    begin
        temp := a[i];
        a[i] := sum;
        sum := sum + temp;
    end;
end;

var 

    a: arr;
    i, ssum: integer;

begin
    for i := 1 to 10 do
        a[i] := i * 2;
    sum(a, ssum);
    write(ssum);
    for i := 1 to 10 do
    begin
        write(a[i]);
    end;
end.