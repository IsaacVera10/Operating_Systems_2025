// EXPECTED OUTPUT: 3

uint64_t count_syscalls();

int main(int argc, char** argv) 
{
    uint64_t number_of_called_syscalls;   
    
    number_of_called_syscalls = count_syscalls();
    exit (number_of_called_syscalls);
}