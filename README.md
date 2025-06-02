# Algoritmo de Tomasulo

## Introdução

O Algoritmo de Tomasulo é uma técnica de execução dinâmica de instruções desenvolvida por Robert Tomasulo em 1967 para o IBM System/360 Model 91. Este algoritmo revolucionou a execução de instruções em processadores superescalares, permitindo:

- Execução fora de ordem (out-of-order execution)
- Renomeação dinâmica de registradores
- Resolução dinâmica de dependências
- Execução paralela de instruções independentes

O algoritmo utiliza um buffer de reordenação (ROB) e estações de reserva para gerenciar a execução das instruções, eliminando hazards de dados e permitindo um melhor aproveitamento dos recursos do processador.

## Tratamento de Conflitos

O simulador implementa mecanismos específicos para lidar com os três tipos principais de conflitos de dados:

### 1. RAW (Read After Write)
- **Detecção**: Ocorre quando uma instrução tenta ler um registrador antes que seu valor seja atualizado por uma instrução anterior
- **Tratamento**:
  - Uso de tags do ROB para rastrear dependências
  - Quando uma instrução lê um registrador, ela recebe a tag do produtor
  - A instrução só executa quando o valor do produtor estiver disponível
  - Implementado em `issueInstruction()` através do sistema de tags (qj/qk)

### 2. WAW (Write After Write)
- **Detecção**: Ocorre quando duas instruções tentam escrever no mesmo registrador
- **Tratamento**:
  - Uso do ROB para garantir ordem de commit
  - Cada escrita é associada a uma entrada no ROB
  - Commits são feitos em ordem, garantindo a sequência correta
  - Implementado em `commitInstruction()` através do controle de ordem do ROB

### 3. WAR (Write After Read)
- **Detecção**: Ocorre quando uma instrução tenta escrever em um registrador antes que uma instrução anterior termine de lê-lo
- **Tratamento**:
  - Renomeação dinâmica de registradores
  - Cada escrita cria uma nova versão do registrador
  - Leituras são associadas à versão correta do registrador
  - Implementado através do sistema de tags e renomeação em `issueInstruction()`

### Mecanismos de Implementação

1. **Sistema de Tags**
   - Cada estação de reserva mantém tags (qj/qk) para operandos
   - Tags identificam a entrada do ROB que produzirá o valor
   - Permite rastrear dependências entre instruções

2. **Renomeação de Registradores**
   - Implementada através do campo `producer_tag` na estrutura Register
   - Cada escrita cria uma nova versão do registrador
   - Leituras são associadas à versão correta através das tags

3. **Buffer de Reordenação (ROB)**
   - Mantém ordem de commit das instruções
   - Garante que escritas sejam feitas na ordem correta
   - Permite execução fora de ordem mantendo a semântica do programa

4. **Common Data Bus (CDB)**
   - Broadcast de resultados para todas as estações
   - Atualização imediata de dependências
   - Resolução dinâmica de conflitos

## Implementação

Este projeto implementa uma simulação do Algoritmo de Tomasulo em C++. A implementação inclui todas as principais características do algoritmo original, com algumas simplificações para fins educacionais.

### Estruturas de Dados Principais

1. **Instrução (Instruction)**
   - Representa uma instrução do programa
   - Contém informações como operação, registradores fonte e destino
   - Rastreia ciclos de emissão, execução e commit

2. **Estação de Reserva (ReservationStation)**
   - Gerencia a execução de uma instrução
   - Armazena operandos e tags de dependências
   - Controla o estado de execução da instrução

3. **Registrador (Register)**
   - Implementa renomeação dinâmica de registradores
   - Mantém valor atual e tag do produtor
   - Controla estado de prontidão e ocupação

4. **Buffer de Reordenação (ReorderBufferEntry)**
   - Gerencia o estado de cada instrução em execução
   - Mantém valores e endereços para commit
   - Controla a ordem de commit das instruções

### Componentes Principais

1. **Estações de Reserva**
   - ADD/SUB: 3 estações
   - MUL/DIV: 2 estações
   - LOAD/STORE: 2 estações cada

