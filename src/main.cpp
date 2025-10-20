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
    string id;
    string tipo;
    string nome;
    string specs;
    double preco;
};

vector<Component> catalog;
vector<Component> buildAtual;

struct Account {
    string username;
    string password;
};

vector<Account> accounts = {{"admin", "Programador"}};

enum Screen { LOGIN, REGISTER, MENU, ESCOLHER_PECAS, VER_PRECOS, COMO_MONTAR };
Screen currentScreen = LOGIN;

string loginUser = "";
string loginPass = "";
bool loginFocusUser = true;

string registerUser = "";
string registerPass = "";
string registerPassConfirm = "";
int registerFocusField = 0; // 0=user, 1=pass, 2=confirm
string registerMessage = "";

static inline string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
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
            if (c == ';') { 
                parts.push_back(cur); 
                cur.clear(); 
            } else {
                cur.push_back(c);
            }
        }
        parts.push_back(cur);
        
        if (parts.size() < 5) continue;
        
        Component c;
        c.id = trim(parts[0]);
        c.tipo = trim(parts[1]);
        c.nome = trim(parts[2]);
        c.specs = trim(parts[3]);
        try { 
            c.preco = stod(trim(parts[4])); 
        } catch(...) { 
            c.preco = 0.0; 
        }
        
        catalog.push_back(c);
    }
    return true;
}

string formatBR(double v) {
    stringstream ss;
    ss << fixed << setprecision(2) << v;
    string s = ss.str();
    
    size_t dotPos = s.find('.');
    if (dotPos != string::npos) {
        s[dotPos] = ',';
    }
    
    return "R$ " + s;
}

double calcularTotal() {
    double total = 0.0;
    for (auto &c : buildAtual) {
        total += c.preco;
    }
    return total;
}

