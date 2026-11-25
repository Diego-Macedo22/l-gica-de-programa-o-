/*
 * =====================================================================
 *  SISTEMA DE REGISTRO DE CATRACAS
 * =====================================================================
 *  Programa em C que gerencia o cadastro dos usuarios que passam pelas
 *  catracas (alunos), guardando os registros em um VETOR DINAMICO
 *  (alocado com malloc e ampliado com realloc) e persistindo os dados
 *  em um arquivo texto.
 *
 *  Arquivos usados pelo programa:
 *    - usuarios.txt  -> arquivo de DADOS (um registro por linha).
 *                       Lido na abertura (fopen "r") e regravado apos
 *                       cada alteracao e ao sair (fopen "w" + fprintf).
 *    - relatorio.txt -> arquivo de RELATORIO com dados gerais (total de
 *                       registros, soma, media, maior e menor valor da
 *                       mensalidade). Regerado junto com os dados.
 *
 *  Formato de cada linha do usuarios.txt (campos separados por ';'):
 *    matricula;nome;CPF;semestre;campus;valorMensalidade
 *    Exemplo: 2024001;Ana Souza;52998224725;3;Centro;850.50
 *
 *  Como compilar e executar (GCC):
 *    gcc catraca.c -o catraca -Wall -Wextra
 *    ./catraca          (Linux/macOS)
 *    catraca.exe        (Windows)
 * =====================================================================
 */

/* ---------------------------------------------------------------------
 *  BIBLIOTECAS
 * --------------------------------------------------------------------- */
#include <stdio.h>   /* Entrada e saida: printf, scanf, fgets, getchar,
                        e manipulacao de arquivos: FILE, fopen, fclose,
                        fprintf, fgets, feof.                           */
#include <stdlib.h>  /* Alocacao dinamica de memoria: malloc, realloc,
                        free. Tambem strtol/strtof (texto -> numero).   */
#include <string.h>  /* Manipulacao de strings: strlen, strcpy, strncpy,
                        strstr, memmove.                                */
#include <ctype.h>   /* Classificacao/conversao de caracteres: isspace,
                        isdigit, tolower, toupper.                      */
#include <limits.h>  /* Limites dos tipos inteiros, como INT_MAX.       */
#include <errno.h>   /* Variavel errno, usada para detectar estouro
                        (ERANGE) nas conversoes strtol/strtof.          */
#include <time.h>    /* Data e hora atuais (time, localtime, strftime),
                        usadas no cabecalho do relatorio.               */

/* ---------------------------------------------------------------------
 *  CONSTANTES
 * --------------------------------------------------------------------- */
#define ARQUIVO_DADOS      "usuarios.txt"   /* arquivo com os registros   */
#define ARQUIVO_RELATORIO  "relatorio.txt"  /* arquivo com dados gerais   */
#define CAPACIDADE_INICIAL 5                /* tamanho inicial do vetor   */
#define FATOR_CRESCIMENTO  2                /* vetor dobra quando enche   */
#define TAM_NOME           100
#define TAM_CAMPUS         20
#define TAM_LINHA          512              /* buffer p/ ler uma linha    */
#define SEMESTRE_MIN       1
#define SEMESTRE_MAX       20
#define MENSALIDADE_MAX    1000000.0f

/* ---------------------------------------------------------------------
 *  ESTRUTURAS
 * --------------------------------------------------------------------- */

/*
 * Registro de um usuario da catraca.
 *
 * Observacao sobre o CPF: um CPF tem 11 digitos (ate 99.999.999.999),
 * mas um 'int' de 32 bits so guarda ate 2.147.483.647 (10 digitos).
 * Por isso o campo usa 'long long' (64 bits), que comporta o CPF
 * completo sem estourar e permite validar os digitos verificadores.
 */
typedef struct {
    char  nome[TAM_NOME];
    int   matricula;
    long long CPF;
    int   semestre;
    char  campus[TAM_CAMPUS];
    float valorMensalidade;
} Usuario;

/*
 * Vetor dinamico de usuarios.
 *  - dados      : ponteiro para a area alocada com malloc/realloc
 *                 (NAO e um vetor de tamanho fixo).
 *  - quantidade : quantos registros estao realmente ocupados.
 *  - capacidade : quantos registros cabem na memoria ja alocada.
 */
typedef struct {
    Usuario *dados;
    int quantidade;
    int capacidade;
} ListaUsuarios;

/* Resultado dos calculos estatisticos sobre o campo valorMensalidade. */
typedef struct {
    int    total;
    double soma;
    double media;
    int    indiceMaior;   /* posicao no vetor do usuario de maior valor */
    int    indiceMenor;   /* posicao no vetor do usuario de menor valor */
} Estatisticas;

/* ---------------------------------------------------------------------
 *  PROTOTIPOS DAS FUNCOES
 * --------------------------------------------------------------------- */
/* Gerenciamento do vetor dinamico */
int  inicializarLista(ListaUsuarios *lista);
int  garantirCapacidade(ListaUsuarios *lista);
int  adicionarUsuario(ListaUsuarios *lista, const Usuario *novo);
void removerUsuario(ListaUsuarios *lista, int indice);
void liberarLista(ListaUsuarios *lista);
int  buscarIndicePorMatricula(const ListaUsuarios *lista, int matricula);

