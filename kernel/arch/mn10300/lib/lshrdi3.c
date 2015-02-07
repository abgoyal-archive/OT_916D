
/* lshrdi3.c extracted from gcc-2.7.2/libgcc2.c which is: */

#define BITS_PER_UNIT 8

typedef 	 int SItype	__attribute__((mode(SI)));
typedef unsigned int USItype	__attribute__((mode(SI)));
typedef		 int DItype	__attribute__((mode(DI)));
typedef		 int word_type	__attribute__((mode(__word__)));

struct DIstruct {
	SItype	low;
	SItype	high;
};

union DIunion {
	struct DIstruct	s;
	DItype		ll;
};

DItype __lshrdi3(DItype u, word_type b)
{
	union DIunion w;
	word_type bm;
	union DIunion uu;

	if (b == 0)
		return u;

	uu.ll = u;

	bm = (sizeof(SItype) * BITS_PER_UNIT) - b;
	if (bm <= 0) {
		w.s.high = 0;
		w.s.low = (USItype) uu.s.high >> -bm;
	} else {
		USItype carries = (USItype) uu.s.high << bm;
		w.s.high = (USItype) uu.s.high >> b;
		w.s.low = ((USItype) uu.s.low >> b) | carries;
	}

	return w.ll;
}
