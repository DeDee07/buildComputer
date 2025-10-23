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
    string username, password, email, phone;
    bool isAdmin;
};

vector<Component> catalog, buildAtual;
vector<Account> accounts;
string currentUser = "";
bool isCurrentUserAdmin = false;

enum Screen { LOGIN, REGISTER, MENU, ESCOLHER_PECAS, VER_PRECOS, COMO_MONTAR, ADMIN_PANEL, ADMIN_ADD, ADMIN_EDIT, ADMIN_DELETE };
Screen currentScreen = LOGIN;

// Login/Register fields
string loginUser = "", loginPass = "";
bool loginFocusUser = true;
string registerUser = "", registerPass = "", registerPassConfirm = "", registerEmail = "", registerPhone = "", registerMessage = "";
<<<<<<< Updated upstream
int registerFocusField = 0;
=======
int registerFocusField = 0; // 0=user, 1=email, 2=phone, 3=pass, 4=confirm
>>>>>>> Stashed changes

// Admin fields
string adminId = "", adminTipo = "", adminNome = "", adminSpecs = "", adminPreco = "", adminMessage = "";
int adminFocusField = 0, adminSelectedIndex = -1, adminScrollOffset = 0;

// UI fields
string filtroCategoria = "Todos", feedbackMessage = "";
int pecasScrollOffset = 0, feedbackTimer = 0;

<<<<<<< Updated upstream
// Função para obter centro da tela
int getCenterX() { return GetScreenWidth() / 2; }
int getCenterY() { return GetScreenHeight() / 2; }

=======
>>>>>>> Stashed changes
static inline string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

// === DATABASE FUNCTIONS ===

void saveAccounts() {
    ofstream f("accounts.db");
<<<<<<< Updated upstream
    if (!f.is_open()) return;
    for (auto &acc : accounts)
        f << acc.username << ";" << acc.password << ";" << acc.email << ";" << acc.phone << ";" << (acc.isAdmin ? "1" : "0") << "\n";
    f.close();
}

void loadAccounts() {
    ifstream f("accounts.db");
    if (!f.is_open()) {
        accounts.push_back({"admin", "Programador", "admin@buildpc.com", "912345678", true});
        accounts.push_back({"admin2", "adminProgramador", "admin2@buildpc.com", "913456789", true});
        saveAccounts();
        return;
    }
    
    accounts.clear();
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
        
        if (parts.size() >= 5) {
            Account acc;
            acc.username = trim(parts[0]);
            acc.password = trim(parts[1]);
            acc.email = trim(parts[2]);
            acc.phone = trim(parts[3]);
            acc.isAdmin = (trim(parts[4]) == "1");
            accounts.push_back(acc);
        }
    }
    f.close();
}

bool validateEmail(const string &email) {
    if (email.empty()) return false;
    size_t atPos = email.find('@');
    if (atPos == string::npos || atPos == 0 || atPos == email.length() - 1) return false;
    size_t dotPos = email.find('.', atPos);
    return !(dotPos == string::npos || dotPos == email.length() - 1);
}

bool validatePhone(const string &phone) {
    if (phone.length() < 9) return false;
    for (char c : phone)
        if (!isdigit(c) && c != '+' && c != ' ' && c != '-') return false;
    return true;
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
    DrawText(label, (int)box.x, (int)box.y - 25, 18, BLACK);
    DrawRectangleRec(box, focused ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(box, 2, focused ? BLUE : GRAY);
    string display = maskPassword ? string(text.length(), '*') : text;
    DrawText(display.c_str(), (int)box.x + 10, (int)box.y + 10, 18, BLACK);
}

void drawButton(Rectangle btn, const char* text, Color color) {
    DrawRectangleRec(btn, color);
    int textWidth = MeasureText(text, 20);
    DrawText(text, (int)btn.x + (int)(btn.width - textWidth) / 2, (int)btn.y + 15, 20, WHITE);
}

void drawLoginScreen() {
    int cx = getCenterX();
    int cy = getCenterY();
    
    int titleWidth = MeasureText("=== BuildComputer ===", 30);
    DrawText("=== BuildComputer ===", cx - titleWidth/2, cy - 200, 30, DARKBLUE);
    
    int subtitleWidth = MeasureText("Login", 20);
    DrawText("Login", cx - subtitleWidth/2, cy - 120, 20, DARKGRAY);
    
    drawTextBox("Usuario:", loginUser, {(float)(cx - 200), (float)(cy - 20), 400, 40}, loginFocusUser);
    drawTextBox("Senha:", loginPass, {(float)(cx - 200), (float)(cy + 80), 400, 40}, !loginFocusUser, true);
    
    drawButton({(float)(cx - 90), (float)(cy + 160), 140, 50}, "ENTRAR", DARKBLUE);
    drawButton({(float)(cx + 70), (float)(cy + 160), 140, 50}, "CRIAR CONTA", GREEN);
}

void drawRegisterScreen() {
    int cx = getCenterX();
    int cy = getCenterY();
    
    int titleWidth = MeasureText("=== Criar Nova Conta ===", 28);
    DrawText("=== Criar Nova Conta ===", cx - titleWidth/2, cy - 260, 28, DARKBLUE);
    
    drawTextBox("Usuario:", registerUser, {(float)(cx - 200), (float)(cy - 170), 400, 38}, registerFocusField == 0);
    drawTextBox("Email:", registerEmail, {(float)(cx - 200), (float)(cy - 100), 400, 38}, registerFocusField == 1);
    drawTextBox("Telemovel:", registerPhone, {(float)(cx - 200), (float)(cy - 30), 400, 38}, registerFocusField == 2);
    drawTextBox("Senha:", registerPass, {(float)(cx - 200), (float)(cy + 40), 400, 38}, registerFocusField == 3, true);
    drawTextBox("Confirmar Senha:", registerPassConfirm, {(float)(cx - 200), (float)(cy + 110), 400, 38}, registerFocusField == 4, true);
    
    if (!registerMessage.empty()) {
        Color msgColor = (registerMessage.find("sucesso") != string::npos) ? GREEN : RED;
        int msgWidth = MeasureText(registerMessage.c_str(), 16);
        DrawText(registerMessage.c_str(), cx - msgWidth/2, cy + 170, 16, msgColor);
    }
    
    drawButton({(float)(cx - 90), (float)(cy + 210), 140, 45}, "CRIAR", GREEN);
    drawButton({(float)(cx + 70), (float)(cy + 210), 140, 45}, "VOLTAR", GRAY);
}

void drawMenuScreen() {
    int cx = getCenterX();
    int cy = getCenterY();
    
    string welcome = "Bem-vindo, " + currentUser + (isCurrentUserAdmin ? " (Admin)" : "");
    int welcomeWidth = MeasureText(welcome.c_str(), 20);
    DrawText(welcome.c_str(), cx - welcomeWidth/2, cy - 270, 20, DARKGRAY);
    
    int titleWidth = MeasureText("=== MENU PRINCIPAL ===", 30);
    DrawText("=== MENU PRINCIPAL ===", cx - titleWidth/2, cy - 220, 30, DARKBLUE);
=======
    if (!f.is_open()) {
        cout << "Erro ao salvar contas!" << endl;
        return;
    }
    
    for (auto &acc : accounts) {
        f << acc.username << ";"
          << acc.password << ";"
          << acc.email << ";"
          << acc.phone << ";"
          << (acc.isAdmin ? "1" : "0") << "\n";
    }
    f.close();
    cout << "Contas salvas com sucesso!" << endl;
}

void loadAccounts() {
    ifstream f("accounts.db");
    
    // Se o arquivo não existe, criar contas admin padrão
    if (!f.is_open()) {
        accounts.push_back({"admin", "Programador", "admin@buildpc.com", "912345678", true});
        accounts.push_back({"admin2", "adminProgramador", "admin2@buildpc.com", "913456789", true});
        saveAccounts();
        cout << "Base de dados criada com contas admin padrao" << endl;
        return;
    }
    
    accounts.clear();
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
        
        if (parts.size() >= 5) {
            Account acc;
            acc.username = trim(parts[0]);
            acc.password = trim(parts[1]);
            acc.email = trim(parts[2]);
            acc.phone = trim(parts[3]);
            acc.isAdmin = (trim(parts[4]) == "1");
            accounts.push_back(acc);
        }
    }
    f.close();
    cout << "Carregadas " << accounts.size() << " contas da base de dados" << endl;
}

