program test;
var
    i, j, k: integer;
begin
    for i := 1 to 3 do
        for j := 1 to 3 do
            for k := 3 downto 1 do
                if (i > j) and (i > k) then
                    write('i is the biggest ', i)
                else if (i < j) and (k < j) then
                    write('j is the biggest ', j)
                else if (k > j) and (i < k) then
                    write('k is the biggest ', k)
                else 
                    write('can''t decide')
end.