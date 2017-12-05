program test;
var 
    i: integer;
begin
    i := 98;
    write('int to real ', real(i), #10, 'int to int ', integer(i), #10, 'int to char ', char(i));
end.