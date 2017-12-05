program test;
var 
    r: real;
begin
    r := 48.5;
    write('real to real ', real(r), #10, 'real to int ', integer(r), #10, 'real to char ', char(r));
end.