program test;
var
    a, b, c, d, e: integer;
    r: real;
begin
    a := 5;
    b := 2;
    c := 13;
    d := 5;
    e := 1;
    r := a << b / b * b + c mod d div e;
    write(r);
end.