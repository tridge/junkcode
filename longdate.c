#define TIME_FIXUP_CONSTANT1 (369.0*365.25*24*60*60-(3.0*24*60*60+6.0*60*60))
#define TIME_FIXUP_CONSTANT2 (TIME_FIXUP_CONSTANT1)

/****************************************************************************
interpret an 8 byte "filetime" structure to a time_t
It's originally in "100ns units since jan 1st 1601"

It appears to be kludge-GMT (at least for file listings). This means
its the GMT you get by taking a localtime and adding the
serverzone. This is NOT the same as GMT in some cases. This routine
converts this to real GMT.
****************************************************************************/
time_t interpret_long_date(char *p)
{
  double d;
  time_t ret;
  uint32 tlow,thigh;
  tlow = IVAL(p,0);
  thigh = IVAL(p,4);

  if (thigh == 0) return(0);

  d = ((double)thigh)*4.0*(double)(1<<30);
  d += (tlow&0xFFF00000);
  d *= 1.0e-7;
 
  /* now adjust by 369 years to make the secs since 1970 */
  d -= TIME_FIXUP_CONSTANT1;

  if (!(TIME_T_MIN <= d && d <= TIME_T_MAX))
    return(0);

  ret = (time_t)(d+0.5);

  /* this takes us from kludge-GMT to real GMT */
  ret -= serverzone;
  ret += LocTimeDiff(ret);

  return(ret);
}


/****************************************************************************
put a 8 byte filetime from a time_t
This takes real GMT as input and converts to kludge-GMT
****************************************************************************/
void put_long_date(char *p,time_t t)
{
  uint32 tlow,thigh;
  double d;

  if (t==0) {
    SIVAL(p,0,0); SIVAL(p,4,0);
    return;
  }

  /* this converts GMT to kludge-GMT */
  t -= LocTimeDiff(t) - serverzone; 

  d = (double) (t);

  d += TIME_FIXUP_CONSTANT2;

  d *= 1.0e7;

  thigh = (uint32)(d * (1.0/(4.0*(double)(1<<30))));
  tlow = (uint32)(d - ((double)thigh)*4.0*(double)(1<<30));

  SIVAL(p,0,tlow);
  SIVAL(p,4,thigh);
}

int main(int argc, char *argv[])
{

}
