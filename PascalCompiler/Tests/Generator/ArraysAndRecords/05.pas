program test;
var

    a, b: record 
        i: integer;
        arr: array [100..101] of 
        record
            i: integer;
            r: real;
            c: char;
        end;
        c: char;
    end;

begin

    a.c := #123;

    a.arr[100].i := 42;
    a.arr[100].r := 123.10e2;
    a.arr[100].c := 'f';

    a.arr[101].c := char(a.arr[100].i);
    a.arr[101].i := integer(a.arr[100].r);
    a.arr[101].r := real(a.arr[100].c);

    a.i := -123456789;

    b := a;

    write(a.i, ' == ', b.i);
    write(a.arr[100].r, ' == ', b.arr[100].r);
    write(a.arr[101].r, ' == ', b.arr[101].r);
    write(a.arr[100].c, ' == ', b.arr[100].c);
    write(a.arr[101].c, ' == ', b.arr[101].c);
    write(a.arr[100].i, ' == ', b.arr[100].i);
    write(a.arr[101].i, ' == ', b.arr[101].i);
    write(a.c, ' == ', b.c);

end.