bool validateEmail(const string &email) {
    if (email.empty()) return false;
    size_t atPos = email.find('@');
    if (atPos == string::npos || atPos == 0 || atPos == email.length() - 1) return false;
    size_t dotPos = email.find('.', atPos);
    if (dotPos == string::npos || dotPos == email.length() - 1) return false;
    return true;
}

bool validatePhone(const string &phone) {
    if (phone.length() < 9) return false;
    for (char c : phone) {
        if (!isdigit(c) && c != '+' && c != ' ' && c != '-') return false;
    }
    return true;
}

// === COMPONENT FUNCTIONS ===

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

// === DRAWING FUNCTIONS ===

void drawTextBox(const char* label, string &text, Rectangle box, bool focused, bool maskPassword = false) {
    DrawText(label, (int)box.x, (int)box.y - 25, 18, BLACK);
    DrawRectangleRec(box, focused ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(box, 2, focused ? BLUE : GRAY);
    string display = maskPassword ? string(text.length(), '*') : text;
    DrawText(display.c_str(), (int)box.x + 10, (int)box.y + 10, 18, BLACK);
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
    DrawText("=== Criar Nova Conta ===", 230, 40, 28, DARKBLUE);
    
    drawTextBox("Usuario:", registerUser, {200, 130, 400, 38}, registerFocusField == 0);
    drawTextBox("Email:", registerEmail, {200, 200, 400, 38}, registerFocusField == 1);
    drawTextBox("Telemovel:", registerPhone, {200, 270, 400, 38}, registerFocusField == 2);
    drawTextBox("Senha:", registerPass, {200, 340, 400, 38}, registerFocusField == 3, true);
    drawTextBox("Confirmar Senha:", registerPassConfirm, {200, 410, 400, 38}, registerFocusField == 4, true);
    
    if (!registerMessage.empty()) {
        Color msgColor = (registerMessage.find("sucesso") != string::npos) ? GREEN : RED;
        DrawText(registerMessage.c_str(), 200, 470, 16, msgColor);
    }
    
    drawButton({250, 510, 140, 45}, "CRIAR", GREEN);
    drawButton({410, 510, 140, 45}, "VOLTAR", GRAY);
}

void drawMenuScreen() {
    string welcome = "Bem-vindo, " + currentUser + (isCurrentUserAdmin ? " (Admin)" : "");
    DrawText(welcome.c_str(), 50, 30, 20, DARKGRAY);
    DrawText("=== MENU PRINCIPAL ===", 250, 80, 30, DARKBLUE);
>>>>>>> Stashed changes
    
    if (isCurrentUserAdmin) {
        const char* labels[] = {"Como Montar", "Escolher Pecas", "Ver Precos", "Painel Admin", "Sair"};
        Color colors[] = {BLUE, GREEN, ORANGE, PURPLE, RED};
        
        for (int i = 0; i < 5; i++) {
<<<<<<< Updated upstream
            int textWidth = MeasureText(labels[i], 20);
            Rectangle btn = {(float)(cx - 150), (float)(cy - 120 + i * 80), 300, 60};
            DrawRectangleRec(btn, colors[i]);
            DrawText(labels[i], cx - textWidth/2, cy - 100 + i * 80, 20, WHITE);
=======
            Rectangle btn = {250, 160.0f + i * 80, 300, 60};
            DrawRectangleRec(btn, colors[i]);
            int textWidth = MeasureText(labels[i], 20);
            DrawText(labels[i], 250 + (300 - textWidth) / 2, 180 + i * 80, 20, WHITE);
>>>>>>> Stashed changes
        }
    } else {
        const char* labels[] = {"Como Montar", "Escolher Pecas", "Ver Precos", "Sair"};
        Color colors[] = {BLUE, GREEN, ORANGE, RED};
        
        for (int i = 0; i < 4; i++) {
<<<<<<< Updated upstream
            int textWidth = MeasureText(labels[i], 20);
            Rectangle btn = {(float)(cx - 150), (float)(cy - 90 + i * 90), 300, 60};
            DrawRectangleRec(btn, colors[i]);
            DrawText(labels[i], cx - textWidth/2, cy - 70 + i * 90, 20, WHITE);
=======
            Rectangle btn = {250, 180.0f + i * 90, 300, 60};
            DrawRectangleRec(btn, colors[i]);
            int textWidth = MeasureText(labels[i], 20);
            DrawText(labels[i], 250 + (300 - textWidth) / 2, 200 + i * 90, 20, WHITE);
>>>>>>> Stashed changes
        }
    }
}

void drawAdminPanelScreen() {
<<<<<<< Updated upstream
    int cx = getCenterX();
    
    int titleWidth = MeasureText("=== PAINEL ADMINISTRATIVO ===", 28);
    DrawText("=== PAINEL ADMINISTRATIVO ===", cx - titleWidth/2, 20, 28, PURPLE);
    DrawText(TextFormat("Total: %d componentes", (int)catalog.size()), cx - 100, 70, 18, DARKGRAY);
=======
    DrawText("=== PAINEL ADMINISTRATIVO ===", 180, 20, 28, PURPLE);
    DrawText(TextFormat("Total: %d componentes", (int)catalog.size()), 50, 70, 18, DARKGRAY);
>>>>>>> Stashed changes
    
    const char* labels[] = {"ADICIONAR", "EDITAR", "EXCLUIR", "VOLTAR"};
    Color colors[] = {GREEN, BLUE, RED, GRAY};
    
    for (int i = 0; i < 4; i++) {
<<<<<<< Updated upstream
        Rectangle btn = {(float)(cx - 340 + i * 170), 110, 150, 45};
        drawButton(btn, labels[i], colors[i]);
    }
    
    DrawLine(cx - 350, 170, cx + 350, 170, DARKGRAY);
=======
        Rectangle btn = {50.0f + i * 170, 110, 150, 45};
        drawButton(btn, labels[i], colors[i]);
    }
    
    DrawLine(50, 170, 750, 170, DARKGRAY);
>>>>>>> Stashed changes
    
    int y = 190, maxVisible = 8;
    for (size_t i = adminScrollOffset; i < catalog.size() && i < (size_t)(adminScrollOffset + maxVisible); i++) {
        auto &c = catalog[i];
        Color bgColor = ((int)i == adminSelectedIndex) ? LIGHTGRAY : WHITE;
<<<<<<< Updated upstream
        DrawRectangle(cx - 350, y - 5, 700, 35, bgColor);
        string line = TextFormat("[%d] %s | %s | %s", (int)i, c.tipo.c_str(), 
                                 c.nome.substr(0, 25).c_str(), formatBR(c.preco).c_str());
        DrawText(line.c_str(), cx - 345, y, 14, BLACK);
        y += 37;
    }
    
    if (catalog.size() > (size_t)maxVisible) {
        int msgWidth = MeasureText("Use SCROLL para navegar | Clique para selecionar", 14);
        DrawText("Use SCROLL para navegar | Clique para selecionar", cx - msgWidth/2, 550, 14, DARKGRAY);
    }
}

