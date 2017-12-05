program test;

type
    arr = array [1..10] of integer;
    rec = record
        a: arr;
        s: integer;
    end;

function foo() : arr;
var
    i: integer;
begin
    for i := 1 to 10 do
        Result[i] := i;
end;

function foo1() : rec;
var
    i: integer;
begin
    for i := 11 to 20 do
        Result.a[i - 10] := i;
end;

var
    i: integer;

begin
    for i := 10 downto 1 do
        write(foo1().a[foo()[i]]);
end.