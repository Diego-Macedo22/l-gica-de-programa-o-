// Sistema de registro de catracas
// Os usuarios ficam em um vetor alocado com malloc (cresce com realloc)
// e sao salvos no arquivo usuarios.txt, um por linha, separados por ';'
// Exemplo de linha: 2024001;Ana Souza;52998224725;3;Centro;850.50

#include <stdio.h>   // printf, scanf e funcoes de arquivo (fopen, fprintf, fscanf, fclose)
#include <stdlib.h>  // malloc, realloc e free (alocacao dinamica)
#include <string.h>  // strlen, strstr (funcoes de texto)

#define CAPACIDADE_INICIAL 5
#define ARQUIVO_DADOS "usuarios.txt"
#define ARQUIVO_RELATORIO "relatorio.txt"

// Registro de cada usuario
// O CPF e texto porque tem 11 digitos e nao cabe em um int
// (o int vai so ate 2.147.483.647). Sao 11 digitos + 1 para o '\0'
typedef struct {
    char nome[100];
    int matricula;
    char CPF[12];
    int semestre;
    char campus[20];
    float valorMensalidade;
} Usuario;

// Variaveis globais do vetor dinamico
Usuario *usuarios = NULL;  // ponteiro para o vetor criado com malloc
int quantidade = 0;        // quantos usuarios estao cadastrados
int capacidade = 0;        // quantos usuarios cabem no vetor atual

// Limpa o que sobrou no teclado (o Enter que o scanf deixa)
void limparBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Dobra o tamanho do vetor quando ele enche, usando realloc
// Retorna 1 se deu certo e 0 se faltou memoria
int aumentarVetor() {
    // guarda em um ponteiro auxiliar: se o realloc der NULL,
    // o vetor antigo nao e perdido
    Usuario *aux = realloc(usuarios, capacidade * 2 * sizeof(Usuario));

    if (aux == NULL) {
        printf("Erro: memoria insuficiente!\n");
        return 0;
    }

    usuarios = aux;
    capacidade = capacidade * 2;
    printf("(Vetor aumentado para %d posicoes)\n", capacidade);
    return 1;
}

// Procura a matricula no vetor e retorna a posicao, ou -1 se nao achar
int buscarPosicao(int matricula) {
    int i;
    for (i = 0; i < quantidade; i++) {
        if (usuarios[i].matricula == matricula) {
            return i;
        }
    }
    return -1;
}

// Confere se o CPF tem 11 caracteres e se todos sao numeros
int cpfValido(char cpf[]) {
    int i;
    if (strlen(cpf) != 11) {
        return 0;
    }
    for (i = 0; i < 11; i++) {
        if (cpf[i] < '0' || cpf[i] > '9') {
            return 0;
        }
    }
    return 1;
}

// Le um numero inteiro do teclado sem deixar o programa travar se
// o usuario digitar letras
int lerInteiro() {
    int numero;
    while (scanf("%d", &numero) != 1) {
        limparBuffer();
        printf("Digite apenas numeros: ");
    }
    limparBuffer();
    return numero;
}

// Mostra os dados de um usuario
void mostrarUsuario(Usuario u) {
    printf("Matricula: %d\n", u.matricula);
    printf("Nome: %s\n", u.nome);
    printf("CPF: %s\n", u.CPF);
    printf("Semestre: %d\n", u.semestre);
    printf("Campus: %s\n", u.campus);
    printf("Mensalidade: R$ %.2f\n", u.valorMensalidade);
    printf("------------------------------\n");
}

// Abre o arquivo no modo "r" (leitura) e coloca os registros no vetor
void carregarArquivo() {
    FILE *arq = fopen(ARQUIVO_DADOS, "r");
    Usuario u;

    // se o arquivo nao existe, o fopen retorna NULL
    if (arq == NULL) {
        printf("Arquivo %s nao encontrado. O sistema vai comecar vazio.\n", ARQUIVO_DADOS);
        return;
    }

    // %[^;] le o texto ate encontrar o ';' (assim o nome pode ter espaco)
    while (fscanf(arq, "%d;%99[^;];%11[^;];%d;%19[^;];%f\n", &u.matricula, u.nome,
                  u.CPF, &u.semestre, u.campus, &u.valorMensalidade) == 6) {
        if (quantidade == capacidade) {
            if (aumentarVetor() == 0) {
                break;
            }
        }
        usuarios[quantidade] = u;
        quantidade++;
    }

    fclose(arq);
    printf("%d usuario(s) carregado(s) do arquivo.\n", quantidade);
}

