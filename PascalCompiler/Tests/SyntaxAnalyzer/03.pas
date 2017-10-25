program test;

type
    int = Integer;
    rec = record
        a: int;
    end;
    arr = array [1..10] of rec;

const 
    
    c = 10;

    cr: rec = (
        a: 10
    );

var

    a: Integer = 0;
    b: Real = 1;
    e, d: Char;
    r: rec;
    ar: arr;

begin
    read(e, d);
    while b <> 10 do
    begin
        for a := 0 downto 10 do
            repeat
                read(e);
                if e = 'r' then
                    write(d)
                else
                    begin
                        read(d);
                        write(c);
                        e := d;
                    end;
            until e <> 'e'; 
        r.a := a;
        ar[10] := r;
        write('I''m doing stuffs');
    end;
    write('END');
end.