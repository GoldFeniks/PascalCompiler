program test;
var
    a: record 
        i: integer;
        j: integer;
        r: real;
        c: char;
    end;

    b: record
        c: char;
        s: char;
        i: integer;
        k: integer;
        j: integer;
    end;

begin
    a.i := 10;
    b.c := 'c';
    b.s := 's';
    b.i := 100;
    a.r := 42.42;
    b.j := -10;
    a.j := -100;
    b.k := 42;
    a.c := 'd';
    write(a.c, ' ', a.r, ' ', a.i, ' ', b.c, ' ', b.i, ' ', b.s, ' ', a.j, ' ' , b.k, ' ', a.c, ' ');
end.