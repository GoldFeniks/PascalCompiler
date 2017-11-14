program test;
var 
    c: char = 'z';
begin
    write('char to real ', real(c), #10, 'char to int ', integer(c), #10, 'char to char ', char(c));
end.