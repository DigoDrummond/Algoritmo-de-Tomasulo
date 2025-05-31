#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace std;

// Tipos de operacao
enum OpType {
    ADD, SUB, MUL, DIV, LOAD, STORE
};

// Estado da instrucao
enum InstrState {
    ISSUED, EXECUTING, WRITE_RESULT, COMMITTED
};

// Estrutura da instrucao
struct Instruction {
    int id;
    OpType op;
    string dest;
    string src1;
    string src2;
    int address; // Para LOAD/STORE
    InstrState state;
    int issue_cycle;
    int exec_start_cycle;
    int exec_end_cycle;
    int write_cycle;
    
    Instruction(int _id, OpType _op, string _dest, string _src1, string _src2 = "", int _addr = 0)
        : id(_id), op(_op), dest(_dest), src1(_src1), src2(_src2), address(_addr), 
          state(ISSUED), issue_cycle(-1), exec_start_cycle(-1), exec_end_cycle(-1), write_cycle(-1) {}
};

// Estação de reserva
struct ReservationStation {
    bool busy;
    OpType op;
    int instr_id;
    string vj, vk; // Valores dos operandos
    string qj, qk; // Tags das estações produtoras
    string dest;   // Registrador destino
    int dest_reg;  // Número do registrador destino
    int cycles_left; // Ciclos restantes para execução
    int address; // Para LOAD/STORE
    
    ReservationStation() : busy(false), instr_id(-1), dest_reg(-1), cycles_left(0), address(0) {}
};

// Registrador com renomeacao
struct Register {
    float value;
    string producer_tag; // Tag da estação que vai produzir o valor
    bool ready;
    bool busy; // Indica se o registrador está ocupado
    
    Register() : value(0.0), producer_tag(""), ready(true), busy(false) {}
};

// Instrucao em execucao
struct ExecutingInstruction {
    int station_idx;
    string station_type;
    int remaining_cycles;
    int instruction_id;
    
    ExecutingInstruction(int idx, string type, int cycles, int id)
        : station_idx(idx), station_type(type), remaining_cycles(cycles), instruction_id(id) {}
};

class TomasuloSimulator {
private:
    // Estações de reserva
    vector<ReservationStation> add_stations;
    vector<ReservationStation> mult_stations;
    vector<ReservationStation> load_stations;
    vector<ReservationStation> store_stations;
    
    // Banco de registradores
    map<string, Register> registers;
    
    // Fila de instrucoes
    queue<Instruction> instruction_queue;
    vector<Instruction> all_instructions;
    
    // Controle de ciclos
    int current_cycle;
    
    // Latências das operacoes
    map<OpType, int> latencies;
    
    // Buffer de resultados (Common Data Bus)
    vector<pair<string, float>> cdb_buffer;
    
    // Memória simulada
    vector<float> memory;
    
    // Instrucoes em execucao
    vector<ExecutingInstruction> executing_instructions;

public:
    TomasuloSimulator() : current_cycle(1) {
        // Inicializar estações de reserva
        add_stations.resize(3);
        mult_stations.resize(2);
        load_stations.resize(2);
        store_stations.resize(2);
        
        // Definir latências
        latencies[ADD] = 2;
        latencies[SUB] = 2;
        latencies[MUL] = 4;
        latencies[DIV] = 8;
        latencies[LOAD] = 3;
        latencies[STORE] = 3;
        
        // Inicializar registradores com valores aleatórios
        srand(time(0)); // Inicializa o gerador de números aleatórios
        for (int i = 0; i < 32; i++) {
            // Gera um número aleatório entre 0 e 9, multiplica por 10
            float random_value = (rand() % 10) * 10.0;
            
            // Inicializa registradores F e R com o mesmo valor
            registers["R" + to_string(i)] = Register();
            registers["F" + to_string(i)] = Register();
            
            registers["R" + to_string(i)].value = random_value;
            registers["F" + to_string(i)].value = random_value;
        }
        
        // Inicializar memória
        memory.resize(1024, 0.0);
    }
    
    // Carregar instrucoes do arquivo
    bool loadInstructions(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cout << "Erro: Nao foi possível abrir o arquivo " << filename << endl;
            return false;
        }
        
        string line;
        int instr_id = 1;
        