// Abre (ou cria) o arquivo no modo "w" (escrita) e grava todos os usuarios
// O modo "w" apaga o conteudo antigo, por isso grava a lista inteira
void salvarArquivo() {
    FILE *arq = fopen(ARQUIVO_DADOS, "w");
    int i;

    if (arq == NULL) {
        printf("Erro ao salvar o arquivo!\n");
        return;
    }

    for (i = 0; i < quantidade; i++) {
        fprintf(arq, "%d;%s;%s;%d;%s;%.2f\n", usuarios[i].matricula, usuarios[i].nome,
                usuarios[i].CPF, usuarios[i].semestre, usuarios[i].campus,
                usuarios[i].valorMensalidade);
    }

    fclose(arq);
}

// Cria o arquivo relatorio.txt com o total de registros e a soma, media,
// maior e menor valor da mensalidade
void gerarRelatorio() {
    FILE *arq = fopen(ARQUIVO_RELATORIO, "w");
    float soma = 0;
    int i, posMaior = 0, posMenor = 0;

    if (arq == NULL) {
        printf("Erro ao criar o relatorio!\n");
        return;
    }

    fprintf(arq, "===== RELATORIO GERAL DA CATRACA =====\n");
    fprintf(arq, "Total de registros: %d\n", quantidade);

    if (quantidade == 0) {
        fprintf(arq, "Nenhum usuario cadastrado.\n");
    } else {
        for (i = 0; i < quantidade; i++) {
            soma = soma + usuarios[i].valorMensalidade;
            if (usuarios[i].valorMensalidade > usuarios[posMaior].valorMensalidade) {
                posMaior = i;
            }
            if (usuarios[i].valorMensalidade < usuarios[posMenor].valorMensalidade) {
                posMenor = i;
            }
        }
        fprintf(arq, "Soma das mensalidades: R$ %.2f\n", soma);
        fprintf(arq, "Media das mensalidades: R$ %.2f\n", soma / quantidade);
        fprintf(arq, "Maior mensalidade: R$ %.2f (%s)\n",
                usuarios[posMaior].valorMensalidade, usuarios[posMaior].nome);
        fprintf(arq, "Menor mensalidade: R$ %.2f (%s)\n",
                usuarios[posMenor].valorMensalidade, usuarios[posMenor].nome);
    }

    fclose(arq);
}

// Opcao 1: cadastra um novo usuario no final do vetor
void cadastrarUsuario() {
    Usuario novo;

    // se o vetor estiver cheio, aumenta antes de cadastrar
    if (quantidade == capacidade) {
        if (aumentarVetor() == 0) {
            return;
        }
    }

    printf("Matricula: ");
    novo.matricula = lerInteiro();
    if (buscarPosicao(novo.matricula) != -1) {
        printf("Essa matricula ja esta cadastrada!\n");
        return;
    }

    printf("Nome: ");
    scanf(" %99[^\n]", novo.nome);
    limparBuffer();

    printf("CPF (somente numeros): ");
    scanf(" %11s", novo.CPF);
    limparBuffer();
    while (cpfValido(novo.CPF) == 0) {
        printf("CPF invalido, digite os 11 numeros: ");
        scanf(" %11s", novo.CPF);
        limparBuffer();
    }

    printf("Semestre: ");
    novo.semestre = lerInteiro();
    while (novo.semestre < 1 || novo.semestre > 20) {
        printf("Semestre invalido, digite de 1 a 20: ");
        novo.semestre = lerInteiro();
    }

    printf("Campus: ");
    scanf(" %19[^\n]", novo.campus);
    limparBuffer();

    printf("Valor da mensalidade: ");
    while (scanf("%f", &novo.valorMensalidade) != 1 || novo.valorMensalidade < 0) {
        limparBuffer();
        printf("Valor invalido, digite novamente: ");
    }
    limparBuffer();

    usuarios[quantidade] = novo;
    quantidade++;

    salvarArquivo();
    gerarRelatorio();
    printf("Usuario cadastrado com sucesso!\n");
}

