/*
** Assumptions:
**
** + dst and src are 8-byte aligned.
** + Copying at least 8 bytes of data
*/

void
dlmemcpy(char *dst, char *src, int nbytes)
	{

	__asm__("
	pushal
	movl %0,%%edi
	movl %1,%%esi
	movl %2,%%eax
	movl %%eax,%%ecx
	and  $7, %%eax
	shr  $3, %%ecx
.align 32
1:
	fldl  (%%esi)
	fstpl (%%edi)
	add    $8,%%esi
	add    $8,%%edi
	dec    %%ecx
	jne    1b
2:
	movl %%eax,%%ecx
	rep
	movsb
	popal
	  " :: "g"(dst), "g"(src), "g"(nbytes));

	}