/* Arquivos */
void carregarArquivo(ListaUsuarios *lista, const char *nomeArquivo);
int  salvarArquivo(const ListaUsuarios *lista, const char *nomeArquivo);
int  gerarRelatorio(const ListaUsuarios *lista, const char *nomeArquivo);
void salvarTudo(const ListaUsuarios *lista);

/* Leitura e validacao de entradas do teclado */
void limparBuffer(void);
void removerEspacos(char *texto);
void substituirPontoEVirgula(char *texto);
int  lerLinha(const char *mensagem, char *destino, size_t tamanho);
int  lerTextoObrigatorio(const char *mensagem, char *destino, size_t tamanho);
int  lerInteiro(const char *mensagem, long minimo, long maximo, int *saida);
int  lerFloat(const char *mensagem, float minimo, float maximo, float *saida);
int  lerCPF(const char *mensagem, long long *saida);
int  lerConfirmacao(const char *mensagem);
int  validarCPF(long long cpf);
int  contemSemDiferenciarCaixa(const char *texto, const char *trecho);

/* Exibicao e estatisticas */
void imprimirUsuario(const Usuario *u);
void imprimirCabecalhoTabela(void);
void imprimirLinhaTabela(const Usuario *u);
void calcularEstatisticas(const ListaUsuarios *lista, Estatisticas *est);

/* Modulos do menu (um por opcao do switch) */
void cadastrarUsuario(ListaUsuarios *lista);
void deletarUsuario(ListaUsuarios *lista);
void validarCadastro(const ListaUsuarios *lista);
void alterarRegistro(ListaUsuarios *lista);
void resumoUsuarios(const ListaUsuarios *lista);
void buscarUsuario(const ListaUsuarios *lista);

/* =====================================================================
 *  FUNCAO PRINCIPAL
 * ===================================================================== */
int main() {
    ListaUsuarios lista;   /* vetor dinamico com todos os registros */
    int opcao;

    /* Aloca a memoria inicial do vetor. Sem memoria nao ha como seguir. */
    if (!inicializarLista(&lista)) {
        printf("ERRO FATAL: nao foi possivel alocar memoria inicial.\n");
        return 1;
    }

    /* Carrega os registros do arquivo (se existir) para a memoria. */
    carregarArquivo(&lista, ARQUIVO_DADOS);

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

        /*
         * scanf devolve quantos valores conseguiu ler.
         *  - Se o usuario digitar letras, devolve 0: tratamos como opcao
         *    invalida (sem isso o programa entraria em loop infinito).
         *  - Se a entrada terminar (EOF, ex.: Ctrl+D / Ctrl+Z), encerramos
         *    de forma segura, salvando os dados.
         */
        if (scanf("%d", &opcao) != 1) {
            opcao = feof(stdin) ? 7 : 0;
        }
        limparBuffer(); /* descarta o '\n' (e o resto da linha) do teclado */

        // Controle das opções escolhidas
        switch (opcao) {
            case 1:
                printf("\n[1] Modulo: Cadastro de novo usuario.\n");
                cadastrarUsuario(&lista);
                break;

            case 2:
                printf("\n[2] Modulo: Deletar usuario.\n");
                deletarUsuario(&lista);
                break;

            case 3:
                printf("\n[3] Modulo: Validar cadastro.\n");
                validarCadastro(&lista);
                break;

            case 4:
                printf("\n[4] Modulo: Alteracao de registro.\n");
                alterarRegistro(&lista);
                break;

            case 5:
                printf("\n[5] Modulo: Resumo de usuarios.\n");
                resumoUsuarios(&lista);
                break;

            case 6:
                printf("\n[6] Modulo: Busca por matricula/nome.\n");
                buscarUsuario(&lista);
                break;

            case 7:
                /* Gravacao final dos dados e do relatorio antes de sair. */
                salvarTudo(&lista);
                printf("\nEncerrando o programa... Ate logo!\n");
                break;

            default:
                printf("\nOpcao invalida! Por favor, digite um numero de 1 a 7.\n");
                break;
        }

    } while (opcao != 7); // Repete enquanto a opção for diferente de 7

    /* Devolve ao sistema toda a memoria alocada dinamicamente. */
    liberarLista(&lista);
    return 0;
}

/* =====================================================================
 *  GERENCIAMENTO DO VETOR DINAMICO (malloc / realloc / free)
 * ===================================================================== */

/*
 * inicializarLista
 * Reserva memoria para CAPACIDADE_INICIAL registros usando malloc.
 * malloc(n bytes) devolve um ponteiro para a area reservada, ou NULL se
 * nao houver memoria disponivel -- por isso o retorno SEMPRE e testado.
 * Retorna 1 em caso de sucesso e 0 em caso de falha.
 */
int inicializarLista(ListaUsuarios *lista) {
    lista->quantidade = 0;
    lista->capacidade = 0;
    lista->dados = (Usuario *) malloc(CAPACIDADE_INICIAL * sizeof(Usuario));

    if (lista->dados == NULL) {      /* teste obrigatorio do malloc */
        return 0;
    }
    lista->capacidade = CAPACIDADE_INICIAL;
    return 1;
}

/*
 * garantirCapacidade
 * Verifica se ha espaco para mais um registro. Se o vetor estiver cheio,
 * aumenta a capacidade (dobra) usando realloc.
 *
 * realloc(ptr, novoTamanho) tenta redimensionar a area apontada por ptr,
 * copiando os dados antigos para a nova area se precisar mudar de lugar.
 * Se falhar, devolve NULL e a area ORIGINAL continua valida. Por isso o
 * resultado e guardado primeiro em um ponteiro temporario: se fizessemos
 * "lista->dados = realloc(...)" e desse NULL, perderiamos o endereco da
 * memoria antiga (vazamento de memoria + perda de todos os registros).
 * Retorna 1 se ha espaco disponivel e 0 se nao foi possivel ampliar.
 */
