program test;
var
    i, j: integer;

    procedure f;
    var
        r, r1: real;

        procedure g;
        begin
            write('i from main ', i, #10'j from main ', j);
            write('r from h ', r, #10'r1 from h ', r1);
        end;

        procedure h;
        var
            r, r1: real;

            procedure h1;
            var
                r, r1: real;
            begin
                r := 10.1;
                r1 := -10.1;
                write('i from main ', i, #10'j from main ', j);
                write('r from h1 ', r, #10'r1 from h1 ', r1);
            end;

            procedure h2;
            begin
                write('i from main ', i, #10'j from main ', j);
                write('r from h ', r, #10'r1 from h ', r1);
            end;

        begin
            r := i * 2;
            r1 := r + j * 2;
            write('i from main ', i, #10'j from main ', j);
            write('r from h ', r, #10'r1 from h ', r1);
            h1;
            h2;
        end;

    begin
        r := i * j;
        r1 := i + j;
        write('i from main ', i, #10'j from main ', j);
        write('r from f ', r, #10'r1 from f ', r1);
        g;
        h;
    end;

begin
    i := 2;
    j := 3;
    f;
end.