void drawAdminEditFields(const char* title) {
    int cx = getCenterX();
    
    int titleWidth = MeasureText(title, 28);
    DrawText(title, cx - titleWidth/2, 30, 28, currentScreen == ADMIN_ADD ? GREEN : BLUE);
=======
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
>>>>>>> Stashed changes
    
    const char* labels[] = {"ID:", "Tipo (CPU/GPU/RAM/Motherboard/Storage/PSU/Case/Cooler):", 
                           "Nome:", "Especificacoes:", "Preco (use ponto para decimal):"};
    string* fields[] = {&adminId, &adminTipo, &adminNome, &adminSpecs, &adminPreco};
    int yPos[] = {135, 210, 285, 360, 435};
    
    for (int i = 0; i < 5; i++) {
<<<<<<< Updated upstream
        DrawText(labels[i], cx - 250, yPos[i] - 25, i == 1 ? 16 : 18, BLACK);
        Rectangle box = {(float)(cx - 250), (float)yPos[i], 500, 35};
        DrawRectangleRec(box, adminFocusField == i ? LIGHTGRAY : WHITE);
        DrawRectangleLinesEx(box, 2, adminFocusField == i ? BLUE : GRAY);
        DrawText(fields[i]->c_str(), cx - 240, yPos[i] + 8, 18, BLACK);
=======
        DrawText(labels[i], 150, yPos[i] - 25, i == 1 ? 16 : 18, BLACK);
        Rectangle box = {150, (float)yPos[i], 500, 35};
        DrawRectangleRec(box, adminFocusField == i ? LIGHTGRAY : WHITE);
        DrawRectangleLinesEx(box, 2, adminFocusField == i ? BLUE : GRAY);
        DrawText(fields[i]->c_str(), 160, yPos[i] + 8, 18, BLACK);
>>>>>>> Stashed changes
    }
    
    if (!adminMessage.empty()) {
        Color msgColor = (adminMessage.find("sucesso") != string::npos) ? GREEN : RED;
<<<<<<< Updated upstream
        int msgWidth = MeasureText(adminMessage.c_str(), 16);
        DrawText(adminMessage.c_str(), cx - msgWidth/2, 490, 16, msgColor);
    }
    
    drawButton({(float)(cx - 90), 520, 130, 45}, "SALVAR", currentScreen == ADMIN_ADD ? GREEN : BLUE);
    drawButton({(float)(cx + 60), 520, 130, 45}, "CANCELAR", GRAY);
=======
        DrawText(adminMessage.c_str(), 150, 490, 16, msgColor);
    }
    
    drawButton({250, 520, 130, 45}, "SALVAR", currentScreen == ADMIN_ADD ? GREEN : BLUE);
    drawButton({420, 520, 130, 45}, "CANCELAR", GRAY);
>>>>>>> Stashed changes
}

void drawAdminAddScreen() { drawAdminEditFields("=== ADICIONAR COMPONENTE ==="); }

void drawAdminEditScreen() {
<<<<<<< Updated upstream
    int cx = getCenterX();
    if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
        drawAdminEditFields("=== EDITAR COMPONENTE ===");
    } else {
        int titleWidth = MeasureText("=== EDITAR COMPONENTE ===", 28);
        DrawText("=== EDITAR COMPONENTE ===", cx - titleWidth/2, 30, 28, BLUE);
        int msgWidth = MeasureText("Nenhum componente selecionado!", 20);
        DrawText("Nenhum componente selecionado!", cx - msgWidth/2, 300, 20, RED);
        drawButton({(float)(cx - 100), 400, 200, 50}, "VOLTAR", GRAY);
=======
    if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
        drawAdminEditFields("=== EDITAR COMPONENTE ===");
    } else {
        DrawText("=== EDITAR COMPONENTE ===", 220, 30, 28, BLUE);
        DrawText("Nenhum componente selecionado!", 250, 300, 20, RED);
        drawButton({300, 400, 200, 50}, "VOLTAR", GRAY);
>>>>>>> Stashed changes
    }
}

void drawAdminDeleteScreen() {
<<<<<<< Updated upstream
    int cx = getCenterX();
    int titleWidth = MeasureText("=== EXCLUIR COMPONENTE ===", 28);
    DrawText("=== EXCLUIR COMPONENTE ===", cx - titleWidth/2, 30, 28, RED);
    
    if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
        auto &c = catalog[adminSelectedIndex];
        int msgWidth = MeasureText("Confirmar exclusao do seguinte componente:", 18);
        DrawText("Confirmar exclusao do seguinte componente:", cx - msgWidth/2, 120, 18, BLACK);
=======
    DrawText("=== EXCLUIR COMPONENTE ===", 210, 30, 28, RED);
    
    if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
        auto &c = catalog[adminSelectedIndex];
        DrawText("Confirmar exclusao do seguinte componente:", 150, 120, 18, BLACK);
>>>>>>> Stashed changes
        
        const char* labels[] = {"ID:", "Tipo:", "Nome:", "Specs:", "Preco:"};
        string values[] = {c.id, c.tipo, c.nome, c.specs, formatBR(c.preco)};
        
<<<<<<< Updated upstream
        for (int i = 0; i < 5; i++) {
            string text = string(labels[i]) + " " + values[i];
            int textWidth = MeasureText(text.c_str(), 16);
            DrawText(text.c_str(), cx - textWidth/2, 170 + i * 30, 16, DARKGRAY);
        }
        
        int warnWidth = MeasureText("Esta acao nao pode ser desfeita!", 18);
        DrawText("Esta acao nao pode ser desfeita!", cx - warnWidth/2, 350, 18, RED);
        
        if (!adminMessage.empty()) {
            Color msgColor = (adminMessage.find("sucesso") != string::npos) ? GREEN : RED;
            int msgWidth2 = MeasureText(adminMessage.c_str(), 16);
            DrawText(adminMessage.c_str(), cx - msgWidth2/2, 400, 16, msgColor);
        }
        
        drawButton({(float)(cx - 90), 450, 130, 45}, "EXCLUIR", RED);
        drawButton({(float)(cx + 60), 450, 130, 45}, "CANCELAR", GRAY);
    } else {
        int msgWidth = MeasureText("Nenhum componente selecionado!", 20);
        DrawText("Nenhum componente selecionado!", cx - msgWidth/2, 300, 20, RED);
        drawButton({(float)(cx - 100), 400, 200, 50}, "VOLTAR", GRAY);
=======
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
>>>>>>> Stashed changes
    }
}

