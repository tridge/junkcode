#include <stdio.h>

/****************************************************************************
interpret the weird netbios "name"
****************************************************************************/
void name_interpret(char *in,char *out)
{

int len = (*in++) / 2;
while (len--)
  {
    *out = ((in[0]-'A')<<4) + (in[1]-'A');
    in += 2;
    out++;
  }
*out = 0;
/* Handle any scope names */
while(*in) 
  {
  *out++ = '.'; /* Scope names are separated by periods */
  len = *(unsigned char *)in++;
  strncpy(out, in, len);
  out += len;
  *out=0;
  in += len;
  }
}


main(int argc,char *argv[])
{
  char out[100];

  name_interpret(argv[1],out);
  puts(out);
}