// Opcao 2: apaga um usuario depois de pedir confirmacao
void deletarUsuario() {
    int matricula, pos, i;
    char resposta;

    printf("Matricula do usuario: ");
    matricula = lerInteiro();
    pos = buscarPosicao(matricula);

    if (pos == -1) {
        printf("Usuario nao encontrado!\n");
        return;
    }

    mostrarUsuario(usuarios[pos]);
    printf("Tem certeza que deseja deletar? (S/N): ");
    scanf(" %c", &resposta);
    limparBuffer();

    if (resposta != 'S' && resposta != 's') {
        printf("Exclusao cancelada.\n");
        return;
    }

    // puxa todos os usuarios seguintes uma posicao para tras
    for (i = pos; i < quantidade - 1; i++) {
        usuarios[i] = usuarios[i + 1];
    }
    quantidade--;

    salvarArquivo();
    gerarRelatorio();
    printf("Usuario deletado!\n");
}

// Opcao 3: confere se o cadastro existe e se os dados estao corretos
// (simula a liberacao da catraca)
void validarCadastro() {
    int matricula, pos;

    printf("Matricula: ");
    matricula = lerInteiro();
    pos = buscarPosicao(matricula);

    if (pos == -1) {
        printf("ACESSO NEGADO: matricula nao cadastrada.\n");
        return;
    }

    mostrarUsuario(usuarios[pos]);

    if (cpfValido(usuarios[pos].CPF) == 0) {
        printf("ACESSO NEGADO: CPF invalido no cadastro.\n");
    } else if (usuarios[pos].semestre < 1 || usuarios[pos].semestre > 20) {
        printf("ACESSO NEGADO: semestre invalido no cadastro.\n");
    } else if (usuarios[pos].valorMensalidade < 0) {
        printf("ACESSO NEGADO: mensalidade invalida no cadastro.\n");
    } else {
        printf("ACESSO LIBERADO! Cadastro valido.\n");
    }
}

// Opcao 4: altera um campo do registro escolhido
void alterarRegistro() {
    int matricula, pos, campo;

    printf("Matricula do usuario: ");
    matricula = lerInteiro();
    pos = buscarPosicao(matricula);

    if (pos == -1) {
        printf("Usuario nao encontrado!\n");
        return;
    }

    mostrarUsuario(usuarios[pos]);
    printf("Qual campo deseja alterar?\n");
    printf("1 - Nome\n2 - CPF\n3 - Semestre\n4 - Campus\n5 - Mensalidade\n0 - Cancelar\n");
    printf("Opcao: ");
    campo = lerInteiro();

    switch (campo) {
        case 1:
            printf("Novo nome: ");
            scanf(" %99[^\n]", usuarios[pos].nome);
            limparBuffer();
            break;
        case 2:
            printf("Novo CPF: ");
            scanf(" %11s", usuarios[pos].CPF);
            limparBuffer();
            while (cpfValido(usuarios[pos].CPF) == 0) {
                printf("CPF invalido, digite os 11 numeros: ");
                scanf(" %11s", usuarios[pos].CPF);
                limparBuffer();
            }
            break;
        case 3:
            printf("Novo semestre: ");
            usuarios[pos].semestre = lerInteiro();
            while (usuarios[pos].semestre < 1 || usuarios[pos].semestre > 20) {
                printf("Semestre invalido, digite de 1 a 20: ");
                usuarios[pos].semestre = lerInteiro();
            }
            break;
        case 4:
            printf("Novo campus: ");
            scanf(" %19[^\n]", usuarios[pos].campus);
            limparBuffer();
            break;
        case 5:
            printf("Nova mensalidade: ");
            while (scanf("%f", &usuarios[pos].valorMensalidade) != 1 ||
                   usuarios[pos].valorMensalidade < 0) {
                limparBuffer();
                printf("Valor invalido, digite novamente: ");
            }
            limparBuffer();
            break;
        default:
            printf("Nenhuma alteracao feita.\n");
            return;
    }

    salvarArquivo();
    gerarRelatorio();
    printf("Registro alterado!\n");
}

