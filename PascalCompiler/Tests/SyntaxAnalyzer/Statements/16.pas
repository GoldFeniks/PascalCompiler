program test;
type
    rec = record
        a :Integer;
        b : Real;
    end;

    arr = array [1..10] of Integer;

var

    r, d: rec;
    a, b: arr;
    c, f: Integer;

begin
    r.a := c;
    r.a := 10;
    r.b := 10.1;
    r := d;

    a[1] := 10;
    a[2] := 20;
    a[3] := c + f;
    a[4] := r.a;    
end.