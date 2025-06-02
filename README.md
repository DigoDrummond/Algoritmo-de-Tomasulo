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

## Implementação Detalhada

### 1. Estruturas de Dados

#### Instruction
```cpp
struct Instruction {
    int id;                 // Identificador único da instrução
    OpType op;             // Tipo de operação (ADD, SUB, MUL, etc.)
    string dest;           // Registrador destino
    string src1, src2;     // Registradores fonte
    int address;           // Endereço para LOAD/STORE
    InstrState state;      // Estado atual da instrução
    int issue_cycle;       // Ciclo de emissão
    int exec_start_cycle;  // Ciclo de início de execução
    int exec_end_cycle;    // Ciclo de fim de execução
    int write_cycle;       // Ciclo de write-back
    int commit_cycle;      // Ciclo de commit
};
```

#### ReservationStation
```cpp
struct ReservationStation {
    bool busy;             // Indica se a estação está ocupada
    OpType op;             // Operação a ser executada
    int instr_id;          // ID da instrução em execução
    string vj, vk;         // Valores dos operandos
    string qj, qk;         // Tags das estações produtoras
    string dest;           // Tag do ROB destino
    int dest_reg;          // Número do registrador destino
    int cycles_left;       // Ciclos restantes para execução
    int address;           // Endereço para LOAD/STORE
};
```

#### Register
```cpp
struct Register {
    float value;           // Valor atual do registrador
    string producer_tag;   // Tag da estação que produzirá o valor
    bool ready;           // Indica se o valor está pronto
    bool busy;            // Indica se o registrador está ocupado
};
```

#### ReorderBufferEntry
```cpp
struct ReorderBufferEntry {
    bool busy;             // Indica se a entrada está ocupada
    int instruction_index; // Índice da instrução
    OpType type;          // Tipo de operação
    string state;         // Estado atual (EMPTY, ISSUE, EXECUTE, WRITE_RESULT)
    string destination_register; // Registrador destino
    float value;          // Valor calculado
    int address;          // Endereço para LOAD/STORE
    bool value_ready;     // Indica se o valor está pronto
};
```

### 2. Componentes do Sistema

#### Estações de Reserva
- **ADD/SUB (3 estações)**
  - Executam operações aritméticas básicas
  - Latência de 2 ciclos
  - Suportam renomeação de registradores

- **MUL/DIV (2 estações)**
  - Executam operações de multiplicação e divisão
  - Latências diferentes (MUL: 10 ciclos, DIV: 40 ciclos)
  - Implementam divisão por zero

- **LOAD/STORE (2 estações cada)**
  - Gerenciam acesso à memória
  - Latência de 3 ciclos
  - Suportam endereçamento base + offset

#### Banco de Registradores
- **Registradores Inteiros (R0-R31)**
  - Valores iniciais aleatórios
  - Suporte a renomeação dinâmica
  - Rastreamento de dependências

- **Registradores de Ponto Flutuante (F0-F31)**
  - Mesmas características dos inteiros
  - Usados para operações aritméticas
  - Suporte a precisão dupla

#### Memória
- **1024 posições**
  - Acesso aleatório
  - Suporte a LOAD/STORE
  - Endereçamento base + offset

#### Common Data Bus (CDB)
- **Broadcast de resultados**
  - Estrutura: `vector<tuple<int, float, string>>`
  - Contém: ID da instrução, valor, tag do ROB
  - Atualização imediata de dependências

### 3. Funções Principais

#### issueInstruction()
```cpp
bool issueInstruction() {
    // 1. Verificar disponibilidade
    if (instruction_queue.empty() || rob_entries_available == 0) 
        return false;
    
    // 2. Verificar hazards
    if (checkHazards(instr)) 
        return false;
    
    // 3. Alocar entrada no ROB
    int current_rob_idx = rob_tail;
    ReorderBufferEntry& rob_entry = rob[current_rob_idx];
    
    // 4. Configurar estação de reserva
    ReservationStation* station = getStation(instr.op);
    
    // 5. Renomear registradores
    if (instr.op != STORE) {
        registers[instr.dest].producer_tag = to_string(current_rob_idx);
    }
    
    // 6. Configurar operandos
    setupOperands(station, instr);
    
    return true;
}
```

