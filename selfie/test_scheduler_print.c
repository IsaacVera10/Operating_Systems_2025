// Test para visualizar el comportamiento del scheduler
// Imprime mensajes identificando cada proceso

uint64_t* string;

uint64_t main() {
  uint64_t pid1;
  uint64_t pid2;
  uint64_t pid3;
  uint64_t i;

  string = "Parent process starting\n";
  write(1, string, 24);

  pid1 = fork();
  
  if (pid1 == 0) {
    string = "Process 1 executing\n";
    write(1, string, 20);
    
    i = 0;
    while (i < 3) {
      i = i + 1;
    }
    
    string = "Process 1 done\n";
    write(1, string, 15);
    exit(0);
  }
  
  pid2 = fork();
  
  if (pid2 == 0) {
    string = "Process 2 executing\n";
    write(1, string, 20);
    
    i = 0;
    while (i < 3) {
      i = i + 1;
    }
    
    string = "Process 2 done\n";
    write(1, string, 15);
    exit(0);
  }
  
  pid3 = fork();
  
  if (pid3 == 0) {
    string = "Process 3 executing\n";
    write(1, string, 20);
    
    i = 0;
    while (i < 3) {
      i = i + 1;
    }
    
    string = "Process 3 done\n";
    write(1, string, 15);
    exit(0);
  }
  
  string = "Parent waiting\n";
  write(1, string, 15);
  
  i = 0;
  while (i < 5) {
    i = i + 1;
  }
  
  string = "Parent done\n";
  write(1, string, 12);
  
  return 0;
}