        while (getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            istringstream iss(line);
            string op_str, dest, src1, src2;
            
            iss >> op_str >> dest >> src1 >> src2;
            
            OpType op;
            if (op_str == "ADD") op = ADD;
            else if (op_str == "SUB") op = SUB;
            else if (op_str == "MUL") op = MUL;
            else if (op_str == "DIV") op = DIV;
            else if (op_str == "LOAD") op = LOAD;
            else if (op_str == "STORE") op = STORE;
            else continue;
            
            // Remover vírgulas
            dest.erase(remove(dest.begin(), dest.end(), ','), dest.end());
            src1.erase(remove(src1.begin(), src1.end(), ','), src1.end());
            src2.erase(remove(src2.begin(), src2.end(), ','), src2.end());
            
            Instruction instr(instr_id++, op, dest, src1, src2);
            instruction_queue.push(instr);
            all_instructions.push_back(instr);
        }
        
        file.close();
        return true;
    }
    
    // Verificar se há estação de reserva disponível
    int findFreeStation(OpType op) {
        vector<ReservationStation>* stations;
        
        switch (op) {
            case ADD:
            case SUB:
                stations = &add_stations;
                break;
            case MUL:
            case DIV:
                stations = &mult_stations;
                break;
            case LOAD:
                stations = &load_stations;
                break;
            case STORE:
                stations = &store_stations;
                break;
        }
        
        for (int i = 0; i < stations->size(); i++) {
            if (!(*stations)[i].busy) {
                return i;
            }
        }
        return -1;
    }
    
    // Verificar hazards
    bool checkHazards(const Instruction& instr) {
        // Check for RAW hazards
        if (!registers[instr.src1].ready || !registers[instr.src2].ready) {
            return true;
        }
        
        // Check for WAW hazards
        if (instr.op != STORE && registers[instr.dest].busy) {
            return true;
        }
        
        return false;
    }
    
    // Emitir instrucao
    bool issueInstruction() {
        if (instruction_queue.empty()) return false;
        
        Instruction& instr = instruction_queue.front();
        
        // Verificar hazards antes de emitir
        if (checkHazards(instr)) return false;
        
        int station_idx = findFreeStation(instr.op);
        if (station_idx == -1) return false; // Hazard estrutural
        
        // Selecionar estação de reserva apropriada
        ReservationStation* station;
        string station_name;
        
        switch (instr.op) {
            case ADD:
            case SUB:
                station = &add_stations[station_idx];
                station_name = "Add" + to_string(station_idx + 1);
                break;
            case MUL:
            case DIV:
                station = &mult_stations[station_idx];
                station_name = "Mult" + to_string(station_idx + 1);
                break;
            case LOAD:
                station = &load_stations[station_idx];
                station_name = "Load" + to_string(station_idx + 1);
                break;
            case STORE:
                station = &store_stations[station_idx];
                station_name = "Store" + to_string(station_idx + 1);
                break;
        }
        
        // Configurar estação de reserva
        station->busy = true;
        station->op = instr.op;
        station->instr_id = instr.id;
        station->cycles_left = latencies[instr.op];
        station->dest = instr.dest;  // Adicionar o registrador destino
        
        // Configurar endereco para LOAD/STORE
        if (instr.op == LOAD || instr.op == STORE) {
            // Extrair offset e registrador base do formato offset(Rbase)
            size_t open_paren = instr.src1.find('(');
            size_t close_paren = instr.src1.find(')');
            
            if (open_paren != string::npos && close_paren != string::npos) {
                int offset = stoi(instr.src1.substr(0, open_paren));
                string base_reg = instr.src1.substr(open_paren + 1, close_paren - open_paren - 1);
                station->address = offset + static_cast<int>(registers[base_reg].value);
            }
        }
        
        // Verificar dependências e renomear registradores
        if (instr.op != STORE) {
            // Para instrucoes que escrevem em registrador
            registers[instr.dest].producer_tag = station_name;
            registers[instr.dest].ready = false;
            registers[instr.dest].busy = true;
        }
        
        // Configurar operandos
        if (registers[instr.src1].ready) {
            station->vj = to_string(registers[instr.src1].value);
            station->qj = "";
        } else {
            station->vj = "";
            station->qj = registers[instr.src1].producer_tag;
        }
        
        if (!instr.src2.empty() && instr.op != LOAD && instr.op != STORE) {
            if (registers[instr.src2].ready) {
                station->vk = to_string(registers[instr.src2].value);
                station->qk = "";
            } else {
                station->vk = "";
                station->qk = registers[instr.src2].producer_tag;
            }
        }
        
        // Adicionar à lista de instrucoes em execucao
        executing_instructions.push_back(ExecutingInstruction(
            station_idx,
            station_name,
            latencies[instr.op],
            instr.id
        ));
        
        instr.issue_cycle = current_cycle;
        instruction_queue.pop();
        
        return true;
    }
    
    // Executar instrucoes
    void executeInstructions() {
        // Verificar todas as estações de reserva
        vector<vector<ReservationStation>*> all_stations = {
            &add_stations, &mult_stations, &load_stations, &store_stations
        };
        
        // Processar instrucoes em execucao
        for (auto it = executing_instructions.begin(); it != executing_instructions.end();) {
            it->remaining_cycles--;
            
            if (it->remaining_cycles <= 0) {
                // Instrucao terminou execucao - calcular resultado real
                float result = 0.0;
                ReservationStation* station = nullptr;
                
                // Encontrar a estação correspondente
                if (it->station_type.find("Add") != string::npos) {
                    station = &add_stations[it->station_idx];
                } else if (it->station_type.find("Mult") != string::npos) {
                    station = &mult_stations[it->station_idx];
                } else if (it->station_type.find("Load") != string::npos) {
                    station = &load_stations[it->station_idx];
                } else if (it->station_type.find("Store") != string::npos) {
                    station = &store_stations[it->station_idx];
                }
                
                if (station != nullptr) {
                    switch (station->op) {
                        case ADD:
                            result = stof(station->vj) + stof(station->vk);
                            break;
                        case SUB:
                            result = stof(station->vj) - stof(station->vk);
                            break;
                        case MUL:
                            result = stof(station->vj) * stof(station->vk);
                            break;
                        case DIV:
                            if (stof(station->vk) != 0) {
                                result = stof(station->vj) / stof(station->vk);
                            } else {
                                cerr << "Erro: Divisao por zero!" << endl;
                                result = 0;
                            }
                            break;
                        case LOAD:
                            result = memory[station->address];
                            break;
                        case STORE:
                            memory[station->address] = stof(station->vj);
                            break;
                    }
                    
                    // Atualizar registradores e estações dependentes imediatamente
                    if (station->op != STORE) {
                        registers[station->dest].value = result;
                        registers[station->dest].ready = true;
                        registers[station->dest].busy = false;
                        registers[station->dest].producer_tag = "";
                    }
                    
                    // Atualizar estações que dependem deste resultado
                    string station_name = it->station_type + to_string(it->station_idx + 1);
                    for (auto stations : all_stations) {
                        for (auto& rs : *stations) {
                            if (rs.busy) {
                                if (rs.qj == station_name) {
                                    rs.vj = to_string(result);
                                    rs.qj = "";
                                }
                                if (rs.qk == station_name) {
                                    rs.vk = to_string(result);
                                    rs.qk = "";
                                }
                            }
                        }
                    }
                    
                    // Liberar a estação
                    station->busy = false;
                }
                
                // Remover da lista de instruções em execução
                it = executing_instructions.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Escrever resultados (Common Data Bus)
    void writeResults() {
        for (auto& result : cdb_buffer) {
            string producer = result.first;
            float value = result.second;
            
            // Atualizar registradores que dependem deste resultado
            for (auto& reg : registers) {
                if (reg.second.producer_tag == producer) {
                    reg.second.value = value;
                    reg.second.ready = true;
                    reg.second.busy = false;
                    reg.second.producer_tag = "";
                }
            }
            
            // Atualizar estações de reserva que aguardam este resultado
            vector<vector<ReservationStation>*> all_stations = {
                &add_stations, &mult_stations, &load_stations, &store_stations
            };
            
            for (auto stations : all_stations) {
                for (auto& station : *stations) {
                    if (station.qj == producer) {
                        station.vj = to_string(value);
                        station.qj = "";
                    }
                    if (station.qk == producer) {
                        station.vk = to_string(value);
                        station.qk = "";
                    }
                }
            }
            
            // Liberar estação que produziu o resultado
            for (auto stations : all_stations) {
                for (auto& station : *stations) {
                    if (station.busy && station.cycles_left == 0) {
                        station.busy = false;
                        station.instr_id = -1;
                    }
                }
            }
        }
        
        cdb_buffer.clear();
    }
    
    // Imprimir estado atual
    void printState() {
        cout << "\n==================== CICLO " << current_cycle << " ====================\n";
        
        // Imprimir estações de reserva ADD/SUB
        cout << "\nEstacoes de Reserva ADD/SUB:\n";
        cout << setw(8) << "Estacao" << setw(8) << "Busy" << setw(8) << "Op" 
             << setw(12) << "Vj" << setw(12) << "Vk" << setw(12) << "Qj" 
             << setw(12) << "Qk" << setw(10) << "Dest" << setw(8) << "Cycles\n";
        cout << string(88, '-') << "\n";
        
        for (int i = 0; i < add_stations.size(); i++) {
            auto& station = add_stations[i];
            cout << setw(8) << ("Add" + to_string(i + 1))
                 << setw(8) << (station.busy ? "Sim" : "Nao")
                 << setw(8) << (station.busy ? (station.op == ADD ? "ADD" : "SUB") : "-")
                 << setw(12) << (station.vj.empty() ? "-" : station.vj)
                 << setw(12) << (station.vk.empty() ? "-" : station.vk)
                 << setw(12) << (station.qj.empty() ? "-" : station.qj)
                 << setw(12) << (station.qk.empty() ? "-" : station.qk)
                 << setw(10) << (station.busy ? to_string(station.dest_reg) : "-")
                 << setw(8) << (station.busy ? to_string(station.cycles_left) : "-") << "\n";
        }
        
        // Imprimir estações de reserva MUL/DIV
        cout << "\nEstacoes de Reserva MUL/DIV:\n";
        cout << setw(8) << "Estacao" << setw(8) << "Busy" << setw(8) << "Op" 
             << setw(12) << "Vj" << setw(12) << "Vk" << setw(12) << "Qj" 
             << setw(12) << "Qk" << setw(10) << "Dest" << setw(8) << "Cycles\n";
        cout << string(88, '-') << "\n";
        
        for (int i = 0; i < mult_stations.size(); i++) {
            auto& station = mult_stations[i];
            cout << setw(8) << ("Mult" + to_string(i + 1))
                 << setw(8) << (station.busy ? "Sim" : "Nao")
                 << setw(8) << (station.busy ? (station.op == MUL ? "MUL" : "DIV") : "-")
                 << setw(12) << (station.vj.empty() ? "-" : station.vj)
                 << setw(12) << (station.vk.empty() ? "-" : station.vk)
                 << setw(12) << (station.qj.empty() ? "-" : station.qj)
                 << setw(12) << (station.qk.empty() ? "-" : station.qk)
                 << setw(10) << (station.busy ? to_string(station.dest_reg) : "-")
                 << setw(8) << (station.busy ? to_string(station.cycles_left) : "-") << "\n";
        }
        
        // Imprimir estações de reserva LOAD/STORE
        cout << "\nEstacoes de Reserva LOAD/STORE:\n";
        cout << setw(8) << "Estacao" << setw(8) << "Busy" << setw(8) << "Op" 
             << setw(12) << "Vj" << setw(12) << "Qj" << setw(12) << "Address" 
             << setw(8) << "Cycles\n";
        cout << string(68, '-') << "\n";
        
        for (int i = 0; i < load_stations.size(); i++) {
            auto& station = load_stations[i];
            cout << setw(8) << ("Load" + to_string(i + 1))
                 << setw(8) << (station.busy ? "Sim" : "Nao")
                 << setw(8) << (station.busy ? "LOAD" : "-")
                 << setw(12) << (station.vj.empty() ? "-" : station.vj)
                 << setw(12) << (station.qj.empty() ? "-" : station.qj)
                 << setw(12) << (station.busy ? to_string(station.address) : "-")
                 << setw(8) << (station.busy ? to_string(station.cycles_left) : "-") << "\n";
        }
        
        for (int i = 0; i < store_stations.size(); i++) {
            auto& station = store_stations[i];
            cout << setw(8) << ("Store" + to_string(i + 1))
                 << setw(8) << (station.busy ? "Sim" : "Nao")
                 << setw(8) << (station.busy ? "STORE" : "-")
                 << setw(12) << (station.vj.empty() ? "-" : station.vj)
                 << setw(12) << (station.qj.empty() ? "-" : station.qj)
                 << setw(12) << (station.busy ? to_string(station.address) : "-")
                 << setw(8) << (station.busy ? to_string(station.cycles_left) : "-") << "\n";
        }
        
        // Imprimir estado dos registradores
        cout << "\nEstado dos Registradores:\n";
        cout << setw(8) << "Reg" << setw(12) << "Valor" << setw(15) << "Produtor" 
             << setw(8) << "Ready" << setw(8) << "Busy\n";
        cout << string(51, '-') << "\n";
        
        for (int i = 0; i < 32; i++) {
            string reg_name = "F" + to_string(i);
            auto& reg = registers[reg_name];
            cout << setw(8) << reg_name
                 << setw(12) << fixed << setprecision(2) << reg.value
                 << setw(15) << (reg.producer_tag.empty() ? "-" : reg.producer_tag)
                 << setw(8) << (reg.ready ? "Sim" : "Nao")
                 << setw(8) << (reg.busy ? "Sim" : "Nao") << "\n";
        }
        
        // Imprimir instrucoes em execucao
        cout << "\nInstrucoes em Execucao:\n";
        cout << setw(8) << "ID" << setw(12) << "Estacao" << setw(8) << "Ciclos\n";
        cout << string(28, '-') << "\n";
        
        for (const auto& exec : executing_instructions) {
            cout << setw(8) << exec.instruction_id
                 << setw(12) << exec.station_type
                 << setw(8) << exec.remaining_cycles << "\n";
        }
        
        // Imprimir conteudo da memoria (apenas posicoes nao-zero)
        cout << "\nConteudo da Memoria (posicoes nao-zero):\n";
        cout << setw(8) << "Endereco" << setw(12) << "Valor\n";
        cout << string(20, '-') << "\n";
        
        for (int i = 0; i < memory.size(); i++) {
            if (memory[i] != 0.0) {
                cout << setw(8) << i
                     << setw(12) << fixed << setprecision(2) << memory[i] << "\n";
            }
        }
        
        cout << "\nInstrucoes na fila: " << instruction_queue.size() << "\n";
    }
    
    // Simular execução
    void simulate() {
        cout << "========== SIMULACAO DO ALGORITMO DE TOMASULO ==========\n";
        
        while (!instruction_queue.empty() || !executing_instructions.empty()) {
            cout << "\nProcessando ciclo " << current_cycle << "...\n";
            
            // 1. Emitir nova instrucao (se possivel)
            if (issueInstruction()) {
                cout << "Instrucao emitida no ciclo " << current_cycle << "\n";
            }
            
            // 2. Executar instrucoes em progresso
            executeInstructions();
            
            // 3. Escrever resultados
            writeResults();
            
            // 4. Mostrar estado atual
            printState();
            
            current_cycle++;
            
            // Limite de segurança
            if (current_cycle > 50) {
                cout << "\nSimulacao interrompida (limite de ciclos atingido)\n";
                break;
            }
        }
        
        cout << "\n========== SIMULACAO CONCLUIDA ==========\n";
    }
    
private:
    bool hasActiveInstructions() {
        vector<vector<ReservationStation>*> all_stations = {
            &add_stations, &mult_stations, &load_stations, &store_stations
        };
        
        for (auto stations : all_stations) {
            for (auto& station : *stations) {
                if (station.busy) return true;
            }
        }
        return false;
    }
};

int main() {
    TomasuloSimulator simulator;
    
    cout << "========== SIMULADOR DO ALGORITMO DE TOMASULO ==========\n";
    cout << "Digite o caminho completo do arquivo de instrucoes: ";
    
    string filename;
    getline(cin, filename);
    
    if (!simulator.loadInstructions(filename)) {
        cout << "Erro: Nao foi possivel carregar o arquivo '" << filename << "'\n";
        cout << "\nFormato esperado do arquivo:\n";
        cout << "# Comentarios comecam com #\n";
        cout << "ADD F1 F2 F3\n";
        cout << "SUB F4 F1 F5\n";
        cout << "MUL F6 F2 F4\n";
        cout << "DIV F7 F6 F3\n";
        cout << "LOAD F8 0(R1)\n";
        cout << "STORE F2 4(R2)\n";
        cout << "\nOperacoes suportadas: ADD, SUB, MUL, DIV, LOAD, STORE\n";
        return 1;
    }
    
    cout << "Arquivo carregado com sucesso!\n";
    simulator.simulate();
    return 0;
}