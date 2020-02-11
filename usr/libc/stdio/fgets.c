#include <stdio_impl.h>
#include <string.h>

#define MIN(a,b) ((a)<(b) ? (a) : (b))

char *fgets(char *restrict s, int n, FILE *restrict f)
{
	char *p = s;
	unsigned char *z;
	size_t k;
	int c;

	FLOCK(f);

	if (n--<=1) {
		f->mode |= f->mode-1;
		FUNLOCK(f);
		if (n) return 0;
		*s = 0;
		return s;
	}

	while (n) {
		z = memchr(f->rpos, '\n', f->rend - f->rpos);
		k = z ? z - f->rpos + 1 : f->rend - f->rpos;
		k = MIN(k, n);
		memcpy(p, f->rpos, k);
		f->rpos += k;
		p += k;
		n -= k;
		if (z || !n) break;
		if ((c = getc_unlocked(f)) < 0) {
			if (p==s || !feof(f)) s = 0;
			break;
		}

		/* (SO3) Immediately return the echo */
		if (f == stdin) {
			if ((c == '\n') || (c == '\r'))
				putchar('\n');
			else if (c == 127) { /* Delete */
				if (p != s) {  /* Start of line ? */
					printf("\b \b");
					fflush(stdout);
					p--;
					n++;
				}
				continue;
			} else if (c == 3) { /* ctrl/c handling */
				*s = 0;
				printf("^C\n");
				return s;
			} else
				putchar(c);
		}
		n--;

		if (((*p = c) == '\n') || ((*p = c) == '\r')) break;
		p++;
	}
	if (s) *p = 0;

	FUNLOCK(f);

	return s;
}

weak_alias(fgets, fgets_unlocked);
