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
    int commit_cycle;  // Adicionado campo commit_cycle
    
    Instruction(int _id, OpType _op, string _dest, string _src1, string _src2 = "", int _addr = 0)
        : id(_id), op(_op), dest(_dest), src1(_src1), src2(_src2), address(_addr), 
          state(ISSUED), issue_cycle(-1), exec_start_cycle(-1), exec_end_cycle(-1), 
          write_cycle(-1), commit_cycle(-1) {}
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

// Estrutura do ROB
struct ReorderBufferEntry {
    bool busy;
    int instruction_index;
    OpType type;
    string state;  // EMPTY, ISSUE, EXECUTE, WRITE_RESULT
    string destination_register;
    float value;
    int address;
    bool value_ready;
    
    ReorderBufferEntry() : busy(false), instruction_index(-1), type(ADD), 
                          state("EMPTY"), value(0), address(0), value_ready(false) {}
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
    
    // Memória simulada
    vector<float> memory;
    
    // Instrucoes em execucao
    vector<ExecutingInstruction> executing_instructions;
    
    // Reorder Buffer (ROB)
    vector<ReorderBufferEntry> rob;
    int rob_size;
    int rob_head;
    int rob_tail;
    int rob_entries_available;
    
    // Fila de instruções completadas aguardando CDB
    vector<tuple<int, float, string>> completed_for_cdb;

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
        latencies[MUL] = 10;
        latencies[DIV] = 40;
        latencies[LOAD] = 3;
        latencies[STORE] = 3;
        
        // Inicializar ROB
        rob_size = 16;
        rob.resize(rob_size);
        rob_head = 0;
        rob_tail = 0;
        rob_entries_available = rob_size;
        
