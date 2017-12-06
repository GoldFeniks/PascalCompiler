program test;
var
    a: array [1..6] of char;
begin
    a[1] := 'c';
    a[2] := 'a';
    a[3] := 't';
    a[4] := 't';
    a[5] := 't';
    a[6] := 'o';
    write(a[1], a[2], a[3], a[4], a[5], a[6]);
end.