program test;
const
    a: record
        a: Integer;

        b: array [1..10] of real;

        c: record
            c: char;
        end
    end = (
        a : 10;
        b : (1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        c : (
            c: 'c';
        );
    );

begin
end.