# Simulador do Algoritimo de Tomasulo
Este projeto é desenvolvido no âmbito da disciplina **Arquitetura de Computadores III** do curso de **Ciências da Computação** da [**PUC Minas**](https://www.pucminas.br/destaques/Paginas/default.aspx).

## 🤓 Contexto
Processadores modernos utilizam técnicas de pipeline para executar múltiplas instruções simultaneamente, explorando o **Paralelismo em Nível de Instrução (ILP)** para aumentar o desempenho. Em arquiteturas superescalares, múltiplas instruções podem ser despachadas e executadas em cada ciclo de clock.

No entanto, essa execução paralela pode levar a conflitos de dados, categorizados como:
- `RAW (Read-After-Write)`: Uma instrução tenta ler um operando antes que a instrução anterior que o escreve tenha completado.
- `WAW (Write-After-Write)`: Duas instruções tentam escrever no mesmo local, e a ordem de escrita precisa ser preservada.
- `WAR (Write-After-Read)`: Uma instrução tenta escrever em um local antes que uma instrução anterior que o lê tenha completado sua leitura.


Para gerenciar esses conflitos e otimizar o ILP, existem duas abordagens principais de escalonamento de instruções:
- **Escalonamento Estático**: Realizado em **tempo de compilação pelo compilador**, que reordena as instruções para evitar conflitos e maximizar o uso dos recursos do processador.
- **Escalonamento Dinâmico**: Realizado em **tempo de execução pelo hardware** do processador. Esta abordagem permite que o processador reordene as instruções com base no estado atual dos operandos e das unidades funcionais.

O **Algoritmo de Tomasulo** é um exemplo de **escalonamento dinâmico**. Ele utiliza estações de reserva e um barramento comum de dados (CDB) para permitir que instruções sejam executadas fora de ordem assim que seus operandos estejam disponíveis, resolvendo dinamicamente os conflitos de dados e aumentando significativamente a eficiência do pipeline.

**Este projeto implementa um simulador do Algoritmo de Tomasulo** para demonstrar e analisar seu funcionamento na gestão de instruções e na resolução de dependências em um ambiente superescalar.

## 🤔 Como Funciona

## 🚀 Como Usar

## 🧩 Colaboradores
| <img src="https://github.com/thomneuenschwander.png" width="100" height="100" alt="Thomas Neuenschwander"/> | <img src="https://github.com/DigoDrummond.png" width="100" height="100" alt="DigoDrummond"/> | <img src="https://github.com/CaioNotini.png" width="100" height="100" alt="CaioNotini "/> | <img src="https://github.com/EduardoAVS.png" width="100" height="100" alt="EduardoAVS "/> |
|:---:|:---:|:---:|:---:|
| [Thomas <br> Neuenschwander](https://github.com/thomneuenschwander) | [Rodrigo <br> Drummond](https://github.com/DigoDrummond) | [Caio Notini](https://github.com/CaioNotini) | [Eduardo Araújo](https://github.com/EduardoAVS) |

## 🧐 Referências

1. **Rodolfo Azevedo** - *Algoritmo de Tomasulo: Trabalho MO401*. Disponível em [Unicamp](https://www.ic.unicamp.br/~rodolfo/Cursos/mo401/2s2005/Trabalho/049239-tomasulo.pdf).
2. **Tomasulo Algorithm** - Documentação e explicação detalhada disponível em [Wikipedia](https://en.wikipedia.org/wiki/Tomasulo_algorithm).