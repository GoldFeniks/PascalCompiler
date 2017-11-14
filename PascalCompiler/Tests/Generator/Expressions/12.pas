program test;
var
    a: integer = 5;
    b: integer = 2;
    c: integer = 13;
    d: integer = 5;
    e: integer = 1;
    r: real;
begin
    r := a << b / b * b + c mod d div e;
    write(r);
end.