void print_number(uint64_t number){
    uint64_t digit;
    uint64_t divisor;
    uint64_t *d;

    d = malloc(sizeof(uint64_t));
    
    divisor = 1;

    while(number/divisor >=10){
        divisor = divisor * 10;
    }

    while(divisor > 0){
        digit = number / divisor;
        number = number  % divisor;
        divisor = divisor / 10;
        
        *d = digit + 48;
        write(1, d, 1);
    }
}

int main(int argc, char** argv) {
    uint64_t x;
    x = dummy_syscall(20);
    print_number(x);
    return x;
}