        // Inicializar registradores com valores aleatórios
        srand(time(0));
        for (int i = 0; i < 32; i++) {
            float random_value = (rand() % 10) * 10.0;
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
        if (instruction_queue.empty() || rob_entries_available == 0) return false;
        
        Instruction& instr = instruction_queue.front();
        
        // Verificar hazards antes de emitir
        if (checkHazards(instr)) return false;
        
        int station_idx = findFreeStation(instr.op);
        if (station_idx == -1) return false; // Hazard estrutural
        
        // Alocar entrada no ROB
        int current_rob_idx = rob_tail;
        ReorderBufferEntry& rob_entry = rob[current_rob_idx];
        rob_entry.busy = true;
        rob_entry.instruction_index = instr.id;
        rob_entry.type = instr.op;
        rob_entry.state = "ISSUE";
        
        // Configurar destino e endereço no ROB
        if (instr.op != STORE) {
            rob_entry.destination_register = instr.dest;
        }
        
        if (instr.op == LOAD || instr.op == STORE) {
            // Extrair offset e registrador base do formato offset(Rbase)
            size_t open_paren = instr.src1.find('(');
            size_t close_paren = instr.src1.find(')');
            
            if (open_paren != string::npos && close_paren != string::npos) {
                int offset = stoi(instr.src1.substr(0, open_paren));
                string base_reg = instr.src1.substr(open_paren + 1, close_paren - open_paren - 1);
                rob_entry.address = offset + static_cast<int>(registers[base_reg].value);
            }
        }
        
        // Atualizar ponteiros do ROB
        rob_tail = (rob_tail + 1) % rob_size;
        rob_entries_available--;
        
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
        station->dest = to_string(current_rob_idx);  // Tag do ROB como destino
        
        // Configurar endereço para LOAD/STORE
        if (instr.op == LOAD || instr.op == STORE) {
            station->address = rob_entry.address;
        }
        
        // Verificar dependências e renomear registradores
        if (instr.op != STORE) {
            // Para instruções que escrevem em registrador
            registers[instr.dest].producer_tag = to_string(current_rob_idx);  // Tag do ROB
            registers[instr.dest].ready = false;
            registers[instr.dest].busy = true;
        }
        
        // Configurar operandos usando tags do ROB
        if (registers[instr.src1].ready) {
            station->vj = to_string(registers[instr.src1].value);
            station->qj = "";
        } else {
            station->vj = "";
            station->qj = registers[instr.src1].producer_tag;  // Tag do ROB
        }
        
        if (!instr.src2.empty() && instr.op != LOAD && instr.op != STORE) {
            if (registers[instr.src2].ready) {
                station->vk = to_string(registers[instr.src2].value);
                station->qk = "";
            } else {
                station->vk = "";
                station->qk = registers[instr.src2].producer_tag;  // Tag do ROB
            }
        }
        
        // Adicionar à lista de instruções em execução
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
                    
                    // Adicionar ao CDB
                    completed_for_cdb.push_back({station->instr_id, result, station->dest});
                    
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
    
    // Imprimir estado atual
    void printState() {
        cout << "\n==================== CICLO " << current_cycle << " ====================" << endl;
        
        // Imprimir estações de reserva ADD/SUB
        cout << "\nEstações de Reserva ADD/SUB:" << endl;
        cout << setw(8) << "Estação" << setw(8) << "Busy" << setw(8) << "Op" << setw(12) << "Vj" 
             << setw(12) << "Vk" << setw(12) << "Qj" << setw(12) << "Qk" << setw(10) << "Dest" 
             << setw(8) << "Ciclos" << endl;
        cout << string(88, '-') << endl;
        
        for (size_t i = 0; i < add_stations.size(); i++) {
            const auto& station = add_stations[i];
            cout << setw(7) << "Add" + to_string(i+1) << setw(8) << station.busy << setw(8) 
                 << station.op << setw(12) << station.vj << setw(12) << station.vk 
                 << setw(12) << station.qj << setw(12) << station.qk 
                 << setw(10) << station.dest << setw(8) << station.cycles_left << endl;
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
        
        // Imprimir estado do ROB
        cout << "\nReorder Buffer (ROB):" << endl;
        cout << setw(6) << "ROB#" << setw(6) << "Busy" << setw(8) << "InstIdx" << setw(8) << "Type" 
             << setw(12) << "State" << setw(8) << "DestReg" << setw(8) << "ValRdy" 
             << setw(8) << "Value" << setw(8) << "Addr" << endl;
        cout << string(70, '-') << endl;
        
        for (size_t i = 0; i < rob.size(); i++) {
            const auto& entry = rob[i];
            if (entry.busy) {
                cout << setw(6) << i << setw(6) << entry.busy << setw(8) << entry.instruction_index 
                     << setw(8) << entry.type << setw(12) << entry.state 
                     << setw(8) << entry.destination_register << setw(8) << entry.value_ready 
                     << setw(8) << entry.value << setw(8) << entry.address << endl;
            }
        }
        
        // Imprimir estado da fila CDB
        cout << "\nCommon Data Bus (CDB):" << endl;
        cout << setw(8) << "Posicao" << setw(8) << "InstID" << setw(12) << "Valor" << setw(8) << "ROB" << endl;
        cout << string(36, '-') << endl;
        
        for (size_t i = 0; i < completed_for_cdb.size(); i++) {
            const auto& cdb_entry = completed_for_cdb[i];
            cout << setw(8) << i << setw(8) << get<0>(cdb_entry) << setw(12) << fixed << setprecision(2) 
                 << get<1>(cdb_entry) << setw(8) << get<2>(cdb_entry) << endl;
        }
        
        cout << "\nInstrucoes na fila: " << instruction_queue.size() << endl;
        cout << "Instrucoes completadas aguardando CDB: " << completed_for_cdb.size() << endl;
        cout << "ROB Head: " << rob_head << ", Tail: " << rob_tail 
             << ", Available: " << rob_entries_available << endl;
    }
    
    // Simular execução
    void simulate() {
        cout << "========== SIMULACAO DO ALGORITMO DE TOMASULO ==========" << endl;
        
        while (hasActiveInstructions()) {
            cout << "\nProcessando ciclo " << current_cycle << "..." << endl;
            
            // 1. Commit
            commitInstruction();
            
            // 2. Write-Back (CDB)
            processWriteBack();
            
            // 3. Issue
            if (issueInstruction()) {
                cout << "Instrução emitida no ciclo " << current_cycle << endl;
            }
            
            // 4. Execute
            executeInstructions();
            
            // 5. Mostrar estado atual
            printState();
            
            current_cycle++;
            
            // Limite de segurança
            if (current_cycle > 50) {
                cout << "\nSimulação interrompida (limite de ciclos atingido)" << endl;
                break;
            }
        }
        
        cout << "\n========== SIMULACAO CONCLUIDA ==========" << endl;
    }
    
    void processWriteBack() {
        if (completed_for_cdb.empty()) return;
        
        const auto& cdb_entry = completed_for_cdb.front();
        int instr_id = get<0>(cdb_entry);
        float result = get<1>(cdb_entry);
        string rob_idx_str = get<2>(cdb_entry);
        
        completed_for_cdb.erase(completed_for_cdb.begin());
        
        int rob_idx = stoi(rob_idx_str);
        if (rob_idx >= 0 && rob_idx < rob_size) {
            ReorderBufferEntry& rob_entry = rob[rob_idx];
            if (rob_entry.busy) {
                rob_entry.value = result;
                rob_entry.value_ready = true;
                rob_entry.state = "WRITE_RESULT";
                
                // Atualizar registradores que dependem deste resultado
                for (auto& reg : registers) {
                    if (reg.second.producer_tag == rob_idx_str) {
                        reg.second.value = result;
                        reg.second.ready = true;
                        reg.second.busy = false;
                        reg.second.producer_tag = "";
                    }
                }
                
                // Atualizar estações de reserva que dependem deste resultado
                for (auto& station : getAllStations()) {
                    if (station->busy) {
                        if (station->qj == rob_idx_str) {
                            station->vj = to_string(result);
                            station->qj = "";
                        }
                        if (station->qk == rob_idx_str) {
                            station->vk = to_string(result);
                            station->qk = "";
                        }
                    }
                }
                
                // Liberar a estação de reserva
                for (auto& station : getAllStations()) {
                    if (station->dest == rob_idx_str) {
                        station->busy = false;
                        station->instr_id = -1;
                        station->qj = "";
                        station->qk = "";
                        station->vj = "0";
                        station->vk = "0";
                        station->address = 0;
                        station->dest = "";
                    }
                }
            }
        }
    }
    
    void commitInstruction() {
        if (rob_entries_available == rob_size || !rob[rob_head].busy) return;
        
        ReorderBufferEntry& head_entry = rob[rob_head];
        
        if (head_entry.state == "WRITE_RESULT" && head_entry.value_ready) {
            Instruction& instr = all_instructions[head_entry.instruction_index];
            instr.commit_cycle = current_cycle;
            
            if (head_entry.type != STORE) {
                registers[head_entry.destination_register].value = head_entry.value;
                registers[head_entry.destination_register].ready = true;
                registers[head_entry.destination_register].busy = false;
                registers[head_entry.destination_register].producer_tag = "";
            } else {
                if (head_entry.address >= 0 && head_entry.address < memory.size()) {
                    memory[head_entry.address] = head_entry.value;
                }
            }
            
            cout << "Ciclo " << current_cycle << ": Commit Inst " 
                 << head_entry.instruction_index << " (ROB " << rob_head << ")" << endl;
            
            head_entry.busy = false;
            head_entry.state = "EMPTY";
            rob_head = (rob_head + 1) % rob_size;
            rob_entries_available++;
        }
    }
    
    vector<ReservationStation*> getAllStations() {
        vector<ReservationStation*> all;
        for (auto& station : add_stations) all.push_back(&station);
        for (auto& station : mult_stations) all.push_back(&station);
        for (auto& station : load_stations) all.push_back(&station);
        for (auto& station : store_stations) all.push_back(&station);
        return all;
    }
    
private:
    bool hasActiveInstructions() {
        return !instruction_queue.empty() || 
               !executing_instructions.empty() || 
               !completed_for_cdb.empty() || 
               rob_entries_available != rob_size;
    }
};

int main() {
    TomasuloSimulator simulator;
    
    cout << "========== SIMULADOR DO ALGORITMO DE TOMASULO ==========" << endl;
    cout << "Digite o nome do arquivo de instrucoes: ";
    
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