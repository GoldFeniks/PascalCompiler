program test;
var
    a: integer;
    b: real;
    c: char;
begin
    a := integer(b);
    c := char(b);
    c := char(a);
    b := real(c);
end.