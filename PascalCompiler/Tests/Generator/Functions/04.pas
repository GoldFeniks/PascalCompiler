program test;

type

    arr = array [1..10] of record
                        max: integer;
                        min: integer;
                    end;


function prod(a: arr) : real;
var
    i: integer;
begin
    Result := 1;
    for i := 1 to 10 do
    begin
        Result := Result * (a[i].max - a[i].min);
        write(a[i].max, ' ', a[i].min);
    end;
end;

var 

    a: arr;
    i: integer;

begin
    for i := 1 to 10 do
    begin
        a[i].max := i * 2;
        a[i].min := i * 2 - 10;
    end;
    write(prod(a));
end.