int garantirCapacidade(ListaUsuarios *lista) {
    Usuario *temporario;
    int novaCapacidade;

    if (lista->quantidade < lista->capacidade) {
        return 1; /* ainda ha espaco, nada a fazer */
    }

    novaCapacidade = lista->capacidade * FATOR_CRESCIMENTO;
    temporario = (Usuario *) realloc(lista->dados,
                                     (size_t) novaCapacidade * sizeof(Usuario));

    if (temporario == NULL) {        /* teste obrigatorio do realloc */
        printf("ERRO: memoria insuficiente para ampliar o vetor.\n");
        return 0;                    /* lista->dados continua intacto */
    }

    lista->dados = temporario;
    lista->capacidade = novaCapacidade;
    printf("(Vetor ampliado: capacidade agora e de %d registros.)\n",
           lista->capacidade);
    return 1;
}

/*
 * adicionarUsuario
 * Insere uma copia de 'novo' no final do vetor, ampliando-o se preciso.
 * Retorna 1 se inseriu e 0 se faltou memoria.
 */
int adicionarUsuario(ListaUsuarios *lista, const Usuario *novo) {
    if (!garantirCapacidade(lista)) {
        return 0;
    }
    lista->dados[lista->quantidade] = *novo; /* copia a struct inteira */
    lista->quantidade++;
    return 1;
}

/*
 * removerUsuario
 * Remove o registro da posicao 'indice' deslocando os seguintes uma
 * posicao para tras (memmove copia blocos de memoria que se sobrepoem
 * com seguranca). A memoria alocada nao muda; so a quantidade diminui.
 */
void removerUsuario(ListaUsuarios *lista, int indice) {
    int restantes = lista->quantidade - indice - 1;

    if (restantes > 0) {
        memmove(&lista->dados[indice], &lista->dados[indice + 1],
                (size_t) restantes * sizeof(Usuario));
    }
    lista->quantidade--;
}

/*
 * liberarLista
 * Devolve ao sistema a memoria obtida com malloc/realloc usando free.
 * Depois do free o ponteiro e zerado (NULL) para evitar uso acidental
 * de uma area ja liberada ("ponteiro pendurado").
 */
void liberarLista(ListaUsuarios *lista) {
    free(lista->dados);
    lista->dados = NULL;
    lista->quantidade = 0;
    lista->capacidade = 0;
}

/*
 * buscarIndicePorMatricula
 * Percorre o vetor procurando a matricula informada.
 * Retorna a posicao encontrada ou -1 se nao existir.
 */
int buscarIndicePorMatricula(const ListaUsuarios *lista, int matricula) {
    int i;
    for (i = 0; i < lista->quantidade; i++) {
        if (lista->dados[i].matricula == matricula) {
            return i;
        }
    }
    return -1;
}

/* =====================================================================
 *  ARQUIVOS (fopen / fgets / fprintf / fclose)
 * ===================================================================== */

/*
 * carregarArquivo
 * Abre o arquivo de dados em modo leitura ("r") e carrega cada linha
 * como um registro no vetor dinamico.
 *  - fopen devolve NULL se o arquivo nao existir (ex.: primeira execucao).
 *    Nesse caso o programa apenas avisa e comeca com a lista vazia,
 *    sem travar nem encerrar.
 *  - fgets le uma linha inteira por vez (com limite de tamanho, seguro).
 *  - sscanf separa os campos da linha usando ';' como delimitador.
 *    "%99[^;]" significa: leia ate 99 caracteres que NAO sejam ';'
 *    (permite nomes com espacos). Os espacos no formato (" ;") aceitam
 *    tambem arquivos escritos com espacos em volta do separador.
 *  - Linhas mal formatadas ou com matricula repetida sao ignoradas com
 *    um aviso, para que um arquivo com defeito nao derrube o programa.
 */
void carregarArquivo(ListaUsuarios *lista, const char *nomeArquivo) {
    FILE *arquivo;
    char linha[TAM_LINHA];
    int numeroLinha = 0;
    int carregados = 0;
    Usuario u;

    arquivo = fopen(nomeArquivo, "r");
    if (arquivo == NULL) {
        printf("AVISO: arquivo '%s' nao encontrado. "
               "O sistema comecara sem registros.\n", nomeArquivo);
        return;
    }

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        numeroLinha++;
        removerEspacos(linha);
        if (linha[0] == '\0') {
            continue; /* ignora linhas em branco */
        }

        if (sscanf(linha, "%d ;%99[^;];%lld ;%d ;%19[^;];%f",
                   &u.matricula, u.nome, &u.CPF, &u.semestre,
                   u.campus, &u.valorMensalidade) != 6) {
            printf("AVISO: linha %d do arquivo ignorada (formato invalido).\n",
                   numeroLinha);
            continue;
        }
        removerEspacos(u.nome);
        removerEspacos(u.campus);

        if (buscarIndicePorMatricula(lista, u.matricula) != -1) {
            printf("AVISO: linha %d ignorada (matricula %d repetida).\n",
                   numeroLinha, u.matricula);
            continue;
        }

        if (!adicionarUsuario(lista, &u)) {
            printf("AVISO: sem memoria; carregamento interrompido na linha %d.\n",
                   numeroLinha);
            break;
        }
        carregados++;
    }

    fclose(arquivo); /* sempre fechar o arquivo apos o uso */
    printf("%d registro(s) carregado(s) de '%s'.\n", carregados, nomeArquivo);
}

