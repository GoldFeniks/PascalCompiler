program test;
var
    a: array [1..2] of record
        a: array [1..2] of array [1..2] of Integer;
    end = (
        (
            a: ( (1, 2), (3, 4) );
        ),
        (
            a: ( (1.0, 2.0), (3.0, 4.0) );
        )
    );
begin
end.