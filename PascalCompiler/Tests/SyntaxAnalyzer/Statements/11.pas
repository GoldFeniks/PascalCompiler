program test;
var
    a, b: Real;
    c: Integer;
begin
    repeat
        a := b + c;
        c += 1;
    until c < a;
end.