/*
 * salvarArquivo
 * Abre (ou CRIA, se nao existir) o arquivo de dados em modo escrita
 * ("w"). O modo "w" apaga o conteudo anterior, por isso todos os
 * registros da memoria sao regravados, um por linha, com fprintf.
 * O CPF e gravado com "%011lld" para manter os zeros a esquerda.
 * Retorna 1 em caso de sucesso e 0 se nao conseguiu abrir o arquivo.
 */
int salvarArquivo(const ListaUsuarios *lista, const char *nomeArquivo) {
    FILE *arquivo;
    int i;

    arquivo = fopen(nomeArquivo, "w");
    if (arquivo == NULL) {
        printf("ERRO: nao foi possivel gravar o arquivo '%s'.\n", nomeArquivo);
        return 0;
    }

    for (i = 0; i < lista->quantidade; i++) {
        const Usuario *u = &lista->dados[i];
        fprintf(arquivo, "%d;%s;%011lld;%d;%s;%.2f\n",
                u->matricula, u->nome, u->CPF, u->semestre,
                u->campus, u->valorMensalidade);
    }

    fclose(arquivo);
    return 1;
}

/*
 * gerarRelatorio
 * Cria (modo "w") o arquivo texto de relatorio com os dados gerais:
 * total de registros e, para o campo numerico valorMensalidade, a soma,
 * a media, o maior e o menor valor (indicando a quem pertencem).
 * Retorna 1 em caso de sucesso e 0 se nao conseguiu criar o arquivo.
 */
int gerarRelatorio(const ListaUsuarios *lista, const char *nomeArquivo) {
    FILE *arquivo;
    Estatisticas est;
    char dataHora[32];
    time_t agora = time(NULL);                 /* instante atual          */
    struct tm *local = localtime(&agora);      /* converte p/ data local  */

    if (local == NULL ||
        strftime(dataHora, sizeof(dataHora), "%d/%m/%Y %H:%M:%S", local) == 0) {
        strcpy(dataHora, "indisponivel");
    }

    arquivo = fopen(nomeArquivo, "w");
    if (arquivo == NULL) {
        printf("ERRO: nao foi possivel gravar o arquivo '%s'.\n", nomeArquivo);
        return 0;
    }

    calcularEstatisticas(lista, &est);

    fprintf(arquivo, "==================================================\n");
    fprintf(arquivo, "   RELATORIO GERAL - SISTEMA DE CATRACAS\n");
    fprintf(arquivo, "==================================================\n");
    fprintf(arquivo, "Gerado em ............: %s\n", dataHora);
    fprintf(arquivo, "Arquivo de dados .....: %s\n", ARQUIVO_DADOS);
    fprintf(arquivo, "Total de registros ...: %d\n", est.total);
    fprintf(arquivo, "--------------------------------------------------\n");
    fprintf(arquivo, "Campo analisado: valorMensalidade\n");
    fprintf(arquivo, "--------------------------------------------------\n");

    if (est.total == 0) {
        fprintf(arquivo, "Nenhum registro cadastrado; nao ha valores a analisar.\n");
    } else {
        const Usuario *maior = &lista->dados[est.indiceMaior];
        const Usuario *menor = &lista->dados[est.indiceMenor];

        fprintf(arquivo, "Soma das mensalidades : R$ %.2f\n", est.soma);
        fprintf(arquivo, "Media das mensalidades: R$ %.2f\n", est.media);
        fprintf(arquivo, "Maior mensalidade ....: R$ %.2f (%s, matricula %d)\n",
                maior->valorMensalidade, maior->nome, maior->matricula);
        fprintf(arquivo, "Menor mensalidade ....: R$ %.2f (%s, matricula %d)\n",
                menor->valorMensalidade, menor->nome, menor->matricula);
    }
    fprintf(arquivo, "==================================================\n");

    fclose(arquivo);
    return 1;
}

/*
 * salvarTudo
 * Grava o arquivo de dados e atualiza o relatorio. E chamada apos cada
 * alteracao (cadastro, exclusao, edicao) e ao encerrar o programa, para
 * que nada se perca caso o programa seja fechado de forma inesperada.
 */
void salvarTudo(const ListaUsuarios *lista) {
    int okDados = salvarArquivo(lista, ARQUIVO_DADOS);
    int okRelatorio = gerarRelatorio(lista, ARQUIVO_RELATORIO);

    if (okDados && okRelatorio) {
        printf("(Dados salvos em '%s' e relatorio atualizado em '%s'.)\n",
               ARQUIVO_DADOS, ARQUIVO_RELATORIO);
    }
}

/* =====================================================================
 *  LEITURA E VALIDACAO DE ENTRADAS
 *  Todas as leituras usam fgets (le a linha inteira, com limite de
 *  tamanho) e depois convertem o texto. Isso evita os problemas comuns
 *  do scanf (lixo no buffer, loop infinito ao digitar letras, estouro
 *  de vetor). As funcoes retornam 0 se a entrada terminar (EOF), para
 *  que a operacao em andamento seja cancelada sem travar o programa.
 * ===================================================================== */

