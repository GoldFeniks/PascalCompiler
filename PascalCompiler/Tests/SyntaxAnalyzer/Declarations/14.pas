program test;
type

    MyInteger = Integer;
    MyArray = array [1..10] of MyInteger;
    MyRecord = record 
        a: MyInteger;
        b: MyArray;
    end;
    MyRecord2 = record
        a: MyRecord;
    end;

var
    
    a1: MyRecord;
    a2: MyRecord2;

begin
end. 