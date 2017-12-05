program test;

procedure print_sum(a, b: integer);
begin
    write('the sum of ', a, ' and ', b, ' is ', a + b);
end;

var 

    a, b: integer;
    c: array [1..10] of integer;
    d: record
        i: integer;
        j: integer;
    end;

begin
    a := 6;
    b := 7;
    c[5] := 23;
    c[3] := 1;
    d.i := 10;
    d.j := -100;
    print_sum(6, 7);
    print_sum(a, b);
    print_sum(c[5], c[3]);
    print_sum(d.i, d.j);
end.