/* Descarta tudo o que sobrou no buffer do teclado ate o fim da linha. */
void limparBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        /* apenas descarta */
    }
}

/* Remove espacos, tabs, '\r' e '\n' do inicio e do fim de um texto. */
void removerEspacos(char *texto) {
    size_t inicio = 0;
    size_t fim = strlen(texto);

    while (texto[inicio] != '\0' && isspace((unsigned char) texto[inicio])) {
        inicio++;
    }
    while (fim > inicio && isspace((unsigned char) texto[fim - 1])) {
        fim--;
    }
    memmove(texto, texto + inicio, fim - inicio);
    texto[fim - inicio] = '\0';
}

/*
 * O ';' e o separador de campos do arquivo. Se aparecesse dentro de um
 * nome, quebraria o registro na proxima leitura; por isso e trocado
 * por ','.
 */
void substituirPontoEVirgula(char *texto) {
    for (; *texto != '\0'; texto++) {
        if (*texto == ';') {
            *texto = ',';
        }
    }
}

/*
 * lerLinha
 * Mostra a mensagem e le uma linha do teclado para 'destino' (no maximo
 * tamanho-1 caracteres). Se o usuario digitar mais do que cabe, o
 * excesso e descartado. Retorna 0 se a entrada terminou (EOF).
 */
int lerLinha(const char *mensagem, char *destino, size_t tamanho) {
    size_t len;

    printf("%s", mensagem);
    if (fgets(destino, (int) tamanho, stdin) == NULL) {
        destino[0] = '\0';
        return 0;
    }

    len = strlen(destino);
    if (len > 0 && destino[len - 1] == '\n') {
        destino[len - 1] = '\0';
    } else {
        limparBuffer(); /* linha maior que o buffer: descarta o restante */
    }
    removerEspacos(destino);
    return 1;
}

/* Le um texto que nao pode ficar vazio (ex.: nome e campus). */
int lerTextoObrigatorio(const char *mensagem, char *destino, size_t tamanho) {
    while (1) {
        if (!lerLinha(mensagem, destino, tamanho)) {
            return 0;
        }
        if (destino[0] != '\0') {
            substituirPontoEVirgula(destino);
            return 1;
        }
        printf("Este campo nao pode ficar vazio.\n");
    }
}

/*
 * lerInteiro
 * Le um numero inteiro entre 'minimo' e 'maximo', repetindo a pergunta
 * ate receber um valor valido. strtol converte o texto e informa, por
 * meio de 'fim', onde a conversao parou: se sobrou algum caractere, o
 * texto nao era um numero puro (ex.: "12abc").
 */
int lerInteiro(const char *mensagem, long minimo, long maximo, int *saida) {
    char buffer[64];
    char *fim;
    long valor;

    while (1) {
        if (!lerLinha(mensagem, buffer, sizeof(buffer))) {
            return 0;
        }
        errno = 0;
        valor = strtol(buffer, &fim, 10);
        if (buffer[0] != '\0' && *fim == '\0' && errno != ERANGE &&
            valor >= minimo && valor <= maximo) {
            *saida = (int) valor;
            return 1;
        }
        printf("Valor invalido. Digite um numero inteiro entre %ld e %ld.\n",
               minimo, maximo);
    }
}

/*
 * lerFloat
 * Le um numero real entre 'minimo' e 'maximo'. Aceita virgula ou ponto
 * como separador decimal (ex.: 850,50 ou 850.50).
 */
int lerFloat(const char *mensagem, float minimo, float maximo, float *saida) {
    char buffer[64];
    char *fim;
    char *p;
    float valor;

    while (1) {
        if (!lerLinha(mensagem, buffer, sizeof(buffer))) {
            return 0;
        }
        for (p = buffer; *p != '\0'; p++) {
            if (*p == ',') {
                *p = '.';
            }
        }
        errno = 0;
        valor = strtof(buffer, &fim);
        if (buffer[0] != '\0' && *fim == '\0' && errno != ERANGE &&
            valor >= minimo && valor <= maximo) {
            *saida = valor;
            return 1;
        }
        printf("Valor invalido. Digite um numero entre %.2f e %.2f.\n",
               minimo, maximo);
    }
}

/*
 * lerCPF
 * Le um CPF aceitando os formatos "52998224725" ou "529.982.247-25".
 * Exige exatamente 11 digitos e digitos verificadores corretos.
 */
int lerCPF(const char *mensagem, long long *saida) {
    char buffer[64];
    char digitos[12];
    int qtd, i, valido;
    long long cpf;

    while (1) {
        if (!lerLinha(mensagem, buffer, sizeof(buffer))) {
            return 0;
        }

        qtd = 0;
        valido = 1;
        for (i = 0; buffer[i] != '\0'; i++) {
            if (isdigit((unsigned char) buffer[i])) {
                if (qtd < 11) {
                    digitos[qtd] = buffer[i];
                }
                qtd++;
            } else if (buffer[i] != '.' && buffer[i] != '-' && buffer[i] != ' ') {
                valido = 0; /* caractere nao permitido */
            }
        }

        if (valido && qtd == 11) {
            digitos[11] = '\0';
            cpf = strtoll(digitos, NULL, 10);
            if (validarCPF(cpf)) {
                *saida = cpf;
                return 1;
            }
            printf("CPF invalido (digitos verificadores nao conferem).\n");
        } else {
            printf("CPF invalido. Digite 11 digitos (ex.: 529.982.247-25).\n");
        }
    }
}

