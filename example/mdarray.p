mdarray;
var arr : array 1 to 10 of array 0 to 5 of integer;

showOps();
begin
  print "Input a command\n";
  print "0 : terminate\n";
  print "1 i j k : set arr[i][j] to k\n";
  print "2 i j : get arr[i][j]\n";
  print "3 i j k : set arr[i][j] to k in function\n";
  print "4 i j : get arr[i][j] in function\n";
  print "5 : dump array\n";
end
end showOps

show(i,j,n:integer);
begin
  print "arr[";
  print i;
  print "][";
  print j;
  print "] = ";
  print n;
  print "\n";
end
end show

funop3(arr:array 1 to 10 of array 0 to 5 of integer; i,j,k:integer);
begin
  arr[i][j] := k;
end
end funop3

funop4(arr:array 1 to 10 of array 0 to 5 of integer; i,j:integer):integer;
begin
  return arr[i][j];
end
end funop4

dumparr();
begin
  print "dump array:\n";
  for i := 1 to 10 do
    for j := 0 to 5 do
      print arr[i][j];
      print " ";
    end do
    print "\n";
  end do
end
end dumparr

begin
  var op,i,j,k:integer;
  print "Md array test\n";
  print "array size: [1~10][0~5]\n";
  showOps();
  read op;
  while op <> 0 do
    if op > 5 or op < 0 then
      print "op not undefstood\n";
      showOps();
    else
      if op = 1 then
        read i; read j; read k; arr[i][j] := k;
      end if
      if op = 2 then
        read i; read j; show(i,j,arr[i][j]);
      end if
      if op = 3 then
        read i; read j; read k; funop3(arr,i,j,k);
      end if
      if op = 4 then
        read i; read j; show(i,j,funop4(arr,i,j));
      end if
      if op = 5 then dumparr(); end if
    end if
    read op;
  end do
end
end mdarray
