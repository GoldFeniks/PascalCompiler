program test;
var

    a, b: array [100..101] of 
    record
        i: integer;
        r: real;
        c: char;
    end;

begin

    a[100].i := 42;
    a[100].r := 123.10e2;
    a[100].c := 'f';

    a[101].c := char(a[100].i);
    a[101].i := integer(a[100].r);
    a[101].r := real(a[100].c);

    b := a;

    write(a[100].r, ' == ', b[100].r);
    write(a[101].r, ' == ', b[101].r);
    write(a[100].c, ' == ', b[100].c);
    write(a[101].c, ' == ', b[101].c);
    write(a[100].i, ' == ', b[100].i);
    write(a[101].i, ' == ', b[101].i);

end.