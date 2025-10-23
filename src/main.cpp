#include "raylib.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace std;

struct Component {
    string id, tipo, nome, specs;
    double preco;
};

struct Account {
    string username, password;
    bool isAdmin;
};

vector<Component> catalog, buildAtual;
vector<Account> accounts = {{"admin", "Programador", true}, {"admin2", "adminProgramador", true}, {"andre", "Test", false}, {"danny", "Test2", false}};
string currentUser = "";
bool isCurrentUserAdmin = false;

enum Screen { LOGIN, REGISTER, MENU, ESCOLHER_PECAS, VER_PRECOS, COMO_MONTAR, ADMIN_PANEL, ADMIN_ADD, ADMIN_EDIT, ADMIN_DELETE };
Screen currentScreen = LOGIN;

// Login/Register fields
string loginUser = "", loginPass = "";
bool loginFocusUser = true;
string registerUser = "", registerPass = "", registerPassConfirm = "", registerMessage = "";
int registerFocusField = 0;

// Admin fields
string adminId = "", adminTipo = "", adminNome = "", adminSpecs = "", adminPreco = "", adminMessage = "";
int adminFocusField = 0, adminSelectedIndex = -1, adminScrollOffset = 0;

// UI fields
string filtroCategoria = "Todos", feedbackMessage = "";
int pecasScrollOffset = 0, feedbackTimer = 0;

static inline string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

void saveComponents() {
    ofstream f("componentes.realyb");
    if (!f.is_open()) return;
    for (auto &c : catalog) 
        f << c.id << ";" << c.tipo << ";" << c.nome << ";" << c.specs << ";" << c.preco << "\n";
    f.close();
}

