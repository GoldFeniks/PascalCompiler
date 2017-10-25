program test;
type

    arr = array [1..10] of Integer;

    rec = record
        a: Integer;
        b: arr;
    end;

    arr1 = array [1..10] of rec;

    arr2 = array [1..10] of array [1..10] of rec;

    rec1 = record
        a: arr2;
    end;

var

    a: arr;
    b: rec;
    c: arr1;
    d : arr2;
    f : rec1;

begin

    b.a := 10;
    b.b := a;
    a := b.b;

    c[1] := b;

    c[1].b := a;

    d[1][2] := b;

    d[2][4].b := a;

    b.b := d[2][4].b;

    f.a[2][10] := b

end.