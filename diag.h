#pragma once
// Diagnostico: detecta (e opcionalmente restaura) modificacoes no codigo carregado.
void diag_snapshot();                 // guarda copia do codigo (chamar 1x no inicio de main)
int  diag_check(const char* stage, bool restore);  // imprime diferencas; retorna n de linhas alteradas
void diag_wait_a();                   // espera botao A
