program test;

type

    arr = array [1..10] of real;


function sum(a: arr) : real;
var
    i: integer;
begin
    Result := 0;
    for i := 1 to 10 do
    begin   
        Result := Result + a[i];
        write(a[i]);
    end;
end;

var 

    a: arr;
    i: integer;

begin
    for i := 1 to 10 do
        a[i] := i * 2;
    write(sum(a));
end.