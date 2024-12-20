#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#pragma pack(push, 1)  // Define a configuração de empacotamento para a estrutura seguinte, onde o empacotamento será de 1 byte

typedef struct
{
    char signature[2];      // Armazena a assinatura do arquivo BMP
    uint32_t fileSize;      // Armazena o tamanho total do arquivo
    uint16_t reserved1;     // Campo reservado
    uint16_t reserved2;     // Campo reservado
    uint32_t dataOffset;    // Armazena o deslocamento para os dados da imagem
    uint32_t headerSize;    // Armazena o tamanho do cabeçalho
    int32_t width;          // Armazena a largura da imagem
    int32_t height;         // Armazena a altura da imagem
    uint16_t planes;        // Armazena o número de planos de cor
    uint16_t bitsPerPixel;  // Armazena a quantidade de bits por pixel
    uint32_t compression;   // Armazena o tipo de compressão utilizado
    uint32_t imageSize;     // Armazena o tamanho da imagem
    int32_t xPixelsPerM;    // Armazena o número de pixels por metro no eixo X
    int32_t yPixelsPerM;    // Armazena o número de pixels por metro no eixo Y
    uint32_t colorsUsed;    // Armazena a quantidade de cores usadas
    uint32_t colorsImportant; // Armazena a quantidade de cores importantes
} BMPHeader;

#pragma pack(pop) // Restaura as configurações padrão de empacotamento

void converterParaEscalaDeCinza(BMPHeader *header, unsigned char *data)
{
    int i;
    unsigned char grey;  // Variável para armazenar o valor do pixel em tons de cinza

    for (i = 0; i < header->imageSize; i += 3)// 3 em 3 bytes (considerando a representação de cores RGB)
        // header->imageSize especifica o tamanho total dos dados da imagem sem contar o cabecalho
    {
        // Fórmula para converter para tons de cinza (0.3R + 0.59G + 0.11B)
        // Calcula o valor de cinza usando os pesos 0.3, 0.59 e 0.11 para R, G e B, respectivamente
        grey = (unsigned char)((0.3 * data[i + 2]) + (0.59 * data[i + 1]) + (0.11 * data[i]));

        // Define o valor de tons de cinza (grey) para os canais RGB do pixel atual
        data[i] = data[i + 1] = data[i + 2] = grey;
    }
}

void recortarImagem(BMPHeader *header, unsigned char *data, int x, int y, int limiar)
{
    int i, j;
    // Dimensões do recorte
    int recorteWidth = 84;
    int recorteHeight = 48;
    int posOriginal, posRecorte;
    FILE *recorteFile;

    if (x < 0 || y < 0 || x + recorteWidth > header->width || y + recorteHeight > header->height) // Verifica se o recorte cabe dentro da imagem
    {
        printf("Coordenadas inválidas para o recorte.\n");
        return;
    }

    // Cria um novo cabeçalho para a imagem recortada baseado no cabeçalho original
    BMPHeader recorteHeader = *header;
    recorteHeader.width = recorteWidth;
    recorteHeader.height = recorteHeight;
    recorteHeader.imageSize = recorteWidth * recorteHeight * (header->bitsPerPixel / 8);
    recorteHeader.fileSize = recorteHeader.imageSize + sizeof(BMPHeader);

    // Aloca memória para os dados do recorte
    unsigned char *recorteData = (unsigned char *)malloc(recorteHeader.imageSize);
    if (recorteData == NULL)
    {
        printf("Erro ao alocar memória para o recorte.\n");
        return;
    }

    // Copia os pixels do recorte, aplicando o limiar para converter em escala de cinza
    // Itera pelas linhas do recorte
    for (i = 0; i < recorteHeight; i++)
    {
        for (j = 0; j < recorteWidth; j++)
        {
            // Calcula as posições do pixel no recorte e na imagem original
            posOriginal = ((header->height - 1 - (y + i)) * header->width + x + j) * (header->bitsPerPixel / 8);
            posRecorte = (i * recorteWidth + j) * (header->bitsPerPixel / 8);

            // Extrai os componentes de cor (RGB) do pixel original da imagem
            unsigned char blue = data[posOriginal]; // azul na posicao inicial
            unsigned char green = data[posOriginal + 1]; // verde na posicao inicial
            unsigned char red = data[posOriginal + 2]; // vermelho na posicao inicial

            // Converte para tons de cinza baseado no limiar fornecido
            unsigned char grey = (red * 0.3) + (green * 0.59) + (blue * 0.11);

            // Verifica se o tom de cinza está acima do limiar fornecido
            // Define o valor do pixel do recorte como branco ou preto baseado nesse limiar
            if (grey > limiar)
            {
                grey = 255; // Branco
            }
            else
            {
                grey = 0;   // Preto
            }

            // Atribui o valor de tons de cinza aos canais RGB do pixel do recorte
            recorteData[posRecorte] = recorteData[posRecorte + 1] = recorteData[posRecorte + 2] = grey;
        }
    }

    recorteFile = fopen("recorte.bmp", "wb");
    if (recorteFile == NULL)
    {
        printf("Erro ao criar o arquivo de recorte.\n");
        free(recorteData);
        return;
    }

    fwrite(&recorteHeader, sizeof(BMPHeader), 1, recorteFile);
    fwrite(recorteData, recorteHeader.imageSize, 1, recorteFile);

    fclose(recorteFile);
    free(recorteData);

    printf("Recorte salvo com sucesso no arquivo recorte.bmp\n");
}

