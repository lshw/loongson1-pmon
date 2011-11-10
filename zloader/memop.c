void *memcpy(void *s1, const void *s2, size_t n)
{
	const char *f = s2;
	char *t = s1;

	if (f < t) {
		f += n;
		t += n;
		while (n-- > 0)
			*--t = *--f;
	} else
		while (n-- > 0)
			*t++ = *f++;
	return s1;
}

static void * memset(void * s,int c, size_t count)
{
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}