void drawLoginScreen() {
    DrawText("=== BuildComputer ===", 250, 100, 30, DARKBLUE);
    DrawText("Login", 350, 180, 20, DARKGRAY);
    
    // Campo usuário
    DrawText("Usuario:", 200, 250, 20, BLACK);
    Rectangle userBox = {200, 280, 400, 40};
    DrawRectangleRec(userBox, loginFocusUser ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(userBox, 2, loginFocusUser ? BLUE : GRAY);
    DrawText(loginUser.c_str(), 210, 290, 20, BLACK);
    
    // Campo senha
    DrawText("Senha:", 200, 350, 20, BLACK);
    Rectangle passBox = {200, 380, 400, 40};
    DrawRectangleRec(passBox, !loginFocusUser ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(passBox, 2, !loginFocusUser ? BLUE : GRAY);
    
    string maskedPass(loginPass.length(), '*');
    DrawText(maskedPass.c_str(), 210, 390, 20, BLACK);
    
    // Botão login
    Rectangle btnLogin = {250, 460, 140, 50};
    DrawRectangleRec(btnLogin, DARKBLUE);
    DrawText("ENTRAR", 280, 475, 20, WHITE);
    
    // Botão criar conta
    Rectangle btnRegister = {410, 460, 140, 50};
    DrawRectangleRec(btnRegister, GREEN);
    DrawText("CRIAR CONTA", 420, 475, 16, WHITE);
    
}

void drawRegisterScreen() {
    DrawText("=== Criar Nova Conta ===", 230, 80, 30, DARKBLUE);
    
    // Campo usuário
    DrawText("Usuario:", 200, 180, 20, BLACK);
    Rectangle userBox = {200, 210, 400, 40};
    DrawRectangleRec(userBox, registerFocusField == 0 ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(userBox, 2, registerFocusField == 0 ? BLUE : GRAY);
    DrawText(registerUser.c_str(), 210, 220, 20, BLACK);
    
    // Campo senha
    DrawText("Senha:", 200, 270, 20, BLACK);
    Rectangle passBox = {200, 300, 400, 40};
    DrawRectangleRec(passBox, registerFocusField == 1 ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(passBox, 2, registerFocusField == 1 ? BLUE : GRAY);
    string maskedPass(registerPass.length(), '*');
    DrawText(maskedPass.c_str(), 210, 310, 20, BLACK);
    
    // Campo confirmar senha
    DrawText("Confirmar Senha:", 200, 360, 20, BLACK);
    Rectangle confirmBox = {200, 390, 400, 40};
    DrawRectangleRec(confirmBox, registerFocusField == 2 ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(confirmBox, 2, registerFocusField == 2 ? BLUE : GRAY);
    string maskedConfirm(registerPassConfirm.length(), '*');
    DrawText(maskedConfirm.c_str(), 210, 400, 20, BLACK);
    
    // Mensagem de erro/sucesso
    if (!registerMessage.empty()) {
        Color msgColor = (registerMessage.find("sucesso") != string::npos) ? GREEN : RED;
        DrawText(registerMessage.c_str(), 200, 450, 18, msgColor);
    }
    
    // Botões
    Rectangle btnCreate = {250, 500, 140, 50};
    DrawRectangleRec(btnCreate, GREEN);
    DrawText("CRIAR", 290, 515, 20, WHITE);
    
    Rectangle btnBack = {410, 500, 140, 50};
    DrawRectangleRec(btnBack, GRAY);
    DrawText("VOLTAR", 440, 515, 20, WHITE);
}

void drawMenuScreen() {
    DrawText("=== MENU PRINCIPAL ===", 250, 80, 30, DARKBLUE);
    
    Rectangle btn1 = {250, 180, 300, 60};
    Rectangle btn2 = {250, 260, 300, 60};
    Rectangle btn3 = {250, 340, 300, 60};
    Rectangle btn4 = {250, 420, 300, 60};
    
    DrawRectangleRec(btn1, BLUE);
    DrawText("Como Montar", 310, 200, 20, WHITE);
    
    DrawRectangleRec(btn2, GREEN);
    DrawText("Escolher Pecas", 295, 280, 20, WHITE);
    
    DrawRectangleRec(btn3, ORANGE);
    DrawText("Ver Precos", 315, 360, 20, WHITE);
    
    DrawRectangleRec(btn4, RED);
    DrawText("Sair", 360, 440, 20, WHITE);
}

void drawEscolherPecasScreen() {
    DrawText("=== ESCOLHER PECAS ===", 230, 30, 30, DARKBLUE);
    
    string buildInfo = "Build: " + to_string(buildAtual.size()) + " itens | Total: " + formatBR(calcularTotal());
    DrawText(buildInfo.c_str(), 50, 80, 18, DARKGRAY);
    
    int y = 130;
    for (size_t i = 0; i < catalog.size() && i < 8; i++) {
        auto &c = catalog[i];
        string line = c.id + " | " + c.tipo + " | " + c.nome + " | " + formatBR(c.preco);
        DrawText(line.c_str(), 50, y, 16, BLACK);
        
        Rectangle btnAdd = {650, (float)y - 5, 100, 30};
        DrawRectangleRec(btnAdd, GREEN);
        DrawText("Adicionar", 660, y, 16, WHITE);
        
        y += 40;
    }
    
    Rectangle btnBack = {300, 520, 200, 50};
    DrawRectangleRec(btnBack, GRAY);
    DrawText("VOLTAR", 355, 535, 20, WHITE);
}

void drawVerPrecosScreen() {
    DrawText("=== VER PRECOS ===", 270, 30, 30, DARKBLUE);
    
    if (buildAtual.empty()) {
        DrawText("Nenhum componente no build.", 250, 200, 20, RED);
    } else {
        int y = 100;
        for (auto &b : buildAtual) {
            string line = b.tipo + " | " + b.nome + " | " + formatBR(b.preco);
            DrawText(line.c_str(), 50, y, 18, BLACK);
            y += 35;
        }
        
        DrawText("-----------------------------------", 50, y, 18, DARKGRAY);
        y += 30;
        string total = "TOTAL: " + formatBR(calcularTotal());
        DrawText(total.c_str(), 50, y, 24, DARKGREEN);
    }
    
    Rectangle btnBack = {300, 520, 200, 50};
    DrawRectangleRec(btnBack, GRAY);
    DrawText("VOLTAR", 355, 535, 20, WHITE);
}

void drawComoMontarScreen() {
    DrawText("=== COMO MONTAR ===", 260, 30, 30, DARKBLUE);
    
    DrawText("Guia de Montagem:", 50, 100, 22, BLACK);
    DrawText("1. Escolha um processador (CPU)", 50, 150, 18, DARKGRAY);
    DrawText("2. Escolha uma placa mae (Motherboard)", 50, 180, 18, DARKGRAY);
    DrawText("3. Adicione memoria RAM (ate 4 modulos)", 50, 210, 18, DARKGRAY);
    DrawText("4. Escolha uma placa de video (GPU)", 50, 240, 18, DARKGRAY);
    DrawText("5. Adicione armazenamento (Storage/SSD)", 50, 270, 18, DARKGRAY);
    DrawText("6. Escolha uma fonte (PSU)", 50, 300, 18, DARKGRAY);
    
    DrawText("Dica: Use 'Escolher Pecas' para montar seu PC!", 50, 360, 16, BLUE);
    
    Rectangle btnBack = {300, 520, 200, 50};
    DrawRectangleRec(btnBack, GRAY);
    DrawText("VOLTAR", 355, 535, 20, WHITE);
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    
    InitWindow(screenWidth, screenHeight, "BuildComputer - Raylib");
    SetTargetFPS(60);
    
    // Carregar componentes
    if (!loadComponents("components.realyb")) {
        TraceLog(LOG_WARNING, "Arquivo components.realyb nao encontrado. Usando dados de exemplo.");
        // Adicionar alguns componentes de exemplo
        catalog.push_back({"cpu1", "CPU", "Intel i5-12400", "6-core 2.5GHz", 1299.90});
        catalog.push_back({"cpu2", "CPU", "AMD Ryzen 5 5600X", "6-core 3.7GHz", 1399.90});
        catalog.push_back({"ram1", "RAM", "Corsair 16GB DDR4", "3200MHz", 399.90});
        catalog.push_back({"gpu1", "GPU", "RTX 3060 Ti", "8GB GDDR6", 2499.90});
        catalog.push_back({"mb1", "Motherboard", "ASUS B550", "AM4 ATX", 899.90});
    }
    
    while (!WindowShouldClose()) {
        // Input
        if (currentScreen == LOGIN) {
            // Detectar clique nas caixas de texto
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mousePos = GetMousePosition();
                Rectangle userBox = {200, 280, 400, 40};
                Rectangle passBox = {200, 380, 400, 40};
                Rectangle btnLogin = {300, 460, 200, 50};
                
                // Clicar na caixa de usuário
                if (CheckCollisionPointRec(mousePos, userBox)) {
                    loginFocusUser = true;
                }
                // Clicar na caixa de senha
                else if (CheckCollisionPointRec(mousePos, passBox)) {
                    loginFocusUser = false;
                }
                // Clicar no botão de login
                else if (CheckCollisionPointRec(mousePos, btnLogin)) {
                    if (loginUser == "admin" && loginPass == "Programador") {
                        currentScreen = MENU;
                    }
                }
            }
            
            // Digitar texto
            int key = GetCharPressed();
            while (key > 0) {
                if (key >= 32 && key <= 125) {
                    if (loginFocusUser) {
                        loginUser += (char)key;
                    } else {
                        loginPass += (char)key;
                    }
                }
                key = GetCharPressed();
            }
            
            // Apagar texto
            if (IsKeyPressed(KEY_BACKSPACE)) {
                if (loginFocusUser && !loginUser.empty()) {
                    loginUser.pop_back();
                } else if (!loginFocusUser && !loginPass.empty()) {
                    loginPass.pop_back();
                }
            }
            
            // TAB para alternar entre campos
            if (IsKeyPressed(KEY_TAB)) {
                loginFocusUser = !loginFocusUser;
            }
            
            // ENTER para fazer login
            if (IsKeyPressed(KEY_ENTER)) {
                if (loginUser == "admin" && loginPass == "Programador") {
                    currentScreen = MENU;
                }
            }
        }
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            
            if (currentScreen == MENU) {
                if (CheckCollisionPointRec(mousePos, {250, 180, 300, 60})) currentScreen = COMO_MONTAR;
                if (CheckCollisionPointRec(mousePos, {250, 260, 300, 60})) currentScreen = ESCOLHER_PECAS;
                if (CheckCollisionPointRec(mousePos, {250, 340, 300, 60})) currentScreen = VER_PRECOS;
                if (CheckCollisionPointRec(mousePos, {250, 420, 300, 60})) break;
            }
            
            if (currentScreen != LOGIN && currentScreen != MENU) {
                if (CheckCollisionPointRec(mousePos, {300, 520, 200, 50})) {
                    currentScreen = MENU;
                }
            }
        }
        
        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        switch (currentScreen) {
            case LOGIN: drawLoginScreen(); break;
            case MENU: drawMenuScreen(); break;
            case ESCOLHER_PECAS: drawEscolherPecasScreen(); break;
            case VER_PRECOS: drawVerPrecosScreen(); break;
            case COMO_MONTAR: drawComoMontarScreen(); break;
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}