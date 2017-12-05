program test;
type
    arr = array [1..2] of array [0..1] of record 
        i: integer;
        j: real;
    end;

function sum(var a: arr) : real;
var
    i, j: integer;
begin
    result := 0;
    for i := 1 to 2 do
        for j := 0 to 1 do
        begin
            write('adding ', a[i][j].i, ' ...');
            result := result + a[i][j].i;
            a[i][j].i := a[i][j].i * 2;
            write('adding ', a[i][j].j, ' ...');
            result := result + a[i][j].j;
            a[i][j].j := a[i][j].j * 2;
        end;
end;

var
    a, b: arr;

begin
    a[1][0].i := 10;
    a[2][1].i := 20;
    a[1][0].j := 10.1;
    a[2][1].j := 20.1;
    a[2][0].i := 30;
    a[1][1].i := 40;
    a[2][0].j := 30.1;
    a[1][1].j := 40.1;
    b := a;
    write('the sum is ', sum(a));
    write('the sum is ', sum(a));
    write('the sum is ', sum(b));
end.