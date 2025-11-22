// Programa de prueba para el scheduler
// Crea múltiples procesos y verifica que se ejecutan

uint64_t main() {
  uint64_t pid1;
  uint64_t pid2;
  uint64_t pid3;
  uint64_t i;

  pid1 = fork();
  
  if (pid1 == 0) {
    // Proceso hijo 1
    i = 0;
    while (i < 5) {
      i = i + 1;
    }
    exit(0);
  }
  
  pid2 = fork();
  
  if (pid2 == 0) {
    // Proceso hijo 2
    i = 0;
    while (i < 5) {
      i = i + 1;
    }
    exit(0);
  }
  
  pid3 = fork();
  
  if (pid3 == 0) {
    // Proceso hijo 3
    i = 0;
    while (i < 5) {
      i = i + 1;
    }
    exit(0);
  }
  
  // Proceso padre espera
  i = 0;
  while (i < 10) {
    i = i + 1;
  }
  
  return 0;
}