// Opcao 5: lista todos os usuarios e mostra os dados gerais
void resumoUsuarios() {
    int i;
    float soma = 0;

    if (quantidade == 0) {
        printf("Nenhum usuario cadastrado.\n");
        return;
    }

    for (i = 0; i < quantidade; i++) {
        mostrarUsuario(usuarios[i]);
        soma = soma + usuarios[i].valorMensalidade;
    }

    printf("Total de usuarios: %d\n", quantidade);
    printf("Media das mensalidades: R$ %.2f\n", soma / quantidade);
    printf("Posicoes usadas no vetor: %d de %d\n", quantidade, capacidade);

    gerarRelatorio();
    printf("Relatorio completo salvo em %s\n", ARQUIVO_RELATORIO);
}

// Opcao 6: busca pela matricula ou por parte do nome
void buscarUsuario() {
    int tipo, matricula, pos, i, achou = 0;
    char nome[100];

    printf("1 - Buscar por matricula\n2 - Buscar por nome\nOpcao: ");
    tipo = lerInteiro();

    if (tipo == 1) {
        printf("Matricula: ");
        matricula = lerInteiro();
        pos = buscarPosicao(matricula);
        if (pos == -1) {
            printf("Usuario nao encontrado!\n");
        } else {
            mostrarUsuario(usuarios[pos]);
        }
    } else if (tipo == 2) {
        printf("Nome: ");
        scanf(" %99[^\n]", nome);
        limparBuffer();
        for (i = 0; i < quantidade; i++) {
            // strstr procura o texto digitado dentro do nome
            if (strstr(usuarios[i].nome, nome) != NULL) {
                mostrarUsuario(usuarios[i]);
                achou = 1;
            }
        }
        if (achou == 0) {
            printf("Nenhum usuario encontrado com esse nome.\n");
        }
    } else {
        printf("Opcao invalida!\n");
    }
}

int main() {
    int opcao;

    // cria o vetor com a capacidade inicial usando malloc
    usuarios = malloc(CAPACIDADE_INICIAL * sizeof(Usuario));
    if (usuarios == NULL) {
        printf("Erro: nao foi possivel alocar memoria!\n");
        return 1;
    }
    capacidade = CAPACIDADE_INICIAL;

    carregarArquivo();

    do {
        // Exibição do menu
        printf("\n==============================\n");
        printf("       MENU DO SISTEMA        \n");
        printf("==============================\n");
        printf("1 - Cadastro de novo usuario\n");
        printf("2 - Deletar usuario\n");
        printf("3 - Validar cadastro\n");
        printf("4 - Alteracao de registro\n");
        printf("5 - Resumo de usuarios\n");
        printf("6 - Busca por matricula/nome\n");
        printf("7 - Encerrar programa\n");
        printf("==============================\n");
        printf("Escolha uma opcao: ");
        opcao = lerInteiro();

        // Controle das opções escolhidas
        switch (opcao) {
            case 1:
                printf("\n[1] Modulo: Cadastro de novo usuario.\n");
                cadastrarUsuario();
                break;

            case 2:
                printf("\n[2] Modulo: Deletar usuario.\n");
                deletarUsuario();
                break;

            case 3:
                printf("\n[3] Modulo: Validar cadastro.\n");
                validarCadastro();
                break;

            case 4:
                printf("\n[4] Modulo: Alteracao de registro.\n");
                alterarRegistro();
                break;

            case 5:
                printf("\n[5] Modulo: Resumo de usuarios.\n");
                resumoUsuarios();
                break;

            case 6:
                printf("\n[6] Modulo: Busca por matricula/nome.\n");
                buscarUsuario();
                break;

            case 7:
                salvarArquivo();
                gerarRelatorio();
                printf("\nEncerrando o programa... Ate logo!\n");
                break;

            default:
                printf("\nOpcao invalida! Por favor, digite um numero de 1 a 7.\n");
                break;
        }

    } while (opcao != 7); // Repete enquanto a opção for diferente de 7

    // libera a memoria do vetor antes de terminar
    free(usuarios);
    return 0;
}
