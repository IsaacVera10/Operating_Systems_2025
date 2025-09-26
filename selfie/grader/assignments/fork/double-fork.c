void print_number(uint64_t number) 
{
    uint64_t digit;
    uint64_t divisor;
    uint64_t *d;

    d = malloc(sizeof(uint64_t)); 
    divisor = 1;

    while (number / divisor >= 10) 
    {
        divisor = divisor * 10;
    }

    while (divisor > 0) 
    {
        digit = number / divisor; // most significant digit
        number = number % divisor; // less significant digits
        divisor = divisor / 10;

        *d = 48 + digit; // 48 = ASCII('0')
        write(1, d, 1);   // write the digit in console
    }
}

int main(int argc, char** argv) {
    uint64_t id;
    
    id = fork(); 
    /*
        returns:
            0 -> for the child
            1 -> for the parent
    */

    id = fork();
    /*
        returns:
            For process with id = 1:
                0 -> for the new child
                2 -> for the process with id=1

            For process with id = 0:
                0 -> for the new child 
                3 -> for the process with id=0
    */

    print_number(id);
}