void gerarImagemMonocromatica(unsigned char *recorteData, int limiar) // vetor
{
    int i;

    FILE *arquivoVetor = fopen("imagem_vetor.h", "w");
    if (arquivoVetor == NULL)
    {
        printf("Erro ao criar o arquivo de vetor.\n");
        return;
    }

    fprintf(arquivoVetor, "unsigned char const imagem[504] = {\n");

    for (i = 0; i < 504; i++)
    {
        if (i % 28 == 0)
        {
            fprintf(arquivoVetor, "\n");
        }

        unsigned char pixel = recorteData[i]; // Obtem o valor do pixel atual

        // Verifica o valor do pixel para determinar a cor no arquivo de vetor
        if (pixel <= 0x0A)   // Valores próximos a 0x00
        {
            fprintf(arquivoVetor, "0x00,");
        }
        else if (pixel >= 0xF0)     // Valores próximos a 0xFF
        {
            fprintf(arquivoVetor, "0xFF,");
        }
        else if (pixel >= (limiar - 10) && pixel <= (limiar + 10))     // Valores intermediários
        {
            fprintf(arquivoVetor, "0xF0,");
        }
        else
        {
            fprintf(arquivoVetor, "0x%02X,", pixel); // Outros valores intermediários
        }
    }

    // Escreve o final do array no arquivo e fecha
    fprintf(arquivoVetor, "\n};\n");
    fclose(arquivoVetor);

    printf("Arquivo de vetor imagem_vetor.h criado com sucesso.\n");
}
int main()
{
    char nomeArquivo[50];
    char novoNomeArquivo[60];
    int limiar, xCoord, yCoord;
    BMPHeader header;

    printf("Digite o nome do arquivo desejado: ");
    gets(nomeArquivo);
    strcat(nomeArquivo,".bmp");

    FILE *file = fopen(nomeArquivo, "rb");

    if (file == NULL)
    {
        printf("Nao foi possível abrir o arquivo.\n");
        return 1;
    }

    size_t bytesRead = fread(&header, sizeof(BMPHeader), 1, file);

    if (bytesRead != 1 || header.signature[0] != 'B' || header.signature[1] != 'M')
    {
        printf("O arquivo não é um BMP valido.\n");
        fclose(file);
        return 1;
    }

    unsigned char *data = (unsigned char *)malloc(header.imageSize);
    if (data == NULL)
    {
        printf("Erro ao alocar memoria.\n");
        fclose(file);
        return 1;
    }

    fseek(file, header.dataOffset, SEEK_SET);
    bytesRead = fread(data, header.imageSize, 1, file);

    if (bytesRead != 1)
    {
        printf("Erro ao ler dados da imagem.\n");
        free(data);
        fclose(file);
        return 1;
    }

    converterParaEscalaDeCinza(&header, data);

    strcpy(novoNomeArquivo, nomeArquivo);
    char *ponto = strrchr(novoNomeArquivo, '.');
    *ponto = '\0';
    strcat(novoNomeArquivo, "_gs.bmp");

    FILE *novoArquivo = fopen(novoNomeArquivo, "wb");
    if (novoArquivo == NULL)
    {
        printf("Erro ao criar o novo arquivo.\n");
        free(data);
        fclose(file);
        return 1;
    }

    fwrite(&header, sizeof(BMPHeader), 1, novoArquivo);
    fwrite(data, header.imageSize, 1, novoArquivo);

    printf("\nNome do arquivo: %s\n", nomeArquivo);
    printf("Largura da imagem: %d pixels\n", header.width);
    printf("Altura da imagem: %d pixels\n", header.height);

    printf("\nDigite o limiar para o recorte (entre 0 e 255): ");
    scanf("%d", &limiar);
    printf("Digite as coordenadas X do canto inferior esquerdo: ");
    scanf("%d", &xCoord);
    printf("Digite as coordenadas Y do canto inferior esquerdo: ");
    scanf("%d", &yCoord);
    printf("\n");

    recortarImagem(&header, data, xCoord, yCoord, limiar);
    gerarImagemMonocromatica(data, limiar);

    fclose(novoArquivo);
    free(data);
    fclose(file);

    printf("Imagem convertida para escala de cinza e salva como: %s\n", novoNomeArquivo);

    return 0;
}
