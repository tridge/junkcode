#include <EXTERN.h>
#include <perl.h>
   
static PerlInterpreter *my_perl;
   
main (int argc, char **argv, char **env)
{
	STRLEN n_a;
	char *embedding[] = { "", "-e", "0" };
   
	my_perl = perl_alloc();
	perl_construct( my_perl );
   
	perl_parse(my_perl, NULL, 3, embedding, NULL);
	perl_run(my_perl);
   
	/** Treat $a as an integer **/
	perl_eval_pv("$a = 3; $a **= 2", TRUE);
	printf("a = %d\n", SvIV(perl_get_sv("a", FALSE)));
   
	/** Treat $a as a float **/
	perl_eval_pv("$a = 3.14; $a **= 2", TRUE);
	printf("a = %f\n", SvNV(perl_get_sv("a", FALSE)));
   
	/** Treat $a as a string **/
	perl_eval_pv("$a = 'rekcaH lreP rehtonA tsuJ'; $a = reverse($a);", TRUE);
	printf("a = %s\n", SvPV(perl_get_sv("a", FALSE), n_a));
   
	perl_destruct(my_perl);
	perl_free(my_perl);
}

