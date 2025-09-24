// EXPECTED OUTPUT: 6

void print_number (uint64_t number) 
{
	uint64_t digit;
	uint64_t divisor;
	uint64_t *d;

	d = malloc (sizeof (uint64_t));
	divisor = 1;

	while (number / divisor >= 10) {
		divisor = divisor * 10;
	}

	while (divisor > 0) {
		digit = number / divisor; // most significant digit
		number = number % divisor; // less significant digits
		divisor = divisor / 10;

		*d = 48 + digit; // 48 = ASCII('0')
		write(1, d, 1); // write the digit in console
	}
}

int search(uint64_t* arr, uint64_t start, uint64_t end, uint64_t target) 
{
    uint64_t j;
    j = start;
    while (j <= end) {
        if (*(arr + j) == target) {
            return j;
        }
        j = j + 1;
    }
    return -1;
}


int main() 
{
    uint64_t i;
    uint64_t n;
    uint64_t target;
    uint64_t* arr;
    uint64_t result;
    uint64_t* number_of_called_syscalls;

    n = 10;
    target = 17;
    arr = malloc(n * sizeof(uint64_t));

    i = 0;
    while (i < n) {
        *(arr + i) = 1 + i * 2;
        i = i + 1;
    }

    result = search(arr, 0, n, target);
    print_number(result);
    number_of_called_syscalls = count_syscalls();
	exit(number_of_called_syscalls);
}