type
   TPoint = record
      x, y: integer;
   end;
   
const
   THE_END = false;
var
   a: integer;
label LOH:

   function isEmptyStr(a: string);
   begin
      exit;
      result := a = '';
      goto LOH;
   end;