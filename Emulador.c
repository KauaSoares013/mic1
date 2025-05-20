#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;
typedef unsigned int palavra;
typedef unsigned long int microinstrucao;

palavra MAR = 0, MDR = 0, PC = 0;
byte MBR = 0;
palavra SP = 0, LV = 0, TOS = 0, OPC = 0, CPP = 0, H = 0;
microinstrucao MIR;
palavra MPC = 0;

palavra Barramento_B, Barramento_C;
byte N, Z;

byte MIR_B, MIR_Operacao, MIR_Deslocador, MIR_MEM, MIR_pulo;
palavra MIR_C;

microinstrucao Armazenamento[512];
byte Memoria[100000000];

void carregar_microprogram_de_controle();
void carregar_programa(const char *programa);
void exibir_processos();
void decodificar_microinstrucao();
void atribuir_barramento_B();
void realizar_operacao_ALU();
void atribuir_barramento_C();
void operar_memoria();
void pular();
void binario(void* valor, int tipo);

int main(int argc, const char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Uso: %s <programa.bin>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	carregar_microprogram_de_controle();
	carregar_programa(argv[1]);

	int ciclos = 0;
	const int MAX_CICLOS = 500; // Limite de execução para evitar loop infinito

	while (ciclos++ < MAX_CICLOS) {
		MIR = Armazenamento[MPC];

		exibir_processos();
		decodificar_microinstrucao();
		atribuir_barramento_B();
		realizar_operacao_ALU();
		atribuir_barramento_C();
		operar_memoria();
		pular();
	}

	printf("Execução encerrada após %d ciclos.\n", MAX_CICLOS);
	return 0;
}

void carregar_microprogram_de_controle() {
	FILE* MicroPrograma = fopen("microprog.rom", "rb");

	if (!MicroPrograma) {
		fprintf(stderr, "Erro ao abrir microprog.rom\n");
		exit(EXIT_FAILURE);
	}

	fread(Armazenamento, sizeof(microinstrucao), 512, MicroPrograma);
	fclose(MicroPrograma);
}

void carregar_programa(const char* prog) {
	FILE* Programa = fopen(prog, "rb");
	if (!Programa) {
		fprintf(stderr, "Erro ao abrir o programa %s\n", prog);
		exit(EXIT_FAILURE);
	}

	palavra tamanho;
	byte tamanho_temp[4];

	fread(tamanho_temp, sizeof(byte), 4, Programa);
	memcpy(&tamanho, tamanho_temp, 4);

	fread(Memoria, sizeof(byte), 20, Programa);
	fread(&Memoria[0x0401], sizeof(byte), tamanho - 20, Programa);

	fclose(Programa);
}

void decodificar_microinstrucao() {
	MIR_B = (MIR) & 0b1111;
	MIR_MEM = (MIR >> 4) & 0b111;
	MIR_C = (MIR >> 7) & 0b111111111;
	MIR_Operacao = (MIR >> 16) & 0b111111;
	MIR_Deslocador = (MIR >> 22) & 0b11;
	MIR_pulo = (MIR >> 24) & 0b111;
	MPC = (MIR >> 27) & 0b111111111;
}

void atribuir_barramento_B() {
	switch (MIR_B) {
		case 0: Barramento_B = MDR; break;
		case 1: Barramento_B = PC; break;
		case 2:
			Barramento_B = MBR;
			if (MBR & 0x80)
				Barramento_B |= 0xFFFFFF00;
			break;
		case 3: Barramento_B = MBR; break;
		case 4: Barramento_B = SP; break;
		case 5: Barramento_B = LV; break;
		case 6: Barramento_B = CPP; break;
		case 7: Barramento_B = TOS; break;
		case 8: Barramento_B = OPC; break;
		default: Barramento_B = 0; break;
	}
}

void realizar_operacao_ALU() {
	switch (MIR_Operacao) {
		case 12: Barramento_C = H & Barramento_B; break;
		case 17: Barramento_C = 1; break;
		case 18: Barramento_C = -1; break;
		case 20: Barramento_C = Barramento_B; break;
		case 24: Barramento_C = H; break;
		case 26: Barramento_C = ~H; break;
		case 28: Barramento_C = H | Barramento_B; break;
		case 44: Barramento_C = ~Barramento_B; break;
		case 53: Barramento_C = Barramento_B + 1; break;
		case 54: Barramento_C = Barramento_B - 1; break;
		case 57: Barramento_C = H + 1; break;
		case 59: Barramento_C = -H; break;
		case 60: Barramento_C = H + Barramento_B; break;
		case 61: Barramento_C = H + Barramento_B + 1; break;
		case 63: Barramento_C = Barramento_B - H; break;
		default: Barramento_C = 0; break;
	}

	N = Barramento_C ? 0 : 1;
	Z = Barramento_C ? 1 : 0;

	if (MIR_Deslocador == 1) Barramento_C <<= 8;
	else if (MIR_Deslocador == 2) Barramento_C >>= 1;
}

void atribuir_barramento_C() {
	if (MIR_C & 0b000000001) MAR = Barramento_C;
	if (MIR_C & 0b000000010) MDR = Barramento_C;
	if (MIR_C & 0b000000100) PC  = Barramento_C;
	if (MIR_C & 0b000001000) SP  = Barramento_C;
	if (MIR_C & 0b000010000) LV  = Barramento_C;
	if (MIR_C & 0b000100000) CPP = Barramento_C;
	if (MIR_C & 0b001000000) TOS = Barramento_C;
	if (MIR_C & 0b010000000) OPC = Barramento_C;
	if (MIR_C & 0b100000000) H   = Barramento_C;
}

void operar_memoria() {
	if (MIR_MEM & 0b001) MBR = Memoria[PC];

	if (MIR_MEM & 0b010) {
		if ((MAR * 4 + 3) < sizeof(Memoria))
			memcpy(&MDR, &Memoria[MAR * 4], 4);
		else
			fprintf(stderr, "ERRO: Leitura de memória fora dos limites\n");
	}

	if (MIR_MEM & 0b100) {
		if ((MAR * 4 + 3) < sizeof(Memoria))
			memcpy(&Memoria[MAR * 4], &MDR, 4);
		else
			fprintf(stderr, "ERRO: Escrita de memória fora dos limites\n");
	}
}

void pular() {
	if (MIR_pulo & 0b001) MPC |= (N << 8);
	if (MIR_pulo & 0b010) MPC |= (Z << 8);
	if (MIR_pulo & 0b100) MPC |= MBR;
}

void exibir_processos() {
	printf("\n--- Estado Atual ---\n");

	if (LV && SP) {
		printf("Pilha:\n");
		for (int i = SP; i >= LV; i--) {
			palavra val;
			memcpy(&val, &Memoria[i * 4], 4);
			printf("%s %02X: %d\n", i == SP ? "SP->" : i == LV ? "LV->" : "    ", i, val);
		}
	}

	printf("PC: %X | MAR: %X | MDR

