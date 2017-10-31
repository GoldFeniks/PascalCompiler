program test;
type
    MyInteger = integer;
    MyChar = char;
    MyReal = real;
var
    a: MyInteger;
    b: myreal;
    c: mychar;
begin
    a := myinteger(b);
    c := mychar(b);
    c := mychar(a);
    b := myreal(c);
end.