bool loadComponents(const string &path) {
    ifstream f(path);
    if (!f.is_open()) return false;
    catalog.clear();
    string line;
    while (getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        vector<string> parts;
        string cur;
        for (char c : line) {
            if (c == ';') { parts.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        parts.push_back(cur);
        
        if (parts.size() < 5) continue;
        
        Component c;
        c.id = trim(parts[0]);
        c.tipo = trim(parts[1]);
        c.nome = trim(parts[2]);
        c.specs = trim(parts[3]);
        try { c.preco = stod(trim(parts[4])); } 
        catch(...) { c.preco = 0.0; }
        
        catalog.push_back(c);
    }
    f.close();
    return true;
}

string formatBR(double v) {
    stringstream ss;
    ss << fixed << setprecision(2) << v;
    string s = ss.str();
    size_t dotPos = s.find('.');
    if (dotPos != string::npos) s[dotPos] = ',';
    return "R$ " + s;
}

double calcularTotal() {
    double total = 0.0;
    for (auto &c : buildAtual) total += c.preco;
    return total;
}

void drawTextBox(const char* label, string &text, Rectangle box, bool focused, bool maskPassword = false) {
    DrawText(label, (int)box.x, (int)box.y - 30, 20, BLACK);
    DrawRectangleRec(box, focused ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(box, 2, focused ? BLUE : GRAY);
    string display = maskPassword ? string(text.length(), '*') : text;
    DrawText(display.c_str(), (int)box.x + 10, (int)box.y + 10, 20, BLACK);
}

void drawButton(Rectangle btn, const char* text, Color color) {
    DrawRectangleRec(btn, color);
    int textWidth = MeasureText(text, 20);
    DrawText(text, (int)btn.x + (int)(btn.width - textWidth) / 2, (int)btn.y + 15, 20, WHITE);
}

void drawLoginScreen() {
    DrawText("=== BuildComputer ===", 250, 100, 30, DARKBLUE);
    DrawText("Login", 350, 180, 20, DARKGRAY);
    
    drawTextBox("Usuario:", loginUser, {200, 280, 400, 40}, loginFocusUser);
    drawTextBox("Senha:", loginPass, {200, 380, 400, 40}, !loginFocusUser, true);
    
    drawButton({250, 460, 140, 50}, "ENTRAR", DARKBLUE);
    drawButton({410, 460, 140, 50}, "CRIAR CONTA", GREEN);
}

void drawRegisterScreen() {
    DrawText("=== Criar Nova Conta ===", 230, 80, 30, DARKBLUE);
    
    drawTextBox("Usuario:", registerUser, {200, 210, 400, 40}, registerFocusField == 0);
    drawTextBox("Senha:", registerPass, {200, 300, 400, 40}, registerFocusField == 1, true);
    drawTextBox("Confirmar Senha:", registerPassConfirm, {200, 390, 400, 40}, registerFocusField == 2, true);
    
    if (!registerMessage.empty()) {
        Color msgColor = (registerMessage.find("sucesso") != string::npos) ? GREEN : RED;
        DrawText(registerMessage.c_str(), 200, 450, 18, msgColor);
    }
    
    drawButton({250, 500, 140, 50}, "CRIAR", GREEN);
    drawButton({410, 500, 140, 50}, "VOLTAR", GRAY);
}

void drawMenuScreen() {
    string welcome = "Bem-vindo, " + currentUser + (isCurrentUserAdmin ? " (Admin)" : "");
    DrawText(welcome.c_str(), 50, 30, 20, DARKGRAY);
    DrawText("=== MENU PRINCIPAL ===", 250, 80, 30, DARKBLUE);
    
    const char* labels[] = {"Como Montar", "Escolher Pecas", "Ver Precos", "Painel Admin", "Sair"};
    Color colors[] = {BLUE, GREEN, ORANGE, isCurrentUserAdmin ? PURPLE : GRAY, RED};
    
    for (int i = 0; i < 5; i++) {
        Rectangle btn = {250, 160.0f + i * 80, 300, 60};
        DrawRectangleRec(btn, colors[i]);
        int textWidth = MeasureText(labels[i], 20);
        DrawText(labels[i], 250 + (300 - textWidth) / 2, 180 + i * 80, 20, 
                 (i == 3 && !isCurrentUserAdmin) ? LIGHTGRAY : WHITE);
    }
}

void drawAdminPanelScreen() {
    DrawText("=== PAINEL ADMINISTRATIVO ===", 180, 20, 28, PURPLE);
    DrawText(TextFormat("Total: %d componentes", (int)catalog.size()), 50, 70, 18, DARKGRAY);
    
    const char* labels[] = {"ADICIONAR", "EDITAR", "EXCLUIR", "VOLTAR"};
    Color colors[] = {GREEN, BLUE, RED, GRAY};
    
    for (int i = 0; i < 4; i++) {
        Rectangle btn = {50.0f + i * 170, 110, 150, 45};
        drawButton(btn, labels[i], colors[i]);
    }
    
    DrawLine(50, 170, 750, 170, DARKGRAY);
    
    int y = 190, maxVisible = 8;
    for (size_t i = adminScrollOffset; i < catalog.size() && i < (size_t)(adminScrollOffset + maxVisible); i++) {
        auto &c = catalog[i];
        Color bgColor = ((int)i == adminSelectedIndex) ? LIGHTGRAY : WHITE;
        DrawRectangle(50, y - 5, 700, 35, bgColor);
        string line = TextFormat("[%d] %s | %s | %s", (int)i, c.tipo.c_str(), 
                                 c.nome.substr(0, 25).c_str(), formatBR(c.preco).c_str());
        DrawText(line.c_str(), 55, y, 14, BLACK);
        y += 37;
    }
    
    if (catalog.size() > (size_t)maxVisible)
        DrawText("Use SCROLL para navegar | Clique para selecionar", 220, 550, 14, DARKGRAY);
}

void drawAdminEditFields(const char* title) {
    DrawText(title, 200, 30, 28, currentScreen == ADMIN_ADD ? GREEN : BLUE);
    
    const char* labels[] = {"ID:", "Tipo (CPU/GPU/RAM/Motherboard/Storage/PSU/Case/Cooler):", 
                           "Nome:", "Especificacoes:", "Preco (use ponto para decimal):"};
    string* fields[] = {&adminId, &adminTipo, &adminNome, &adminSpecs, &adminPreco};
    int yPos[] = {135, 210, 285, 360, 435};
    
    for (int i = 0; i < 5; i++) {
        DrawText(labels[i], 150, yPos[i] - 25, i == 1 ? 16 : 18, BLACK);
        Rectangle box = {150, (float)yPos[i], 500, 35};
        DrawRectangleRec(box, adminFocusField == i ? LIGHTGRAY : WHITE);
        DrawRectangleLinesEx(box, 2, adminFocusField == i ? BLUE : GRAY);
        DrawText(fields[i]->c_str(), 160, yPos[i] + 8, 18, BLACK);
    }
    
    if (!adminMessage.empty()) {
        Color msgColor = (adminMessage.find("sucesso") != string::npos) ? GREEN : RED;
        DrawText(adminMessage.c_str(), 150, 490, 16, msgColor);
    }
    
    drawButton({250, 520, 130, 45}, "SALVAR", currentScreen == ADMIN_ADD ? GREEN : BLUE);
    drawButton({420, 520, 130, 45}, "CANCELAR", GRAY);
}

void drawAdminAddScreen() { drawAdminEditFields("=== ADICIONAR COMPONENTE ==="); }

void drawAdminEditScreen() {
    if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
        drawAdminEditFields("=== EDITAR COMPONENTE ===");
    } else {
        DrawText("=== EDITAR COMPONENTE ===", 220, 30, 28, BLUE);
        DrawText("Nenhum componente selecionado!", 250, 300, 20, RED);
        drawButton({300, 400, 200, 50}, "VOLTAR", GRAY);
    }
}

void drawAdminDeleteScreen() {
    DrawText("=== EXCLUIR COMPONENTE ===", 210, 30, 28, RED);
    
    if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
        auto &c = catalog[adminSelectedIndex];
        DrawText("Confirmar exclusao do seguinte componente:", 150, 120, 18, BLACK);
        
        const char* labels[] = {"ID:", "Tipo:", "Nome:", "Specs:", "Preco:"};
        string values[] = {c.id, c.tipo, c.nome, c.specs, formatBR(c.preco)};
        
        for (int i = 0; i < 5; i++)
            DrawText(TextFormat("%s %s", labels[i], values[i].c_str()), 150, 170 + i * 30, 16, DARKGRAY);
        
        DrawText("Esta acao nao pode ser desfeita!", 220, 350, 18, RED);
        
        if (!adminMessage.empty()) {
            Color msgColor = (adminMessage.find("sucesso") != string::npos) ? GREEN : RED;
            DrawText(adminMessage.c_str(), 150, 400, 16, msgColor);
        }
        
        drawButton({250, 450, 130, 45}, "EXCLUIR", RED);
        drawButton({420, 450, 130, 45}, "CANCELAR", GRAY);
    } else {
        DrawText("Nenhum componente selecionado!", 250, 300, 20, RED);
        drawButton({300, 400, 200, 50}, "VOLTAR", GRAY);
    }
}

void drawEscolherPecasScreen() {
    DrawText("=== ESCOLHER PECAS ===", 250, 20, 28, GREEN);
    
    DrawText("Filtro:", 50, 70, 18, BLACK);
    vector<string> categorias = {"Todos", "CPU", "GPU", "RAM", "Motherboard", "Storage", "PSU", "Case", "Cooler"};
    int btnX = 130;
    for (auto &cat : categorias) {
        int btnWidth = MeasureText(cat.c_str(), 14) + 20;
        Rectangle btn = {(float)btnX, 65, (float)btnWidth, 30};
        Color btnColor = (filtroCategoria == cat) ? BLUE : LIGHTGRAY;
        DrawRectangleRec(btn, btnColor);
        DrawRectangleLinesEx(btn, 1, DARKGRAY);
        DrawText(cat.c_str(), btnX + 10, 72, 14, (filtroCategoria == cat) ? WHITE : BLACK);
        btnX += btnWidth + 5;
    }
    
    DrawLine(50, 110, 750, 110, DARKGRAY);
    
    vector<Component> filtered;
    for (auto &c : catalog)
        if (filtroCategoria == "Todos" || c.tipo == filtroCategoria)
            filtered.push_back(c);
    
    int y = 130, maxVisible = 9;
    for (size_t i = pecasScrollOffset; i < filtered.size() && i < (size_t)(pecasScrollOffset + maxVisible); i++) {
        auto &c = filtered[i];
        Rectangle itemRect = {50, (float)y - 5, 650, 40};
        DrawRectangleRec(itemRect, WHITE);
        DrawRectangleLinesEx(itemRect, 1, LIGHTGRAY);
        
        DrawText(c.nome.c_str(), 60, y, 16, BLACK);
        DrawText(c.specs.c_str(), 60, y + 18, 12, DARKGRAY);
        DrawText(formatBR(c.preco).c_str(), 550, y + 8, 16, DARKGREEN);
        
        DrawRectangleRec({710, (float)y, 40, 35}, GREEN);
        DrawText("+", 723, y + 8, 20, WHITE);
        y += 45;
    }
    
    if (filtered.size() > (size_t)maxVisible)
        DrawText("Use SCROLL para navegar", 300, 555, 14, DARKGRAY);
    
    DrawText(TextFormat("Build Atual (%d pecas)", (int)buildAtual.size()), 50, 555, 16, DARKBLUE);
    DrawText(TextFormat("Total: %s", formatBR(calcularTotal()).c_str()), 600, 555, 18, DARKGREEN);
    drawButton({650, 20, 100, 35}, "VOLTAR", GRAY);
}

void drawVerPrecosScreen() {
    DrawText("=== VER PRECOS DA BUILD ===", 200, 20, 28, ORANGE);
    
    if (buildAtual.empty()) {
        DrawText("Sua build esta vazia!", 280, 300, 20, DARKGRAY);
        DrawText("Va em 'Escolher Pecas' para adicionar componentes.", 180, 340, 16, GRAY);
    } else {
        DrawLine(50, 70, 750, 70, DARKGRAY);
        
        int y = 90;
        for (size_t i = 0; i < buildAtual.size(); i++) {
            auto &c = buildAtual[i];
            DrawText(c.nome.c_str(), 60, y, 16, BLACK);
            DrawText(c.specs.c_str(), 60, y + 18, 12, DARKGRAY);
            DrawText(formatBR(c.preco).c_str(), 550, y + 8, 16, DARKGREEN);
            
            DrawRectangleRec({710, (float)y, 40, 35}, RED);
            DrawText("X", 723, y + 8, 20, WHITE);
            y += 45;
        }
        
        DrawLine(50, y + 10, 750, y + 10, DARKGRAY);
        DrawText("TOTAL:", 500, y + 30, 22, BLACK);
        DrawText(formatBR(calcularTotal()).c_str(), 600, y + 30, 22, DARKGREEN);
        
        drawButton({50, (float)y + 70, 150, 40}, "LIMPAR BUILD", RED);
    }
    
    drawButton({650, 20, 100, 35}, "VOLTAR", GRAY);
}

void drawComoMontarScreen() {
    DrawText("=== COMO MONTAR UM PC ===", 220, 20, 28, BLUE);
    
    const char* steps[] = {
        "Prepare a area de trabalho", "Instale a CPU na Motherboard", "Instale a RAM",
        "Monte a Motherboard no Case", "Instale a PSU", "Conecte os cabos",
        "Instale Storage e GPU", "Teste antes de fechar"
    };
    
    const char* details[][3] = {
        {"Mesa limpa e espaco adequado", "Boa iluminacao", ""},
        {"Alinhe os pinos/contatos corretamente", "Trave o socket da CPU", ""},
        {"Verifique os slots corretos (manual da placa-mae)", "Pressione ate ouvir o clique", ""},
        {"Instale o I/O shield primeiro", "Use os standoffs corretos", ""},
        {"Ventilador voltado para fora/baixo", "", ""},
        {"CPU Power (4/8 pinos)", "Motherboard Power (24 pinos)", "GPU Power (se necessario)"},
        {"", "", ""}, {"", "", ""}
    };
    
    int y = 80;
    for (int i = 0; i < 8; i++) {
        DrawText(TextFormat("%d. %s", i + 1, steps[i]), 60, y, 16, BLACK);
        y += 30;
        for (int j = 0; j < 3 && details[i][j][0] != '\0'; j++) {
            DrawText(TextFormat("   - %s", details[i][j]), 70, y, 14, DARKGRAY);
            y += 20;
        }
        y += 10;
    }
    
    drawButton({300, 540, 200, 40}, "VOLTAR", GRAY);
}

void handleTextInput(string &field, size_t maxLen = 50) {
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125 && field.length() < maxLen)
            field += (char)key;
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !field.empty())
        field.pop_back();
}

