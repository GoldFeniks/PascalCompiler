program test;

type
    arr = array [1..10] of integer;
    rec = record
        a: arr;
        s: integer;
    end;

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

function pick_if_divides_store(a: arr; b: integer) : rec;
var
    i: integer;
begin
    Result.a := pick_if_divides(a, b);
    Result.s := 0;
    for i := 1 to 10 do
    if a[i] <> 0 then
        Result.s := Result.s + a[i];
end;

function add_multiply(a: arr; b: integer; d: integer) : arr;
var
    i: integer;
begin
    for i := 1 to 10 do
    begin
        a[i] := a[i] + b;
        a[i] := a[i] * d;
    end;
    exit(a);
end;

var
    a: arr;
    i: integer;

begin
    for i := 1 to 10 do
        a[i] := i;
    a := add_multiply(pick_if_divides_store(a, 2).a, pick_if_divides(a, 2)[5], 123);
    for i := 1 to 10 do
        write(a[i]);
end.