/*
 * validarCPF
 * Confere os dois digitos verificadores do CPF pelo algoritmo oficial
 * (modulo 11). Rejeita tambem sequencias repetidas (ex.: 111.111.111-11).
 * Retorna 1 se valido e 0 se invalido.
 */
int validarCPF(long long cpf) {
    int d[11];
    int i, soma, resto, todosIguais = 1;

    if (cpf <= 0 || cpf > 99999999999LL) {
        return 0;
    }

    /* separa os 11 digitos, do ultimo para o primeiro */
    for (i = 10; i >= 0; i--) {
        d[i] = (int) (cpf % 10);
        cpf /= 10;
    }

    for (i = 1; i < 11; i++) {
        if (d[i] != d[0]) {
            todosIguais = 0;
        }
    }
    if (todosIguais) {
        return 0;
    }

    /* primeiro digito verificador */
    soma = 0;
    for (i = 0; i < 9; i++) {
        soma += d[i] * (10 - i);
    }
    resto = (soma * 10) % 11;
    if (resto == 10) resto = 0;
    if (resto != d[9]) return 0;

    /* segundo digito verificador */
    soma = 0;
    for (i = 0; i < 10; i++) {
        soma += d[i] * (11 - i);
    }
    resto = (soma * 10) % 11;
    if (resto == 10) resto = 0;
    return resto == d[10];
}

/*
 * lerConfirmacao
 * Pergunta S/N e repete ate obter uma resposta valida.
 * Retorna 1 para "S" e 0 para "N" (ou se a entrada terminar).
 */
int lerConfirmacao(const char *mensagem) {
    char buffer[16];

    while (1) {
        if (!lerLinha(mensagem, buffer, sizeof(buffer))) {
            return 0;
        }
        if (buffer[1] == '\0') {
            char c = (char) toupper((unsigned char) buffer[0]);
            if (c == 'S') return 1;
            if (c == 'N') return 0;
        }
        printf("Responda apenas S (sim) ou N (nao).\n");
    }
}

/*
 * contemSemDiferenciarCaixa
 * Verifica se 'trecho' aparece dentro de 'texto' ignorando maiusculas e
 * minusculas (ex.: "ana" encontra "Ana Souza" e "Mariana").
 */
int contemSemDiferenciarCaixa(const char *texto, const char *trecho) {
    char a[TAM_NOME];
    char b[TAM_NOME];
    size_t i;

    for (i = 0; texto[i] != '\0' && i < TAM_NOME - 1; i++) {
        a[i] = (char) tolower((unsigned char) texto[i]);
    }
    a[i] = '\0';

    for (i = 0; trecho[i] != '\0' && i < TAM_NOME - 1; i++) {
        b[i] = (char) tolower((unsigned char) trecho[i]);
    }
    b[i] = '\0';

    return strstr(a, b) != NULL; /* strstr procura b dentro de a */
}

/* =====================================================================
 *  EXIBICAO E ESTATISTICAS
 * ===================================================================== */

/* Mostra todos os campos de um usuario, um por linha. */
void imprimirUsuario(const Usuario *u) {
    printf("------------------------------\n");
    printf("Matricula ....: %d\n", u->matricula);
    printf("Nome .........: %s\n", u->nome);
    printf("CPF ..........: %011lld\n", u->CPF);
    printf("Semestre .....: %d\n", u->semestre);
    printf("Campus .......: %s\n", u->campus);
    printf("Mensalidade ..: R$ %.2f\n", u->valorMensalidade);
    printf("------------------------------\n");
}

void imprimirCabecalhoTabela(void) {
    printf("%-10s | %-30s | %-11s | %-3s | %-15s | %10s\n",
           "MATRICULA", "NOME", "CPF", "SEM", "CAMPUS", "MENSAL.");
    printf("-----------+--------------------------------+-------------+-----"
           "+-----------------+-----------\n");
}

/* "%-30.30s" alinha a esquerda e corta nomes longos em 30 caracteres. */
void imprimirLinhaTabela(const Usuario *u) {
    printf("%-10d | %-30.30s | %011lld | %3d | %-15.15s | %10.2f\n",
           u->matricula, u->nome, u->CPF, u->semestre,
           u->campus, u->valorMensalidade);
}

/*
 * calcularEstatisticas
 * Percorre o vetor uma vez acumulando a soma e guardando as posicoes do
 * maior e do menor valorMensalidade. A soma usa 'double' para reduzir
 * erros de arredondamento ao somar muitos valores 'float'.
 */
void calcularEstatisticas(const ListaUsuarios *lista, Estatisticas *est) {
    int i;

    est->total = lista->quantidade;
    est->soma = 0.0;
    est->media = 0.0;
    est->indiceMaior = -1;
    est->indiceMenor = -1;

    for (i = 0; i < lista->quantidade; i++) {
        float valor = lista->dados[i].valorMensalidade;
        est->soma += valor;

        if (est->indiceMaior == -1 ||
            valor > lista->dados[est->indiceMaior].valorMensalidade) {
            est->indiceMaior = i;
        }
        if (est->indiceMenor == -1 ||
            valor < lista->dados[est->indiceMenor].valorMensalidade) {
            est->indiceMenor = i;
        }
    }

    if (est->total > 0) {
        est->media = est->soma / est->total;
    }
}

/* =====================================================================
 *  MODULOS DO MENU
 * ===================================================================== */