int main() {
    InitWindow(800, 600, "BuildComputer System");
    SetTargetFPS(60);
    
    if (!loadComponents("componentes.realyb"))
        cout << "Aviso: Arquivo componentes.realyb nao encontrado!" << endl;
    
    while (!WindowShouldClose()) {
        if (feedbackTimer > 0) {
            feedbackTimer--;
            if (feedbackTimer == 0) feedbackMessage = "";
        }
        
        Vector2 mousePos = GetMousePosition();
        bool mouseClick = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        float mouseWheel = GetMouseWheelMove();
        
        // LOGIN SCREEN
        if (currentScreen == LOGIN) {
            if (loginFocusUser) handleTextInput(loginUser, 30);
            else handleTextInput(loginPass, 30);
            
            if (IsKeyPressed(KEY_TAB)) loginFocusUser = !loginFocusUser;
            
            if (mouseClick) {
                if (CheckCollisionPointRec(mousePos, {200, 280, 400, 40})) loginFocusUser = true;
                else if (CheckCollisionPointRec(mousePos, {200, 380, 400, 40})) loginFocusUser = false;
                else if (CheckCollisionPointRec(mousePos, {250, 460, 140, 50})) {
                    bool found = false;
                    for (auto &acc : accounts) {
                        if (acc.username == loginUser && acc.password == loginPass) {
                            currentUser = acc.username;
                            isCurrentUserAdmin = acc.isAdmin;
                            currentScreen = MENU;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        feedbackMessage = "Usuario ou senha incorretos!";
                        feedbackTimer = 180;
                    }
                    loginUser = loginPass = "";
                } else if (CheckCollisionPointRec(mousePos, {410, 460, 140, 50})) {
                    currentScreen = REGISTER;
                    registerUser = registerPass = registerPassConfirm = registerMessage = "";
                    registerFocusField = 0;
                }
            }
            
            if (IsKeyPressed(KEY_ENTER)) {
                bool found = false;
                for (auto &acc : accounts) {
                    if (acc.username == loginUser && acc.password == loginPass) {
                        currentUser = acc.username;
                        isCurrentUserAdmin = acc.isAdmin;
                        currentScreen = MENU;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    feedbackMessage = "Usuario ou senha incorretos!";
                    feedbackTimer = 180;
                }
                loginUser = loginPass = "";
            }
        }
        
        // REGISTER SCREEN
        else if (currentScreen == REGISTER) {
            if (registerFocusField == 0) handleTextInput(registerUser, 30);
            else if (registerFocusField == 1) handleTextInput(registerPass, 30);
            else if (registerFocusField == 2) handleTextInput(registerPassConfirm, 30);
            
            if (IsKeyPressed(KEY_TAB)) registerFocusField = (registerFocusField + 1) % 3;
            
            if (mouseClick) {
                if (CheckCollisionPointRec(mousePos, {200, 210, 400, 40})) registerFocusField = 0;
                else if (CheckCollisionPointRec(mousePos, {200, 300, 400, 40})) registerFocusField = 1;
                else if (CheckCollisionPointRec(mousePos, {200, 390, 400, 40})) registerFocusField = 2;
                else if (CheckCollisionPointRec(mousePos, {250, 500, 140, 50})) {
                    if (registerUser.empty() || registerPass.empty()) {
                        registerMessage = "Preencha todos os campos!";
                    } else if (registerPass != registerPassConfirm) {
                        registerMessage = "Senhas nao coincidem!";
                    } else {
                        bool exists = false;
                        for (auto &acc : accounts) {
                            if (acc.username == registerUser) {
                                exists = true;
                                break;
                            }
                        }
                        if (exists) {
                            registerMessage = "Usuario ja existe!";
                        } else {
                            accounts.push_back({registerUser, registerPass, false});
                            registerMessage = "Conta criada com sucesso!";
                            feedbackTimer = 120;
                        }
                    }
                } else if (CheckCollisionPointRec(mousePos, {410, 500, 140, 50})) {
                    currentScreen = LOGIN;
                }
            }
        }
        
        // MENU SCREEN
        else if (currentScreen == MENU) {
            if (mouseClick) {
                if (CheckCollisionPointRec(mousePos, {250, 160, 300, 60})) currentScreen = COMO_MONTAR;
                else if (CheckCollisionPointRec(mousePos, {250, 240, 300, 60})) currentScreen = ESCOLHER_PECAS;
                else if (CheckCollisionPointRec(mousePos, {250, 320, 300, 60})) currentScreen = VER_PRECOS;
                else if (CheckCollisionPointRec(mousePos, {250, 400, 300, 60}) && isCurrentUserAdmin) {
                    currentScreen = ADMIN_PANEL;
                    adminSelectedIndex = -1;
                    adminScrollOffset = 0;
                }
                else if (CheckCollisionPointRec(mousePos, {250, 480, 300, 60})) {
                    currentScreen = LOGIN;
                    currentUser = "";
                    isCurrentUserAdmin = false;
                    buildAtual.clear();
                }
            }
        }
        
        // ESCOLHER PECAS SCREEN
        else if (currentScreen == ESCOLHER_PECAS) {
            if (mouseClick) {
                if (CheckCollisionPointRec(mousePos, {650, 20, 100, 35})) {
                    currentScreen = MENU;
                    pecasScrollOffset = 0;
                }
                
                vector<string> categorias = {"Todos", "CPU", "GPU", "RAM", "Motherboard", "Storage", "PSU", "Case", "Cooler"};
                int btnX = 130;
                for (auto &cat : categorias) {
                    int btnWidth = MeasureText(cat.c_str(), 14) + 20;
                    if (CheckCollisionPointRec(mousePos, {(float)btnX, 65, (float)btnWidth, 30})) {
                        filtroCategoria = cat;
                        pecasScrollOffset = 0;
                    }
                    btnX += btnWidth + 5;
                }
                
                vector<Component> filtered;
                for (auto &c : catalog)
                    if (filtroCategoria == "Todos" || c.tipo == filtroCategoria)
                        filtered.push_back(c);
                
                int y = 130;
                for (size_t i = pecasScrollOffset; i < filtered.size() && i < (size_t)(pecasScrollOffset + 9); i++) {
                    if (CheckCollisionPointRec(mousePos, {710, (float)y, 40, 35})) {
                        buildAtual.push_back(filtered[i]);
                        feedbackMessage = "Componente adicionado!";
                        feedbackTimer = 60;
                    }
                    y += 45;
                }
            }
            
            if (mouseWheel != 0) {
                vector<Component> filtered;
                for (auto &c : catalog)
                    if (filtroCategoria == "Todos" || c.tipo == filtroCategoria)
                        filtered.push_back(c);
                
                pecasScrollOffset -= (int)mouseWheel;
                if (pecasScrollOffset < 0) pecasScrollOffset = 0;
                if (pecasScrollOffset > (int)filtered.size() - 9) 
                    pecasScrollOffset = max(0, (int)filtered.size() - 9);
            }
        }
        
        // VER PRECOS SCREEN
        else if (currentScreen == VER_PRECOS) {
            if (mouseClick) {
                if (CheckCollisionPointRec(mousePos, {650, 20, 100, 35})) currentScreen = MENU;
                
                if (!buildAtual.empty()) {
                    int y = 90;
                    for (size_t i = 0; i < buildAtual.size(); i++) {
                        if (CheckCollisionPointRec(mousePos, {710, (float)y, 40, 35})) {
                            buildAtual.erase(buildAtual.begin() + i);
                            feedbackMessage = "Componente removido!";
                            feedbackTimer = 60;
                            break;
                        }
                        y += 45;
                    }
                    
                    int totalY = 90 + (int)buildAtual.size() * 45 + 70;
                    if (!buildAtual.empty() && CheckCollisionPointRec(mousePos, {50, (float)totalY, 150, 40})) {
                        buildAtual.clear();
                        feedbackMessage = "Build limpa!";
                        feedbackTimer = 60;
                    }
                }
            }
        }
        
        // COMO MONTAR SCREEN
        else if (currentScreen == COMO_MONTAR) {
            if (mouseClick && CheckCollisionPointRec(mousePos, {300, 540, 200, 40})) {
                currentScreen = MENU;
            }
        }
        
        // ADMIN PANEL SCREEN
        else if (currentScreen == ADMIN_PANEL) {
            if (mouseClick) {
                if (CheckCollisionPointRec(mousePos, {50, 110, 150, 45})) {
                    currentScreen = ADMIN_ADD;
                    adminId = adminTipo = adminNome = adminSpecs = adminPreco = adminMessage = "";
                    adminFocusField = 0;
                }
                else if (CheckCollisionPointRec(mousePos, {220, 110, 150,45})) {
                    if (adminSelectedIndex >= 0) {
                        currentScreen = ADMIN_EDIT;
                        auto &c = catalog[adminSelectedIndex];
                        adminId = c.id;
                        adminTipo = c.tipo;
                        adminNome = c.nome;
                        adminSpecs = c.specs;
                        adminPreco = to_string(c.preco);
                        adminMessage = "";
                        adminFocusField = 0;
                    } else {
                        adminMessage = "Selecione um componente primeiro!";
                        feedbackTimer = 120;
                    }
                }
                else if (CheckCollisionPointRec(mousePos, {390, 110, 150, 45})) {
                    if (adminSelectedIndex >= 0) {
                        currentScreen = ADMIN_DELETE;
                        adminMessage = "";
                    } else {
                        adminMessage = "Selecione um componente primeiro!";
                        feedbackTimer = 120;
                    }
                }
                else if (CheckCollisionPointRec(mousePos, {560, 110, 150, 45})) {
                    currentScreen = MENU;
                    adminSelectedIndex = -1;
                    adminScrollOffset = 0;
                }
                
                // Click to select component
                int y = 190;
                for (size_t i = adminScrollOffset; i < catalog.size() && i < (size_t)(adminScrollOffset + 8); i++) {
                    if (CheckCollisionPointRec(mousePos, {50, (float)y - 5, 700, 35})) {
                        adminSelectedIndex = (int)i;
                        break;
                    }
                    y += 37;
                }
            }
            
            if (mouseWheel != 0) {
                adminScrollOffset -= (int)mouseWheel;
                if (adminScrollOffset < 0) adminScrollOffset = 0;
                if (adminScrollOffset > (int)catalog.size() - 8)
                    adminScrollOffset = max(0, (int)catalog.size() - 8);
            }
        }
        
        // ADMIN ADD SCREEN
        else if (currentScreen == ADMIN_ADD) {
            if (adminFocusField == 0) handleTextInput(adminId, 20);
            else if (adminFocusField == 1) handleTextInput(adminTipo, 30);
            else if (adminFocusField == 2) handleTextInput(adminNome, 100);
            else if (adminFocusField == 3) handleTextInput(adminSpecs, 150);
            else if (adminFocusField == 4) handleTextInput(adminPreco, 20);
            
            if (IsKeyPressed(KEY_TAB)) adminFocusField = (adminFocusField + 1) % 5;
            
            if (mouseClick) {
                int yPos[] = {135, 210, 285, 360, 435};
                for (int i = 0; i < 5; i++) {
                    if (CheckCollisionPointRec(mousePos, {150, (float)yPos[i], 500, 35})) {
                        adminFocusField = i;
                        break;
                    }
                }
                
                if (CheckCollisionPointRec(mousePos, {250, 520, 130, 45})) {
                    if (adminId.empty() || adminTipo.empty() || adminNome.empty() || adminPreco.empty()) {
                        adminMessage = "Preencha todos os campos obrigatorios!";
                    } else {
                        bool exists = false;
                        for (auto &c : catalog) {
                            if (c.id == adminId) {
                                exists = true;
                                break;
                            }
                        }
                        if (exists) {
                            adminMessage = "ID ja existe!";
                        } else {
                            Component newComp;
                            newComp.id = adminId;
                            newComp.tipo = adminTipo;
                            newComp.nome = adminNome;
                            newComp.specs = adminSpecs;
                            try {
                                newComp.preco = stod(adminPreco);
                                catalog.push_back(newComp);
                                saveComponents();
                                adminMessage = "Componente adicionado com sucesso!";
                                feedbackTimer = 120;
                            } catch(...) {
                                adminMessage = "Preco invalido!";
                            }
                        }
                    }
                }
                else if (CheckCollisionPointRec(mousePos, {420, 520, 130, 45})) {
                    currentScreen = ADMIN_PANEL;
                    adminMessage = "";
                }
            }
        }
        
        // ADMIN EDIT SCREEN
        else if (currentScreen == ADMIN_EDIT) {
            if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
                if (adminFocusField == 0) handleTextInput(adminId, 20);
                else if (adminFocusField == 1) handleTextInput(adminTipo, 30);
                else if (adminFocusField == 2) handleTextInput(adminNome, 100);
                else if (adminFocusField == 3) handleTextInput(adminSpecs, 150);
                else if (adminFocusField == 4) handleTextInput(adminPreco, 20);
                
                if (IsKeyPressed(KEY_TAB)) adminFocusField = (adminFocusField + 1) % 5;
                
                if (mouseClick) {
                    int yPos[] = {135, 210, 285, 360, 435};
                    for (int i = 0; i < 5; i++) {
                        if (CheckCollisionPointRec(mousePos, {150, (float)yPos[i], 500, 35})) {
                            adminFocusField = i;
                            break;
                        }
                    }
                    
                    if (CheckCollisionPointRec(mousePos, {250, 520, 130, 45})) {
                        if (adminId.empty() || adminTipo.empty() || adminNome.empty() || adminPreco.empty()) {
                            adminMessage = "Preencha todos os campos obrigatorios!";
                        } else {
                            catalog[adminSelectedIndex].id = adminId;
                            catalog[adminSelectedIndex].tipo = adminTipo;
                            catalog[adminSelectedIndex].nome = adminNome;
                            catalog[adminSelectedIndex].specs = adminSpecs;
                            try {
                                catalog[adminSelectedIndex].preco = stod(adminPreco);
                                saveComponents();
                                adminMessage = "Componente editado com sucesso!";
                                feedbackTimer = 120;
                            } catch(...) {
                                adminMessage = "Preco invalido!";
                            }
                        }
                    }
                    else if (CheckCollisionPointRec(mousePos, {420, 520, 130, 45})) {
                        currentScreen = ADMIN_PANEL;
                        adminMessage = "";
                    }
                }
            } else {
                if (mouseClick && CheckCollisionPointRec(mousePos, {300, 400, 200, 50})) {
                    currentScreen = ADMIN_PANEL;
                }
            }
        }
        
        // ADMIN DELETE SCREEN
        else if (currentScreen == ADMIN_DELETE) {
            if (mouseClick) {
                if (CheckCollisionPointRec(mousePos, {250, 450, 130, 45})) {
                    if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
                        catalog.erase(catalog.begin() + adminSelectedIndex);
                        saveComponents();
                        adminMessage = "Componente excluido com sucesso!";
                        adminSelectedIndex = -1;
                        feedbackTimer = 120;
                        currentScreen = ADMIN_PANEL;
                    }
                }
                else if (CheckCollisionPointRec(mousePos, {420, 450, 130, 45})) {
                    currentScreen = ADMIN_PANEL;
                    adminMessage = "";
                }
                else if (adminSelectedIndex < 0 && CheckCollisionPointRec(mousePos, {300, 400, 200, 50})) {
                    currentScreen = ADMIN_PANEL;
                }
            }
        }
        
        // DRAW
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        switch (currentScreen) {
            case LOGIN: drawLoginScreen(); break;
            case REGISTER: drawRegisterScreen(); break;
            case MENU: drawMenuScreen(); break;
            case ESCOLHER_PECAS: drawEscolherPecasScreen(); break;
            case VER_PRECOS: drawVerPrecosScreen(); break;
            case COMO_MONTAR: drawComoMontarScreen(); break;
            case ADMIN_PANEL: drawAdminPanelScreen(); break;
            case ADMIN_ADD: drawAdminAddScreen(); break;
            case ADMIN_EDIT: drawAdminEditScreen(); break;
            case ADMIN_DELETE: drawAdminDeleteScreen(); break;
        }
        
        if (!feedbackMessage.empty() && feedbackTimer > 0) {
            DrawRectangle(200, 10, 400, 40, Fade(RED, 0.8f));
            int textWidth = MeasureText(feedbackMessage.c_str(), 16);
            DrawText(feedbackMessage.c_str(), 400 - textWidth / 2, 20, 16, WHITE);
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}