void drawEscolherPecasScreen() {
<<<<<<< Updated upstream
    int cx = getCenterX();
    int sw = GetScreenWidth();
    
    int titleWidth = MeasureText("=== ESCOLHER PECAS ===", 28);
    DrawText("=== ESCOLHER PECAS ===", cx - titleWidth/2, 20, 28, GREEN);
    
    DrawText("Filtro:", cx - sw/2 + 50, 70, 18, BLACK);
    vector<string> categorias = {"Todos", "CPU", "GPU", "RAM", "Motherboard", "Storage", "PSU", "Case", "Cooler"};
    int btnX = cx - sw/2 + 130;
=======
    DrawText("=== ESCOLHER PECAS ===", 250, 20, 28, GREEN);
    
    DrawText("Filtro:", 50, 70, 18, BLACK);
    vector<string> categorias = {"Todos", "CPU", "GPU", "RAM", "Motherboard", "Storage", "PSU", "Case", "Cooler"};
    int btnX = 130;
>>>>>>> Stashed changes
    for (auto &cat : categorias) {
        int btnWidth = MeasureText(cat.c_str(), 14) + 20;
        Rectangle btn = {(float)btnX, 65, (float)btnWidth, 30};
        Color btnColor = (filtroCategoria == cat) ? BLUE : LIGHTGRAY;
        DrawRectangleRec(btn, btnColor);
        DrawRectangleLinesEx(btn, 1, DARKGRAY);
        DrawText(cat.c_str(), btnX + 10, 72, 14, (filtroCategoria == cat) ? WHITE : BLACK);
        btnX += btnWidth + 5;
    }
    
<<<<<<< Updated upstream
    DrawLine(cx - 350, 110, cx + 350, 110, DARKGRAY);
=======
    DrawLine(50, 110, 750, 110, DARKGRAY);
>>>>>>> Stashed changes
    
    vector<Component> filtered;
    for (auto &c : catalog)
        if (filtroCategoria == "Todos" || c.tipo == filtroCategoria)
            filtered.push_back(c);
    
    int y = 130, maxVisible = 9;
    for (size_t i = pecasScrollOffset; i < filtered.size() && i < (size_t)(pecasScrollOffset + maxVisible); i++) {
        auto &c = filtered[i];
<<<<<<< Updated upstream
        Rectangle itemRect = {(float)(cx - 350), (float)(y - 5), 650, 40};
        DrawRectangleRec(itemRect, WHITE);
        DrawRectangleLinesEx(itemRect, 1, LIGHTGRAY);
        
        DrawText(c.nome.c_str(), cx - 340, y, 16, BLACK);
        DrawText(c.specs.c_str(), cx - 340, y + 18, 12, DARKGRAY);
        DrawText(formatBR(c.preco).c_str(), cx + 150, y + 8, 16, DARKGREEN);
        
        DrawRectangleRec({(float)(cx + 310), (float)y, 40, 35}, GREEN);
        DrawText("+", cx + 323, y + 8, 20, WHITE);
        y += 45;
    }
    
    if (filtered.size() > (size_t)maxVisible) {
        int msgWidth = MeasureText("Use SCROLL para navegar", 14);
        DrawText("Use SCROLL para navegar", cx - msgWidth/2, 555, 14, DARKGRAY);
    }
    
    DrawText(TextFormat("Build Atual (%d pecas)", (int)buildAtual.size()), cx - 350, 555, 16, DARKBLUE);
    DrawText(TextFormat("Total: %s", formatBR(calcularTotal()).c_str()), cx + 200, 555, 18, DARKGREEN);
    drawButton({(float)(cx + 250), 20, 100, 35}, "VOLTAR", GRAY);
}

void drawVerPrecosScreen() {
    int cx = getCenterX();
    
    int titleWidth = MeasureText("=== VER PRECOS DA BUILD ===", 28);
    DrawText("=== VER PRECOS DA BUILD ===", cx - titleWidth/2, 20, 28, ORANGE);
    
    if (buildAtual.empty()) {
        int msg1Width = MeasureText("Sua build esta vazia!", 20);
        DrawText("Sua build esta vazia!", cx - msg1Width/2, 300, 20, DARKGRAY);
        int msg2Width = MeasureText("Va em 'Escolher Pecas' para adicionar componentes.", 16);
        DrawText("Va em 'Escolher Pecas' para adicionar componentes.", cx - msg2Width/2, 340, 16, GRAY);
    } else {
        DrawLine(cx - 350, 70, cx + 350, 70, DARKGRAY);
=======
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
>>>>>>> Stashed changes
        
        int y = 90;
        for (size_t i = 0; i < buildAtual.size(); i++) {
            auto &c = buildAtual[i];
<<<<<<< Updated upstream
            DrawText(c.nome.c_str(), cx - 340, y, 16, BLACK);
            DrawText(c.specs.c_str(), cx - 340, y + 18, 12, DARKGRAY);
            DrawText(formatBR(c.preco).c_str(), cx + 150, y + 8, 16, DARKGREEN);
            
            DrawRectangleRec({(float)(cx + 310), (float)y, 40, 35}, RED);
            DrawText("X", cx + 323, y + 8, 20, WHITE);
            y += 45;
        }
        
        DrawLine(cx - 350, y + 10, cx + 350, y + 10, DARKGRAY);
        DrawText("TOTAL:", cx + 100, y + 30, 22, BLACK);
        DrawText(formatBR(calcularTotal()).c_str(), cx + 200, y + 30, 22, DARKGREEN);
        
        drawButton({(float)(cx - 350), (float)(y + 70), 150, 40}, "LIMPAR BUILD", RED);
    }
    
    drawButton({(float)(cx + 250), 20, 100, 35}, "VOLTAR", GRAY);
}

