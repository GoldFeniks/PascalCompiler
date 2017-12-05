program test;

procedure print_biggest(a, b: integer);
begin
    if a > b then
    begin
        write(a);
        exit;
    end;
    if a = b then
    begin
        write('a is equal to b');
        exit;
    end;
    write(b);
end;

function max(a, b: real) : real;
begin
    if a > b then
        exit(a);
    exit(b);
end;

var
    a: real;

begin
    print_biggest(10, 20);
    print_biggest(15, 15);
    print_biggest(20, 10);
    a := max(10, 20);
    write(a);
    a := max(20, 10);
    write(a);
end.