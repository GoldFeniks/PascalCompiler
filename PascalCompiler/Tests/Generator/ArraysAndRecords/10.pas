program test;
type
    t = array [1..3] of record
                i: integer;
            end;
var 
    a, b: record
        i: integer;
        b: array [0..1] of real;
        c: array [0..0] of record 
            j: integer;
            r: real;
            a: t;
        end;
    end;
    test: t;
begin
    a.i := 10;
    a.b[0] := 10.1;
    a.b[1] := 20.1;
    a.c[0].j := 20;
    a.c[0].r := 30.1;
    a.c[0].a[1].i := 30;
    a.c[0].a[2].i := 40;
    a.c[0].a[3].i := 50;
    b := a;
    write(b.b[1]);
    write(b.b[0]);
    write(b.i);
    write(b.c[0].j);
    write(b.c[0].a[1].i);
    write(b.c[0].r);
    write(b.c[0].a[3].i);
    write(b.c[0].a[2].i);
    test := b.c[0].a;
    write(test[2].i);
    write(b.b[0] + b.c[0].a[2].i + test[2].i);
end.