2. **Banco de Registradores**
   - 32 registradores inteiros (R0-R31)
   - 32 registradores de ponto flutuante (F0-F31)

3. **Memória**
   - Simulação de memória com 1024 posições
   - Suporte a operações LOAD/STORE

4. **Common Data Bus (CDB)**
   - Canal de comunicação para resultados
   - Permite broadcast de resultados para dependências

### Funções Principais

1. **issueInstruction()**
   - Emite instruções para estações de reserva
   - Aloca entradas no ROB
   - Gerencia renomeação de registradores
   - Verifica hazards estruturais e de dados

2. **executeInstructions()**
   - Executa instruções nas estações de reserva
   - Calcula resultados das operações
   - Adiciona resultados ao CDB
   - Gerencia latências das operações

3. **processWriteBack()**
   - Processa resultados do CDB
   - Atualiza registradores e estações dependentes
   - Libera estações de reserva
   - Atualiza estado do ROB

4. **commitInstruction()**
   - Commita instruções em ordem
   - Atualiza banco de registradores
   - Atualiza memória para STORE
   - Libera entradas do ROB

5. **printState()**
   - Exibe estado detalhado da simulação
   - Mostra estado das estações de reserva
   - Exibe conteúdo dos registradores
   - Mostra estado do ROB e CDB

### Latências das Operações

- ADD/SUB: 2 ciclos
- MUL: 10 ciclos
- DIV: 40 ciclos
- LOAD/STORE: 3 ciclos

### Formato de Entrada

O simulador aceita instruções no formato:
```
ADD F1 F2 F3    # F1 = F2 + F3
SUB F4 F1 F5    # F4 = F1 - F5
MUL F6 F2 F4    # F6 = F2 * F4
DIV F7 F6 F3    # F7 = F6 / F3
LOAD F8 0(R1)   # F8 = Mem[R1 + 0]
STORE F2 4(R2)  # Mem[R2 + 4] = F2
```

### Execução

1. Carrega instruções do arquivo
2. Emite instruções para estações de reserva
3. Executa instruções em paralelo
4. Processa resultados através do CDB
5. Commita instruções em ordem
6. Exibe estado detalhado a cada ciclo

### Limitações e Simplificações

1. Tamanho fixo do ROB (16 entradas)
2. Número fixo de estações de reserva
3. Memória simplificada
4. Sem suporte a branches/loops
5. Sem cache ou hierarquia de memória

### Uso

1. Compile o programa:
```bash
g++ main.cpp -o tomasulo
```

2. Execute com um arquivo de instruções:
```bash
./tomasulo
```

3. Digite o nome do arquivo de instruções quando solicitado

### Saída

O simulador exibe o estado detalhado a cada ciclo, incluindo:
- Estado das estações de reserva
- Conteúdo dos registradores
- Estado do ROB
- Conteúdo do CDB
- Instruções em execução
- Conteúdo da memória

## 🧩 Colaboradores
| <img src="https://github.com/thomneuenschwander.png" width="100" height="100" alt="Thomas Neuenschwander"/> | <img src="https://github.com/DigoDrummond.png" width="100" height="100" alt="DigoDrummond"/> | <img src="https://github.com/CaioNotini.png" width="100" height="100" alt="CaioNotini "/> | <img src="https://github.com/EduardoAVS.png" width="100" height="100" alt="EduardoAVS "/> |
|:---:|:---:|:---:|:---:|
| [Thomas <br> Neuenschwander](https://github.com/thomneuenschwander) | [Rodrigo <br> Drummond](https://github.com/DigoDrummond) | [Caio Notini](https://github.com/CaioNotini) | [Eduardo Araújo](https://github.com/EduardoAVS) |

## 🧐 Referências

1. **Rodolfo Azevedo** - *Algoritmo de Tomasulo: Trabalho MO401*. Disponível em [Unicamp](https://www.ic.unicamp.br/~rodolfo/Cursos/mo401/2s2005/Trabalho/049239-tomasulo.pdf).
2. **Tomasulo Algorithm** - Documentação e explicação detalhada disponível em [Wikipedia](https://en.wikipedia.org/wiki/Tomasulo_algorithm).