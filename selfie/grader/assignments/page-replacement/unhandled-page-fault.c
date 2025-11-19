uint64_t size;

uint64_t main () {
	uint64_t *a;
	uint64_t *b;
	uint64_t *c;
	uint64_t i;
	uint64_t res;

	size = 258049;
	a = malloc (sizeof (uint64_t) * size);
	b = malloc (sizeof (uint64_t) * size);
	c = malloc (sizeof (uint64_t) * size);

	i = 0;
	while (i < size) {
		res = *(a + i);
		i = i + 1;
	}
}