void drawComoMontarScreen() {
    int cx = getCenterX();
    
    int titleWidth = MeasureText("=== COMO MONTAR UM PC ===", 28);
    DrawText("=== COMO MONTAR UM PC ===", cx - titleWidth/2, 20, 28, BLUE);
=======
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
>>>>>>> Stashed changes
    
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
<<<<<<< Updated upstream
        DrawText(TextFormat("%d. %s", i + 1, steps[i]), cx - 340, y, 16, BLACK);
        y += 30;
        for (int j = 0; j < 3 && details[i][j][0] != '\0'; j++) {
            DrawText(TextFormat("   - %s", details[i][j]), cx - 330, y, 14, DARKGRAY);
=======
        DrawText(TextFormat("%d. %s", i + 1, steps[i]), 60, y, 16, BLACK);
        y += 30;
        for (int j = 0; j < 3 && details[i][j][0] != '\0'; j++) {
            DrawText(TextFormat("   - %s", details[i][j]), 70, y, 14, DARKGRAY);
>>>>>>> Stashed changes
            y += 20;
        }
        y += 10;
    }
    
<<<<<<< Updated upstream
    drawButton({(float)(cx - 100), 540, 200, 40}, "VOLTAR", GRAY);
=======
    drawButton({300, 540, 200, 40}, "VOLTAR", GRAY);
>>>>>>> Stashed changes
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
<<<<<<< Updated upstream
    InitWindow(800, 600, "BuildComputer System - Pressione F11 para Fullscreen");
    SetTargetFPS(60);
    
    loadAccounts();
=======
    InitWindow(800, 600, "BuildComputer System");
    SetTargetFPS(60);
    
    // Carregar base de dados de contas
    loadAccounts();
    
    // Carregar componentes
>>>>>>> Stashed changes
    if (!loadComponents("componentes.realyb"))
        cout << "Aviso: Arquivo componentes.realyb nao encontrado!" << endl;
    
    while (!WindowShouldClose()) {
<<<<<<< Updated upstream
        // Toggle fullscreen com F11
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }
        
        if (feedbackTimer > 0) {
            feedbackTimer--;
            if (feedbackTimer == 0) feedbackMessage = "";
        }
        
=======
        if (feedbackTimer > 0) {
            feedbackTimer--;
            if (feedbackTimer == 0) feedbackMessage = "";
        }
        
>>>>>>> Stashed changes
        Vector2 mousePos = GetMousePosition();
        bool mouseClick = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        float mouseWheel = GetMouseWheelMove();
        
<<<<<<< Updated upstream
        int cx = getCenterX();
        int cy = getCenterY();
        
=======
>>>>>>> Stashed changes
        // LOGIN SCREEN
        if (currentScreen == LOGIN) {
            if (loginFocusUser) handleTextInput(loginUser, 30);
            else handleTextInput(loginPass, 30);
            
            if (IsKeyPressed(KEY_TAB)) loginFocusUser = !loginFocusUser;
            
            if (mouseClick) {
<<<<<<< Updated upstream
                if (CheckCollisionPointRec(mousePos, {(float)(cx - 200), (float)(cy - 20), 400, 40})) loginFocusUser = true;
                else if (CheckCollisionPointRec(mousePos, {(float)(cx - 200), (float)(cy + 80), 400, 40})) loginFocusUser = false;
                else if (CheckCollisionPointRec(mousePos, {(float)(cx - 90), (float)(cy + 160), 140, 50})) {
=======
                if (CheckCollisionPointRec(mousePos, {200, 280, 400, 40})) loginFocusUser = true;
                else if (CheckCollisionPointRec(mousePos, {200, 380, 400, 40})) loginFocusUser = false;
                else if (CheckCollisionPointRec(mousePos, {250, 460, 140, 50})) {
>>>>>>> Stashed changes
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
<<<<<<< Updated upstream
                } else if (CheckCollisionPointRec(mousePos, {(float)(cx + 70), (float)(cy + 160), 140, 50})) {
=======
                } else if (CheckCollisionPointRec(mousePos, {410, 460, 140, 50})) {
>>>>>>> Stashed changes
                    currentScreen = REGISTER;
                    registerUser = registerPass = registerPassConfirm = registerEmail = registerPhone = registerMessage = "";
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
            else if (registerFocusField == 1) handleTextInput(registerEmail, 50);
            else if (registerFocusField == 2) handleTextInput(registerPhone, 20);
            else if (registerFocusField == 3) handleTextInput(registerPass, 30);
            else if (registerFocusField == 4) handleTextInput(registerPassConfirm, 30);
            
            if (IsKeyPressed(KEY_TAB)) registerFocusField = (registerFocusField + 1) % 5;
            
            if (mouseClick) {
<<<<<<< Updated upstream
                if (CheckCollisionPointRec(mousePos, {(float)(cx - 200), (float)(cy - 170), 400, 38})) registerFocusField = 0;
                else if (CheckCollisionPointRec(mousePos, {(float)(cx - 200), (float)(cy - 100), 400, 38})) registerFocusField = 1;
                else if (CheckCollisionPointRec(mousePos, {(float)(cx - 200), (float)(cy - 30), 400, 38})) registerFocusField = 2;
                else if (CheckCollisionPointRec(mousePos, {(float)(cx - 200), (float)(cy + 40), 400, 38})) registerFocusField = 3;
                else if (CheckCollisionPointRec(mousePos, {(float)(cx - 200), (float)(cy + 110), 400, 38})) registerFocusField = 4;
                else if (CheckCollisionPointRec(mousePos, {(float)(cx - 90), (float)(cy + 210), 140, 45})) {
=======
                if (CheckCollisionPointRec(mousePos, {200, 130, 400, 38})) registerFocusField = 0;
                else if (CheckCollisionPointRec(mousePos, {200, 200, 400, 38})) registerFocusField = 1;
                else if (CheckCollisionPointRec(mousePos, {200, 270, 400, 38})) registerFocusField = 2;
                else if (CheckCollisionPointRec(mousePos, {200, 340, 400, 38})) registerFocusField = 3;
                else if (CheckCollisionPointRec(mousePos, {200, 410, 400, 38})) registerFocusField = 4;
                else if (CheckCollisionPointRec(mousePos, {250, 510, 140, 45})) {
                    // Validação
>>>>>>> Stashed changes
                    if (registerUser.empty() || registerEmail.empty() || registerPhone.empty() || 
                        registerPass.empty() || registerPassConfirm.empty()) {
                        registerMessage = "Preencha todos os campos!";
                    } else if (!validateEmail(registerEmail)) {
                        registerMessage = "Email invalido!";
                    } else if (!validatePhone(registerPhone)) {
                        registerMessage = "Numero de telemovel invalido (min 9 digitos)!";
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
<<<<<<< Updated upstream
=======
                            // Criar nova conta
>>>>>>> Stashed changes
                            Account newAcc;
                            newAcc.username = registerUser;
                            newAcc.password = registerPass;
                            newAcc.email = registerEmail;
                            newAcc.phone = registerPhone;
                            newAcc.isAdmin = false;
                            accounts.push_back(newAcc);
<<<<<<< Updated upstream
                            saveAccounts();
                            registerMessage = "Conta criada com sucesso!";
                            feedbackTimer = 180;
                        }
                    }
                } else if (CheckCollisionPointRec(mousePos, {(float)(cx + 70), (float)(cy + 210), 140, 45})) {
=======
                            saveAccounts(); // Salvar na base de dados
                            registerMessage = "Conta criada com sucesso!";
                            feedbackTimer = 180;
                            cout << "Nova conta criada: " << registerUser << " | " << registerEmail << " | " << registerPhone << endl;
                        }
                    }
                } else if (CheckCollisionPointRec(mousePos, {410, 510, 140, 45})) {
>>>>>>> Stashed changes
                    currentScreen = LOGIN;
                }
            }
        }
        
        // MENU SCREEN
        else if (currentScreen == MENU) {
            if (mouseClick) {
<<<<<<< Updated upstream
                if (isCurrentUserAdmin) {
                    if (CheckCollisionPointRec(mousePos, {(float)(cx - 150), (float)(cy - 120), 300, 60})) {
                        currentScreen = COMO_MONTAR;
                    } else if (CheckCollisionPointRec(mousePos, {(float)(cx - 150), (float)(cy - 40), 300, 60})) {
                        currentScreen = ESCOLHER_PECAS;
                        pecasScrollOffset = 0;
                    } else if (CheckCollisionPointRec(mousePos, {(float)(cx - 150), (float)(cy + 40), 300, 60})) {
                        currentScreen = VER_PRECOS;
                    } else if (CheckCollisionPointRec(mousePos, {(float)(cx - 150), (float)(cy + 120), 300, 60})) {
                        currentScreen = ADMIN_PANEL;
                        adminScrollOffset = 0;
                        adminSelectedIndex = -1;
                    } else if (CheckCollisionPointRec(mousePos, {(float)(cx - 150), (float)(cy + 200), 300, 60})) {
=======
                if (isCurrentUserAdmin) {if (CheckCollisionPointRec(mousePos, {250, 160, 300, 60})) {
                        currentScreen = COMO_MONTAR;
                    } else if (CheckCollisionPointRec(mousePos, {250, 240, 300, 60})) {
                        currentScreen = ESCOLHER_PECAS;
                        pecasScrollOffset = 0;
                    } else if (CheckCollisionPointRec(mousePos, {250, 320, 300, 60})) {
                        currentScreen = VER_PRECOS;
                    } else if (CheckCollisionPointRec(mousePos, {250, 400, 300, 60})) {
                        currentScreen = ADMIN_PANEL;
                        adminScrollOffset = 0;
                        adminSelectedIndex = -1;
                    } else if (CheckCollisionPointRec(mousePos, {250, 480, 300, 60})) {
>>>>>>> Stashed changes
                        currentUser = "";
                        isCurrentUserAdmin = false;
                        currentScreen = LOGIN;
                        buildAtual.clear();
                    }
                } else {
<<<<<<< Updated upstream
                    if (CheckCollisionPointRec(mousePos, {(float)(cx - 150), (float)(cy - 90), 300, 60})) {
                        currentScreen = COMO_MONTAR;
                    } else if (CheckCollisionPointRec(mousePos, {(float)(cx - 150), (float)cy, 300, 60})) {
                        currentScreen = ESCOLHER_PECAS;
                        pecasScrollOffset = 0;
                    } else if (CheckCollisionPointRec(mousePos, {(float)(cx - 150), (float)(cy + 90), 300, 60})) {
                        currentScreen = VER_PRECOS;
                    } else if (CheckCollisionPointRec(mousePos, {(float)(cx - 150), (float)(cy + 180), 300, 60})) {
=======
                    if (CheckCollisionPointRec(mousePos, {250, 180, 300, 60})) {
                        currentScreen = COMO_MONTAR;
                    } else if (CheckCollisionPointRec(mousePos, {250, 270, 300, 60})) {
                        currentScreen = ESCOLHER_PECAS;
                        pecasScrollOffset = 0;
                    } else if (CheckCollisionPointRec(mousePos, {250, 360, 300, 60})) {
                        currentScreen = VER_PRECOS;
                    } else if (CheckCollisionPointRec(mousePos, {250, 450, 300, 60})) {
>>>>>>> Stashed changes
                        currentUser = "";
                        isCurrentUserAdmin = false;
                        currentScreen = LOGIN;
                        buildAtual.clear();
                    }
                }
            }
        }
        
        // ESCOLHER PECAS SCREEN
        else if (currentScreen == ESCOLHER_PECAS) {
<<<<<<< Updated upstream
            if (mouseClick) {
                vector<string> categorias = {"Todos", "CPU", "GPU", "RAM", "Motherboard", "Storage", "PSU", "Case", "Cooler"};
                int btnX = cx - GetScreenWidth()/2 + 130;
=======
            // Filtro de categorias
            if (mouseClick) {
                vector<string> categorias = {"Todos", "CPU", "GPU", "RAM", "Motherboard", "Storage", "PSU", "Case", "Cooler"};
                int btnX = 130;
>>>>>>> Stashed changes
                for (auto &cat : categorias) {
                    int btnWidth = MeasureText(cat.c_str(), 14) + 20;
                    if (CheckCollisionPointRec(mousePos, {(float)btnX, 65, (float)btnWidth, 30})) {
                        filtroCategoria = cat;
                        pecasScrollOffset = 0;
                        break;
                    }
                    btnX += btnWidth + 5;
                }
                
<<<<<<< Updated upstream
                if (CheckCollisionPointRec(mousePos, {(float)(cx + 250), 20, 100, 35})) {
                    currentScreen = MENU;
                }
                
=======
                // Botão voltar
                if (CheckCollisionPointRec(mousePos, {650, 20, 100, 35})) {
                    currentScreen = MENU;
                }
                
                // Adicionar peças à build
>>>>>>> Stashed changes
                vector<Component> filtered;
                for (auto &c : catalog)
                    if (filtroCategoria == "Todos" || c.tipo == filtroCategoria)
                        filtered.push_back(c);
                
                int y = 130, maxVisible = 9;
                for (size_t i = pecasScrollOffset; i < filtered.size() && i < (size_t)(pecasScrollOffset + maxVisible); i++) {
<<<<<<< Updated upstream
                    if (CheckCollisionPointRec(mousePos, {(float)(cx + 310), (float)y, 40, 35})) {
=======
                    if (CheckCollisionPointRec(mousePos, {710, (float)y, 40, 35})) {
>>>>>>> Stashed changes
                        buildAtual.push_back(filtered[i]);
                        feedbackMessage = "Componente adicionado!";
                        feedbackTimer = 120;
                        break;
                    }
                    y += 45;
                }
            }
            
<<<<<<< Updated upstream
=======
            // Scroll
>>>>>>> Stashed changes
            if (mouseWheel != 0) {
                vector<Component> filtered;
                for (auto &c : catalog)
                    if (filtroCategoria == "Todos" || c.tipo == filtroCategoria)
                        filtered.push_back(c);
                
                int maxVisible = 9;
                pecasScrollOffset -= (int)mouseWheel;
                if (pecasScrollOffset < 0) pecasScrollOffset = 0;
                if (pecasScrollOffset > (int)filtered.size() - maxVisible) 
                    pecasScrollOffset = max(0, (int)filtered.size() - maxVisible);
            }
        }
        
        // VER PRECOS SCREEN
        else if (currentScreen == VER_PRECOS) {
            if (mouseClick) {
<<<<<<< Updated upstream
                if (CheckCollisionPointRec(mousePos, {(float)(cx + 250), 20, 100, 35})) {
                    currentScreen = MENU;
                }
                
                if (!buildAtual.empty()) {
                    int y = 90;
                    for (size_t i = 0; i < buildAtual.size(); i++) {
                        if (CheckCollisionPointRec(mousePos, {(float)(cx + 310), (float)y, 40, 35})) {
=======
                // Botão voltar
                if (CheckCollisionPointRec(mousePos, {650, 20, 100, 35})) {
                    currentScreen = MENU;
                }
                
                // Remover componentes
                if (!buildAtual.empty()) {
                    int y = 90;
                    for (size_t i = 0; i < buildAtual.size(); i++) {
                        if (CheckCollisionPointRec(mousePos, {710, (float)y, 40, 35})) {
>>>>>>> Stashed changes
                            buildAtual.erase(buildAtual.begin() + i);
                            feedbackMessage = "Componente removido!";
                            feedbackTimer = 120;
                            break;
                        }
                        y += 45;
                    }
                    
<<<<<<< Updated upstream
                    y = 90 + buildAtual.size() * 45;
                    if (CheckCollisionPointRec(mousePos, {(float)(cx - 350), (float)(y + 70), 150, 40})) {
=======
                    // Botão limpar build
                    y = 90 + buildAtual.size() * 45;
                    if (CheckCollisionPointRec(mousePos, {50, (float)y + 70, 150, 40})) {
>>>>>>> Stashed changes
                        buildAtual.clear();
                        feedbackMessage = "Build limpa!";
                        feedbackTimer = 120;
                    }
                }
            }
        }
        
        // COMO MONTAR SCREEN
        else if (currentScreen == COMO_MONTAR) {
<<<<<<< Updated upstream
            if (mouseClick && CheckCollisionPointRec(mousePos, {(float)(cx - 100), 540, 200, 40})) {
=======
            if (mouseClick && CheckCollisionPointRec(mousePos, {300, 540, 200, 40})) {
>>>>>>> Stashed changes
                currentScreen = MENU;
            }
        }
        
        // ADMIN PANEL SCREEN
        else if (currentScreen == ADMIN_PANEL) {
            if (mouseClick) {
<<<<<<< Updated upstream
                if (CheckCollisionPointRec(mousePos, {(float)(cx - 340), 110, 150, 45})) {
                    currentScreen = ADMIN_ADD;
                    adminId = adminTipo = adminNome = adminSpecs = adminPreco = adminMessage = "";
                    adminFocusField = 0;
                } else if (CheckCollisionPointRec(mousePos, {(float)(cx - 170), 110, 150, 45})) {
=======
                // Botões superiores
                if (CheckCollisionPointRec(mousePos, {50, 110, 150, 45})) {
                    currentScreen = ADMIN_ADD;
                    adminId = adminTipo = adminNome = adminSpecs = adminPreco = adminMessage = "";
                    adminFocusField = 0;
                } else if (CheckCollisionPointRec(mousePos, {220, 110, 150, 45})) {
>>>>>>> Stashed changes
                    if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
                        auto &c = catalog[adminSelectedIndex];
                        adminId = c.id;
                        adminTipo = c.tipo;
                        adminNome = c.nome;
                        adminSpecs = c.specs;
                        adminPreco = to_string(c.preco);
                        adminMessage = "";
                        adminFocusField = 0;
                        currentScreen = ADMIN_EDIT;
                    } else {
                        feedbackMessage = "Selecione um componente primeiro!";
                        feedbackTimer = 120;
                    }
<<<<<<< Updated upstream
                } else if (CheckCollisionPointRec(mousePos, {(float)cx, 110, 150, 45})) {
=======
                } else if (CheckCollisionPointRec(mousePos, {390, 110, 150, 45})) {
>>>>>>> Stashed changes
                    if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
                        currentScreen = ADMIN_DELETE;
                        adminMessage = "";
                    } else {
                        feedbackMessage = "Selecione um componente primeiro!";
                        feedbackTimer = 120;
                    }
<<<<<<< Updated upstream
                } else if (CheckCollisionPointRec(mousePos, {(float)(cx + 170), 110, 150, 45})) {
=======
                } else if (CheckCollisionPointRec(mousePos, {560, 110, 150, 45})) {
>>>>>>> Stashed changes
                    currentScreen = MENU;
                    adminSelectedIndex = -1;
                }
                
<<<<<<< Updated upstream
                int y = 190, maxVisible = 8;
                for (size_t i = adminScrollOffset; i < catalog.size() && i < (size_t)(adminScrollOffset + maxVisible); i++) {
                    if (CheckCollisionPointRec(mousePos, {(float)(cx - 350), (float)(y - 5), 700, 35})) {
=======
                // Selecionar componente na lista
                int y = 190, maxVisible = 8;
                for (size_t i = adminScrollOffset; i < catalog.size() && i < (size_t)(adminScrollOffset + maxVisible); i++) {
                    if (CheckCollisionPointRec(mousePos, {50, (float)y - 5, 700, 35})) {
>>>>>>> Stashed changes
                        adminSelectedIndex = i;
                        break;
                    }
                    y += 37;
                }
            }
            
<<<<<<< Updated upstream
=======
            // Scroll
>>>>>>> Stashed changes
            if (mouseWheel != 0) {
                int maxVisible = 8;
                adminScrollOffset -= (int)mouseWheel;
                if (adminScrollOffset < 0) adminScrollOffset = 0;
                if (adminScrollOffset > (int)catalog.size() - maxVisible)
                    adminScrollOffset = max(0, (int)catalog.size() - maxVisible);
            }
        }
        
        // ADMIN ADD SCREEN
        else if (currentScreen == ADMIN_ADD) {
            if (adminFocusField == 0) handleTextInput(adminId, 20);
            else if (adminFocusField == 1) handleTextInput(adminTipo, 30);
            else if (adminFocusField == 2) handleTextInput(adminNome, 50);
            else if (adminFocusField == 3) handleTextInput(adminSpecs, 100);
            else if (adminFocusField == 4) handleTextInput(adminPreco, 20);
            
            if (IsKeyPressed(KEY_TAB)) adminFocusField = (adminFocusField + 1) % 5;
            
            if (mouseClick) {
                int yPos[] = {135, 210, 285, 360, 435};
                for (int i = 0; i < 5; i++) {
<<<<<<< Updated upstream
                    if (CheckCollisionPointRec(mousePos, {(float)(cx - 250), (float)yPos[i], 500, 35})) {
=======
                    if (CheckCollisionPointRec(mousePos, {150, (float)yPos[i], 500, 35})) {
>>>>>>> Stashed changes
                        adminFocusField = i;
                        break;
                    }
                }
                
<<<<<<< Updated upstream
                if (CheckCollisionPointRec(mousePos, {(float)(cx - 90), 520, 130, 45})) {
                    if (adminId.empty() || adminTipo.empty() || adminNome.empty() || adminPreco.empty()) {
                        adminMessage = "Preencha todos os campos obrigatorios!";
                    } else {
=======
                // Botão SALVAR
                if (CheckCollisionPointRec(mousePos, {250, 520, 130, 45})) {
                    if (adminId.empty() || adminTipo.empty() || adminNome.empty() || adminPreco.empty()) {
                        adminMessage = "Preencha todos os campos obrigatorios!";
                    } else {
                        // Verificar se ID já existe
>>>>>>> Stashed changes
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
                            try {
                                Component newComp;
                                newComp.id = adminId;
                                newComp.tipo = adminTipo;
                                newComp.nome = adminNome;
                                newComp.specs = adminSpecs.empty() ? "N/A" : adminSpecs;
                                newComp.preco = stod(adminPreco);
                                catalog.push_back(newComp);
                                saveComponents();
                                adminMessage = "Componente adicionado com sucesso!";
                                feedbackTimer = 180;
<<<<<<< Updated upstream
=======
                                cout << "Novo componente: " << newComp.id << " | " << newComp.nome << endl;
>>>>>>> Stashed changes
                            } catch (...) {
                                adminMessage = "Erro no formato do preco (use ponto)!";
                            }
                        }
                    }
                }
<<<<<<< Updated upstream
                else if (CheckCollisionPointRec(mousePos, {(float)(cx + 60), 520, 130, 45})) {
=======
                // Botão CANCELAR
                else if (CheckCollisionPointRec(mousePos, {420, 520, 130, 45})) {
>>>>>>> Stashed changes
                    currentScreen = ADMIN_PANEL;
                }
            }
        }
        
        // ADMIN EDIT SCREEN
        else if (currentScreen == ADMIN_EDIT) {
            if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
                if (adminFocusField == 0) handleTextInput(adminId, 20);
                else if (adminFocusField == 1) handleTextInput(adminTipo, 30);
                else if (adminFocusField == 2) handleTextInput(adminNome, 50);
                else if (adminFocusField == 3) handleTextInput(adminSpecs, 100);
                else if (adminFocusField == 4) handleTextInput(adminPreco, 20);
                
                if (IsKeyPressed(KEY_TAB)) adminFocusField = (adminFocusField + 1) % 5;
                
                if (mouseClick) {
                    int yPos[] = {135, 210, 285, 360, 435};
                    for (int i = 0; i < 5; i++) {
<<<<<<< Updated upstream
                        if (CheckCollisionPointRec(mousePos, {(float)(cx - 250), (float)yPos[i], 500, 35})) {
=======
                        if (CheckCollisionPointRec(mousePos, {150, (float)yPos[i], 500, 35})) {
>>>>>>> Stashed changes
                            adminFocusField = i;
                            break;
                        }
                    }
                    
<<<<<<< Updated upstream
                    if (CheckCollisionPointRec(mousePos, {(float)(cx - 90), 520, 130, 45})) {
=======
                    // Botão SALVAR
                    if (CheckCollisionPointRec(mousePos, {250, 520, 130, 45})) {
>>>>>>> Stashed changes
                        if (adminId.empty() || adminTipo.empty() || adminNome.empty() || adminPreco.empty()) {
                            adminMessage = "Preencha todos os campos obrigatorios!";
                        } else {
                            try {
                                catalog[adminSelectedIndex].id = adminId;
                                catalog[adminSelectedIndex].tipo = adminTipo;
                                catalog[adminSelectedIndex].nome = adminNome;
                                catalog[adminSelectedIndex].specs = adminSpecs.empty() ? "N/A" : adminSpecs;
                                catalog[adminSelectedIndex].preco = stod(adminPreco);
                                saveComponents();
                                adminMessage = "Componente editado com sucesso!";
                                feedbackTimer = 180;
<<<<<<< Updated upstream
=======
                                cout << "Componente editado: " << adminId << endl;
>>>>>>> Stashed changes
                            } catch (...) {
                                adminMessage = "Erro no formato do preco (use ponto)!";
                            }
                        }
                    }
<<<<<<< Updated upstream
                    else if (CheckCollisionPointRec(mousePos, {(float)(cx + 60), 520, 130, 45})) {
=======
                    // Botão CANCELAR
                    else if (CheckCollisionPointRec(mousePos, {420, 520, 130, 45})) {
>>>>>>> Stashed changes
                        currentScreen = ADMIN_PANEL;
                    }
                }
            } else {
<<<<<<< Updated upstream
                if (mouseClick && CheckCollisionPointRec(mousePos, {(float)(cx - 100), 400, 200, 50})) {
=======
                if (mouseClick && CheckCollisionPointRec(mousePos, {300, 400, 200, 50})) {
>>>>>>> Stashed changes
                    currentScreen = ADMIN_PANEL;
                }
            }
        }
        
        // ADMIN DELETE SCREEN
        else if (currentScreen == ADMIN_DELETE) {
            if (mouseClick) {
                if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
<<<<<<< Updated upstream
                    if (CheckCollisionPointRec(mousePos, {(float)(cx - 90), 450, 130, 45})) {
=======
                    // Botão EXCLUIR
                    if (CheckCollisionPointRec(mousePos, {250, 450, 130, 45})) {
                        string deletedName = catalog[adminSelectedIndex].nome;
>>>>>>> Stashed changes
                        catalog.erase(catalog.begin() + adminSelectedIndex);
                        saveComponents();
                        adminMessage = "Componente excluido com sucesso!";
                        adminSelectedIndex = -1;
                        feedbackTimer = 180;
<<<<<<< Updated upstream
                        currentScreen = ADMIN_PANEL;
                    }
                    else if (CheckCollisionPointRec(mousePos, {(float)(cx + 60), 450, 130, 45})) {
                        currentScreen = ADMIN_PANEL;
                    }
                } else {
                    if (CheckCollisionPointRec(mousePos, {(float)(cx - 100), 400, 200, 50})) {
=======
                        cout << "Componente excluido: " << deletedName << endl;
                        currentScreen = ADMIN_PANEL;
                    }
                    // Botão CANCELAR
                    else if (CheckCollisionPointRec(mousePos, {420, 450, 130, 45})) {
                        currentScreen = ADMIN_PANEL;
                    }
                } else {
                    if (CheckCollisionPointRec(mousePos, {300, 400, 200, 50})) {
>>>>>>> Stashed changes
                        currentScreen = ADMIN_PANEL;
                    }
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
        
        // Feedback message (global)
        if (feedbackTimer > 0 && !feedbackMessage.empty()) {
<<<<<<< Updated upstream
            int msgWidth = MeasureText(feedbackMessage.c_str(), 18);
            DrawRectangle(cx - msgWidth/2 - 10, 10, msgWidth + 20, 40, Fade(BLACK, 0.7f));
            DrawText(feedbackMessage.c_str(), cx - msgWidth/2, 20, 18, WHITE);
=======
            DrawRectangle(200, 10, 400, 40, Fade(BLACK, 0.7f));
            DrawText(feedbackMessage.c_str(), 210, 20, 18, WHITE);
>>>>>>> Stashed changes
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}