/*
 * [1] cadastrarUsuario
 * Le e valida todos os campos de um novo usuario, impede matricula
 * repetida, insere no vetor dinamico (que cresce com realloc se estiver
 * cheio) e salva o arquivo imediatamente.
 */
void cadastrarUsuario(ListaUsuarios *lista) {
    Usuario novo;

    if (!lerInteiro("Matricula: ", 1, INT_MAX, &novo.matricula)) return;
    if (buscarIndicePorMatricula(lista, novo.matricula) != -1) {
        printf("Ja existe um usuario com a matricula %d. Cadastro cancelado.\n",
               novo.matricula);
        return;
    }

    if (!lerTextoObrigatorio("Nome: ", novo.nome, sizeof(novo.nome))) return;
    if (!lerCPF("CPF (somente numeros ou 000.000.000-00): ", &novo.CPF)) return;
    if (!lerInteiro("Semestre (1 a 20): ", SEMESTRE_MIN, SEMESTRE_MAX,
                    &novo.semestre)) return;
    if (!lerTextoObrigatorio("Campus: ", novo.campus, sizeof(novo.campus))) return;
    if (!lerFloat("Valor da mensalidade (R$): ", 0.0f, MENSALIDADE_MAX,
                  &novo.valorMensalidade)) return;

    if (!adicionarUsuario(lista, &novo)) {
        printf("Nao foi possivel cadastrar: memoria insuficiente.\n");
        return;
    }

    printf("\nUsuario cadastrado com sucesso! (%d/%d posicoes usadas)\n",
           lista->quantidade, lista->capacidade);
    salvarTudo(lista);
}

/*
 * [2] deletarUsuario
 * Localiza o usuario pela matricula, mostra os dados e PEDE CONFIRMACAO
 * antes de remover. Apos remover, salva o arquivo.
 */
void deletarUsuario(ListaUsuarios *lista) {
    int matricula, indice;

    if (lista->quantidade == 0) {
        printf("Nao ha usuarios cadastrados.\n");
        return;
    }

    if (!lerInteiro("Matricula do usuario a deletar: ", 1, INT_MAX,
                    &matricula)) return;

    indice = buscarIndicePorMatricula(lista, matricula);
    if (indice == -1) {
        printf("Matricula %d nao encontrada.\n", matricula);
        return;
    }

    imprimirUsuario(&lista->dados[indice]);
    if (!lerConfirmacao("Confirma a exclusao deste usuario? (S/N): ")) {
        printf("Exclusao cancelada.\n");
        return;
    }

    removerUsuario(lista, indice);
    printf("Usuario removido com sucesso.\n");
    salvarTudo(lista);
}

/*
 * [3] validarCadastro
 * Simula a passagem na catraca: pede a matricula, verifica se o
 * cadastro existe e se os dados estao consistentes (CPF valido,
 * semestre dentro da faixa, campos preenchidos, mensalidade valida).
 * Informa ACESSO LIBERADO ou ACESSO NEGADO com o motivo.
 */
void validarCadastro(const ListaUsuarios *lista) {
    int matricula, indice, problemas = 0;
    const Usuario *u;

    if (!lerInteiro("Matricula para validacao: ", 1, INT_MAX, &matricula)) return;

    indice = buscarIndicePorMatricula(lista, matricula);
    if (indice == -1) {
        printf("\n>>> ACESSO NEGADO: matricula %d nao cadastrada. <<<\n",
               matricula);
        return;
    }

    u = &lista->dados[indice];
    imprimirUsuario(u);

    if (u->nome[0] == '\0') {
        printf("- Problema: nome nao informado.\n");
        problemas++;
    }
    if (!validarCPF(u->CPF)) {
        printf("- Problema: CPF %011lld invalido.\n", u->CPF);
        problemas++;
    }
    if (u->semestre < SEMESTRE_MIN || u->semestre > SEMESTRE_MAX) {
        printf("- Problema: semestre %d fora da faixa (%d a %d).\n",
               u->semestre, SEMESTRE_MIN, SEMESTRE_MAX);
        problemas++;
    }
    if (u->campus[0] == '\0') {
        printf("- Problema: campus nao informado.\n");
        problemas++;
    }
    if (u->valorMensalidade < 0.0f) {
        printf("- Problema: valor de mensalidade negativo.\n");
        problemas++;
    }

    if (problemas == 0) {
        printf("\n>>> ACESSO LIBERADO: cadastro valido. Bem-vindo(a), %s! <<<\n",
               u->nome);
    } else {
        printf("\n>>> ACESSO NEGADO: %d problema(s) no cadastro. "
               "Use a opcao 4 para corrigir. <<<\n", problemas);
    }
}

/*
 * [4] alterarRegistro
 * Localiza o usuario pela matricula e permite alterar um ou mais campos
 * em sequencia. As alteracoes sao feitas em uma COPIA do registro e so
 * gravadas no vetor (e no arquivo) quando o usuario conclui.
 */
