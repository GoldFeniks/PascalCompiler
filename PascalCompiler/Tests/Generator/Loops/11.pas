program test;
var
    a: array [1..2] of array [1..3] of array [1..4] of Real;
    i, j, k: integer;
begin
    for i := 1 to 2 do
    begin
        j := 1;
        while j <= 3 do
        begin
            k := 1;
            repeat
            a[i][j][k] := i * j * k;
            k += 1;
            until k > 4;
            j += 1;
        end;
    end;
    write(a[1][2][3]);
    write(a[2][3][4]);
end.