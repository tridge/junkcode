main()
{
  unsigned end = 0xf0001000;
  unsigned start = 0xf0000000;
  unsigned limit = 128*1024;
  int size;

  size = start - end;
  
  printf("%d %x\n",size,size);

  if (size > limit)
    printf("yes\n");
  else
    printf("no\n");    
}