void alterarRegistro(ListaUsuarios *lista) {
    int matricula, indice, campo, novaMatricula, alterou = 0;
    Usuario copia;

    if (lista->quantidade == 0) {
        printf("Nao ha usuarios cadastrados.\n");
        return;
    }

    if (!lerInteiro("Matricula do usuario a alterar: ", 1, INT_MAX,
                    &matricula)) return;

    indice = buscarIndicePorMatricula(lista, matricula);
    if (indice == -1) {
        printf("Matricula %d nao encontrada.\n", matricula);
        return;
    }

    copia = lista->dados[indice];

    do {
        imprimirUsuario(&copia);
        printf("Qual campo deseja alterar?\n");
        printf("1 - Nome\n2 - CPF\n3 - Semestre\n4 - Campus\n");
        printf("5 - Valor da mensalidade\n6 - Matricula\n");
        printf("0 - Concluir alteracoes\n");
        if (!lerInteiro("Campo: ", 0, 6, &campo)) {
            campo = -1; /* entrada encerrada: sai sem salvar */
            break;
        }

        switch (campo) {
            case 1:
                if (lerTextoObrigatorio("Novo nome: ", copia.nome,
                                        sizeof(copia.nome))) alterou = 1;
                break;
            case 2:
                if (lerCPF("Novo CPF: ", &copia.CPF)) alterou = 1;
                break;
            case 3:
                if (lerInteiro("Novo semestre (1 a 20): ", SEMESTRE_MIN,
                               SEMESTRE_MAX, &copia.semestre)) alterou = 1;
                break;
            case 4:
                if (lerTextoObrigatorio("Novo campus: ", copia.campus,
                                        sizeof(copia.campus))) alterou = 1;
                break;
            case 5:
                if (lerFloat("Novo valor da mensalidade (R$): ", 0.0f,
                             MENSALIDADE_MAX, &copia.valorMensalidade)) alterou = 1;
                break;
            case 6:
                if (lerInteiro("Nova matricula: ", 1, INT_MAX, &novaMatricula)) {
                    int dono = buscarIndicePorMatricula(lista, novaMatricula);
                    if (dono != -1 && dono != indice) {
                        printf("A matricula %d ja pertence a outro usuario.\n",
                               novaMatricula);
                    } else {
                        copia.matricula = novaMatricula;
                        alterou = 1;
                    }
                }
                break;
        }
    } while (campo != 0);

    if (campo == 0 && alterou) {
        lista->dados[indice] = copia; /* grava a copia no vetor */
        printf("Registro alterado com sucesso.\n");
        salvarTudo(lista);
    } else {
        printf("Nenhuma alteracao foi gravada.\n");
    }
}

/*
 * [5] resumoUsuarios
 * Lista todos os usuarios em formato de tabela, mostra os dados gerais
 * (total, soma, media, maior e menor mensalidade, uso da memoria) e
 * gera/atualiza o arquivo relatorio.txt.
 */
void resumoUsuarios(const ListaUsuarios *lista) {
    Estatisticas est;
    int i;

    if (lista->quantidade == 0) {
        printf("Nao ha usuarios cadastrados.\n");
    } else {
        imprimirCabecalhoTabela();
        for (i = 0; i < lista->quantidade; i++) {
            imprimirLinhaTabela(&lista->dados[i]);
        }

        calcularEstatisticas(lista, &est);
        printf("\nTotal de registros ....: %d\n", est.total);
        printf("Soma das mensalidades .: R$ %.2f\n", est.soma);
        printf("Media das mensalidades : R$ %.2f\n", est.media);
        printf("Maior mensalidade .....: R$ %.2f (%s)\n",
               lista->dados[est.indiceMaior].valorMensalidade,
               lista->dados[est.indiceMaior].nome);
        printf("Menor mensalidade .....: R$ %.2f (%s)\n",
               lista->dados[est.indiceMenor].valorMensalidade,
               lista->dados[est.indiceMenor].nome);
    }

    printf("Memoria do vetor ......: %d de %d posicoes usadas\n",
           lista->quantidade, lista->capacidade);

    if (gerarRelatorio(lista, ARQUIVO_RELATORIO)) {
        printf("Relatorio gravado em '%s'.\n", ARQUIVO_RELATORIO);
    }
}

/*
 * [6] buscarUsuario
 * Busca por matricula (resultado exato) ou por nome (parte do nome,
 * sem diferenciar maiusculas/minusculas, podendo retornar varios).
 */
void buscarUsuario(const ListaUsuarios *lista) {
    int tipo, matricula, indice, i, encontrados = 0;
    char trecho[TAM_NOME];

    if (lista->quantidade == 0) {
        printf("Nao ha usuarios cadastrados.\n");
        return;
    }

    printf("1 - Buscar por matricula\n2 - Buscar por nome\n");
    if (!lerInteiro("Tipo de busca: ", 1, 2, &tipo)) return;

    if (tipo == 1) {
        if (!lerInteiro("Matricula: ", 1, INT_MAX, &matricula)) return;
        indice = buscarIndicePorMatricula(lista, matricula);
        if (indice == -1) {
            printf("Nenhum usuario com a matricula %d.\n", matricula);
        } else {
            imprimirUsuario(&lista->dados[indice]);
        }
        return;
    }

    if (!lerTextoObrigatorio("Nome (ou parte dele): ", trecho,
                             sizeof(trecho))) return;

    for (i = 0; i < lista->quantidade; i++) {
        if (contemSemDiferenciarCaixa(lista->dados[i].nome, trecho)) {
            if (encontrados == 0) {
                imprimirCabecalhoTabela();
            }
            imprimirLinhaTabela(&lista->dados[i]);
            encontrados++;
        }
    }

    if (encontrados == 0) {
        printf("Nenhum usuario encontrado com \"%s\" no nome.\n", trecho);
    } else {
        printf("\n%d usuario(s) encontrado(s).\n", encontrados);
    }
}