#### executeInstructions()
```cpp
void executeInstructions() {
    for (auto& instr : executing_instructions) {
        // 1. Decrementar ciclos
        instr.remaining_cycles--;
        
        // 2. Verificar conclusão
        if (instr.remaining_cycles <= 0) {
            // 3. Calcular resultado
            float result = calculateResult(instr);
            
            // 4. Adicionar ao CDB
            completed_for_cdb.push_back({
                instr.instruction_id,
                result,
                instr.station->dest
            });
        }
    }
}
```

#### processWriteBack()
```cpp
void processWriteBack() {
    if (completed_for_cdb.empty()) return;
    
    // 1. Obter resultado do CDB
    auto [instr_id, result, rob_tag] = completed_for_cdb.front();
    
    // 2. Atualizar ROB
    ReorderBufferEntry& rob_entry = rob[stoi(rob_tag)];
    rob_entry.value = result;
    rob_entry.value_ready = true;
    
    // 3. Atualizar dependências
    updateDependencies(rob_tag, result);
    
    // 4. Liberar estação
    releaseStation(rob_tag);
}
```

#### commitInstruction()
```cpp
void commitInstruction() {
    if (!rob[rob_head].busy) return;
    
    ReorderBufferEntry& head = rob[rob_head];
    
    // 1. Verificar prontidão
    if (head.state == "WRITE_RESULT" && head.value_ready) {
        // 2. Atualizar registrador/memória
        if (head.type != STORE) {
            registers[head.destination_register].value = head.value;
        } else {
            memory[head.address] = head.value;
        }
        
        // 3. Liberar entrada do ROB
        head.busy = false;
        rob_head = (rob_head + 1) % rob_size;
        rob_entries_available++;
    }
}
```

### 4. Fluxo de Execução

1. **Carregamento de Instruções**
   - Leitura do arquivo de entrada
   - Parsing das instruções
   - Inicialização das estruturas

2. **Emissão**
   - Verificação de hazards
   - Alocação no ROB
   - Configuração da estação de reserva
   - Renomeação de registradores

3. **Execução**
   - Contagem de ciclos
   - Cálculo de resultados
   - Broadcast pelo CDB

4. **Write-Back**
   - Processamento do CDB
   - Atualização de dependências
   - Liberação de recursos

5. **Commit**
   - Verificação de ordem
   - Atualização de registradores
   - Atualização de memória
   - Liberação do ROB

### 5. Detalhes de Implementação

#### Gerenciamento de Dependências
- Uso de tags do ROB para rastrear dependências
- Sistema de broadcast pelo CDB
- Atualização imediata de estações dependentes

#### Renomeação de Registradores
- Implementação através do producer_tag
- Criação de novas versões a cada escrita
- Associação de leituras com versões corretas

#### Controle de Ordem
- Uso do ROB para garantir ordem de commit
- Execução fora de ordem permitida
- Commit em ordem sequencial

#### Tratamento de Erros
- Verificação de divisão por zero
- Validação de endereços de memória
- Controle de overflow do ROB

## 🧩 Colaboradores
| <img src="https://github.com/thomneuenschwander.png" width="100" height="100" alt="Thomas Neuenschwander"/> | <img src="https://github.com/DigoDrummond.png" width="100" height="100" alt="DigoDrummond"/> | <img src="https://github.com/CaioNotini.png" width="100" height="100" alt="CaioNotini "/> | <img src="https://github.com/EduardoAVS.png" width="100" height="100" alt="EduardoAVS "/> |
|:---:|:---:|:---:|:---:|
| [Thomas <br> Neuenschwander](https://github.com/thomneuenschwander) | [Rodrigo <br> Drummond](https://github.com/DigoDrummond) | [Caio Notini](https://github.com/CaioNotini) | [Eduardo Araújo](https://github.com/EduardoAVS) |

## 🧐 Referências

1. **Rodolfo Azevedo** - *Algoritmo de Tomasulo: Trabalho MO401*. Disponível em [Unicamp](https://www.ic.unicamp.br/~rodolfo/Cursos/mo401/2s2005/Trabalho/049239-tomasulo.pdf).
2. **Tomasulo Algorithm** - Documentação e explicação detalhada disponível em [Wikipedia](https://en.wikipedia.org/wiki/Tomasulo_algorithm).