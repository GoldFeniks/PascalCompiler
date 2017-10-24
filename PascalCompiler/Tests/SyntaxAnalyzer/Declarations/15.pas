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

const

    a: MyInteger = 10;
    b: MyArray = (1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    c: MyRecord = (
        a: 10;
        b: (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
    );
    d:MyRecord2 = (
        a:  (
                a: 10;
                b: (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
        );
    );

begin
end.
