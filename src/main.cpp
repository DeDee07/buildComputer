#include "raylib.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cmath>

using namespace std;

// ==================== ESTRUTURAS ====================

struct Component {
    string id, tipo, nome, specs;
    double preco;
};

struct Account {
    string username, password, email, phone;
    bool isAdmin;
};

struct Card {
    Rectangle bounds;
    string title;
    string description;
    Color iconColor;
    bool isHovered;
    int targetScreen;
    
    Card(Rectangle b, const string& t, const string& d, Color c, bool h, int s)
        : bounds(b), title(t), description(d), iconColor(c), isHovered(h), targetScreen(s) {}
};

struct PCSlot {
    Rectangle bounds;
    string tipo;
    string label;
    bool filled;
    Component component;
    
    PCSlot(Rectangle b, const string& t, const string& l)
        : bounds(b), tipo(t), label(l), filled(false) {}
};

// ==================== ENUMERAÇÕES ====================

enum Screen { 
    LOGIN, REGISTER, MENU, ESCOLHER_PECAS, VER_PRECOS, 
    COMO_MONTAR, ADMIN_PANEL, ADMIN_ADD, ADMIN_EDIT, ADMIN_DELETE 
};

// ==================== VARIÁVEIS GLOBAIS ====================

vector<Component> catalog, buildAtual;
vector<Account> accounts;
vector<PCSlot> pcSlots;
string currentUser = "";
bool isCurrentUserAdmin = false;

Screen currentScreen = LOGIN;

string loginUser = "", loginPass = "";
bool loginFocusUser = true;
string registerUser = "", registerPass = "", registerPassConfirm = "", registerEmail = "", registerPhone = "", registerMessage = "";
int registerFocusField = 0;

string adminId = "", adminTipo = "", adminNome = "", adminSpecs = "", adminPreco = "", adminMessage = "";
int adminFocusField = 0, adminSelectedIndex = -1, adminScrollOffset = 0;

string filtroCategoria = "Todos", feedbackMessage = "";
int pecasScrollOffset = 0, feedbackTimer = 0;
Font arialFont;

Component* draggedComponent = nullptr;
Vector2 dragOffset = {0, 0};

// ==================== DECLARAÇÕES DE FUNÇÕES ====================
void drawLoginScreen();
void drawRegisterScreen();
void drawMenuScreen();
void drawEscolherPecasScreen();
void drawVerPrecosScreen();
void drawComoMontarScreen();
void drawAdminPanelScreen();
void drawAdminAddScreen();
void drawAdminEditScreen();
void drawAdminDeleteScreen();

void DrawNavBar();
void DrawWolfLogo(float x, float y, float size);
bool DrawButton(Rectangle bounds, const char* text, Color normalColor, Color hoverColor, Color textColor);
void DrawCard(Card& card, const char* icon);
void drawTextBox(const char* label, string &text, Rectangle box, bool focused, bool maskPassword = false);

void saveAccounts();
void loadAccounts();
void saveComponents();
bool loadComponents(const string &path);
string formatBR(double v);
double calcularTotal();
void initPCSlots();
bool validateEmail(const string &email);
bool validatePhone(const string &phone);

void handleTextInput(string &target, int maxLength);
void handleLoginInput();
void handleRegisterInput();
void handleAdminInput();

// ==================== FUNÇÕES AUXILIARES ====================

static inline string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

int getCenterX() { return GetScreenWidth() / 2; }
int getCenterY() { return GetScreenHeight() / 2; }

void DrawWolfLogo(float x, float y, float size) {
    static Texture2D logo = { 0 };
    static bool logoLoaded = false;
    if (!logoLoaded) {
        logo = LoadTexture("resourcer/logo.png");
        logoLoaded = true;
    }
 
    if (logo.id != 0) {
        Rectangle src = { 0.0f, 0.0f, (float)logo.width, (float)logo.height };
        Rectangle dest = { x, y, size, size };
        Vector2 origin = { 0.0f, 0.0f };
        DrawTexturePro(logo, src, dest, origin, 0.0f, WHITE);
        return;
    }
 
    float cx = x + size * 0.5f;
    float cy = y + size * 0.5f;
    DrawCircle(cx, cy, size * 0.45f, DARKGRAY);
    DrawCircleLines(cx, cy, size * 0.45f, WHITE);
 
    constexpr float PI_VAL = 3.14159265358979323846f;
    Vector2 pts[12];
    float r = size * 0.45f;
    for (int i = 0; i < 12; i++) {
        float angle = (2.0f * PI_VAL * i) / 12.0f;
        pts[i] = { cx + cosf(angle) * r, cy + sinf(angle) * r };
    }
 
    for (int i = 0; i < 12; i++) {
        DrawLineEx(pts[i], pts[(i + 1) % 12], 2.0f, WHITE);
    }
 
    DrawCircle(cx - size * 0.08f, cy - size * 0.05f, size * 0.03f, RED);
    DrawCircle(cx + size * 0.08f, cy - size * 0.05f, size * 0.03f, RED);
 
    DrawLineEx(Vector2{ cx - size * 0.05f, cy + size * 0.05f },
               Vector2{ cx, cy + size * 0.08f }, 2.0f, WHITE);
    DrawLineEx(Vector2{ cx, cy + size * 0.08f },
               Vector2{ cx + size * 0.05f, cy + size * 0.05f }, 2.0f, WHITE);
}

// ==================== DATABASE ====================

void saveAccounts() {
    ofstream f("accounts.db");
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

void initPCSlots() {
    pcSlots.clear();
    int startX = 100;
    int startY = 200;
    
    pcSlots.push_back(PCSlot({(float)startX, (float)startY, 180, 400}, "Case", "Gabinete"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 20), 140, 50}, "Motherboard", "Placa-Mae"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 80), 140, 40}, "CPU", "Processador"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 130), 140, 60}, "GPU", "Placa Video"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 200), 140, 40}, "RAM", "Memoria RAM"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 250), 140, 40}, "Storage", "Armazenamento"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 300), 140, 40}, "PSU", "Fonte"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 350), 140, 40}, "Cooler", "Cooler"));
}

// ==================== UI DRAWING ====================

void DrawNavBar() {
    int sw = GetScreenWidth();
    DrawRectangle(0, 0, sw, 80, BLACK);
    DrawLine(0, 80, sw, 80, GRAY);
    DrawWolfLogo(20, 15, 50);
    DrawTextEx(arialFont, "Build", Vector2{85, 20}, 28, 1, RED);
    DrawTextEx(arialFont, "Computer", Vector2{175, 20}, 28, 1, WHITE);
    DrawTextEx(arialFont, "Monte o seu PC passo a passo!", Vector2{85, 50}, 12, 1, GRAY);
}

bool DrawButton(Rectangle bounds, const char* text, Color normalColor, Color hoverColor, Color textColor) {
    Vector2 mousePos = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mousePos, bounds);
    bool isClicked = isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
 
    Color currentColor = isHovered ? hoverColor : normalColor;
    DrawRectangleRounded(bounds, 0.2f, 20, currentColor);
 
    if (isHovered) {
        DrawRectangleRoundedLines(bounds, 0.2f, 20, RED);
    }
 
    Vector2 textSize = MeasureTextEx(arialFont, text, 20, 1);
    DrawTextEx(arialFont, text,
        Vector2{bounds.x + (bounds.width - textSize.x) / 2, bounds.y + (bounds.height - textSize.y) / 2},
        20, 1, textColor);
 
    return isClicked;
}

void DrawCard(Card& card, const char* icon) {
    Vector2 mousePos = GetMousePosition();
    card.isHovered = CheckCollisionPointRec(mousePos, card.bounds);
 
    if (card.isHovered) {
        DrawRectangleRounded(
            {card.bounds.x + 5, card.bounds.y + 5, card.bounds.width, card.bounds.height},
            0.1f, 20, Fade(RED, 0.2f)
        );
    }
 
    DrawRectangleRounded(card.bounds, 0.1f, 20, {30, 30, 30, 200});
    Color borderColor = card.isHovered ? RED : GRAY;
    DrawRectangleRoundedLines(card.bounds, 0.1f, 20, borderColor);
 
    float centerX = card.bounds.x + card.bounds.width / 2;
    float iconY = card.bounds.y + 60;
    DrawCircle(centerX, iconY, 40, Fade(RED, 0.2f));
 
    Vector2 iconSize = MeasureTextEx(arialFont, icon, 40, 1);
    DrawTextEx(arialFont, icon, Vector2{centerX - iconSize.x / 2, iconY - 20}, 40, 1, RED);
 
    Vector2 titleSize = MeasureTextEx(arialFont, card.title.c_str(), 24, 1);
    DrawTextEx(arialFont, card.title.c_str(), Vector2{centerX - titleSize.x / 2, iconY + 50}, 24, 1, WHITE);
 
    Vector2 descSize = MeasureTextEx(arialFont, card.description.c_str(), 16, 1);
    DrawTextEx(arialFont, card.description.c_str(), Vector2{centerX - descSize.x / 2, iconY + 85}, 16, 1, GRAY);
 
    Rectangle btnBounds = {
        card.bounds.x + 20,
        card.bounds.y + card.bounds.height - 60,
        card.bounds.width - 40,
        40
    };
 
    DrawRectangleRounded(btnBounds, 0.2f, 20, Fade(BLACK, 0.5f));
    DrawRectangleRoundedLines(btnBounds, 0.2f, 20, RED);
 
    Vector2 btnTextSize = MeasureTextEx(arialFont, "Acessar", 18, 1);
    DrawTextEx(arialFont, "Acessar",
        Vector2{btnBounds.x + (btnBounds.width - btnTextSize.x) / 2,
                btnBounds.y + (btnBounds.height - btnTextSize.y) / 2},
        18, 1, WHITE);
}

void drawTextBox(const char* label, string &text, Rectangle box, bool focused, bool maskPassword) {
    DrawTextEx(arialFont, label, Vector2{box.x, box.y - 25}, 18, 1, WHITE);
    DrawRectangleRounded(box, 0.1f, 20, focused ? Fade(RED, 0.2f) : Fade(WHITE, 0.1f));
    DrawRectangleRoundedLines(box, 0.1f, 20, focused ? RED : GRAY);
    string display = maskPassword ? string(text.length(), '*') : text;
    DrawTextEx(arialFont, display.c_str(), Vector2{box.x + 10, box.y + 10}, 18, 1, WHITE);
}

// ==================== TELAS ====================

void drawLoginScreen() {
    int cx = getCenterX();
    int cy = getCenterY();
    
    DrawWolfLogo(cx - 75, cy - 200, 150);
    
    Vector2 buildSize = MeasureTextEx(arialFont, "Build", 50, 1);
    Vector2 computerSize = MeasureTextEx(arialFont, "Computer", 50, 1);
    float totalWidth = buildSize.x + computerSize.x;
    float startX = (GetScreenWidth() - totalWidth) / 2;
    
    DrawTextEx(arialFont, "Build", Vector2{startX, (float)(cy - 30)}, 50, 1, RED);
    DrawTextEx(arialFont, "Computer", Vector2{startX + buildSize.x, (float)(cy - 30)}, 50, 1, WHITE);
    
    const char* subtitle = "Sistema de Login";
    Vector2 subtitleSize = MeasureTextEx(arialFont, subtitle, 20, 1);
    DrawTextEx(arialFont, subtitle, Vector2{(GetScreenWidth() - subtitleSize.x) / 2, (float)(cy + 30)}, 20, 1, GRAY);
    
    drawTextBox("Usuario:", loginUser, {(float)(cx - 200), (float)(cy + 80), 400, 40}, loginFocusUser);
    drawTextBox("Senha:", loginPass, {(float)(cx - 200), (float)(cy + 160), 400, 40}, !loginFocusUser, true);
    
    bool loginClicked = DrawButton({(float)(cx - 210), (float)(cy + 240), 180, 50}, "ENTRAR", {40, 40, 40, 255}, {60, 60, 60, 255}, WHITE);
    bool registerClicked = DrawButton({(float)(cx + 30), (float)(cy + 240), 180, 50}, "CRIAR CONTA", {0, 100, 0, 255}, {0, 140, 0, 255}, WHITE);
    
    if (loginClicked) {
        for (auto &acc : accounts) {
            if (acc.username == loginUser && acc.password == loginPass) {
                currentUser = acc.username;
                isCurrentUserAdmin = acc.isAdmin;
                currentScreen = MENU;
                loginUser = loginPass = "";
                break;
            }
        }
    }
    
    if (registerClicked) {
        currentScreen = REGISTER;
        registerUser = registerPass = registerPassConfirm = registerEmail = registerPhone = registerMessage = "";
        registerFocusField = 0;
    }
}

void drawRegisterScreen() {
    int cx = getCenterX();
    int cy = getCenterY();
    
    DrawWolfLogo(cx - 50, 50, 100);
    
    Vector2 titleSize = MeasureTextEx(arialFont, "=== Criar Nova Conta ===", 28, 1);
    DrawTextEx(arialFont, "=== Criar Nova Conta ===", Vector2{(GetScreenWidth() - titleSize.x)/2, 170}, 28, 1, RED);
    
    drawTextBox("Usuario:", registerUser, {(float)(cx - 250), (float)(cy - 120), 500, 38}, registerFocusField == 0);
    drawTextBox("Email:", registerEmail, {(float)(cx - 250), (float)(cy - 60), 500, 38}, registerFocusField == 1);
    drawTextBox("Telemovel:", registerPhone, {(float)(cx - 250), (float)cy, 500, 38}, registerFocusField == 2);
    drawTextBox("Senha:", registerPass, {(float)(cx - 250), (float)(cy + 60), 500, 38}, registerFocusField == 3, true);
    drawTextBox("Confirmar Senha:", registerPassConfirm, {(float)(cx - 250), (float)(cy + 120), 500, 38}, registerFocusField == 4, true);
    
    if (!registerMessage.empty()) {
        Color msgColor = (registerMessage.find("sucesso") != string::npos) ? GREEN : RED;
        Vector2 msgSize = MeasureTextEx(arialFont, registerMessage.c_str(), 16, 1);
        DrawTextEx(arialFont, registerMessage.c_str(), Vector2{(GetScreenWidth() - msgSize.x)/2, (float)(cy + 180)}, 16, 1, msgColor);
    }
    
    bool createClicked = DrawButton({(float)(cx - 150), (float)(cy + 220), 130, 45}, "CRIAR", {0, 100, 0, 255}, {0, 140, 0, 255}, WHITE);
    bool backClicked = DrawButton({(float)(cx + 20), (float)(cy + 220), 130, 45}, "VOLTAR", {60, 60, 60, 255}, {80, 80, 80, 255}, WHITE);
    
    if (createClicked) {
        if (registerUser.empty() || registerEmail.empty() || registerPhone.empty() || registerPass.empty()) {
            registerMessage = "Preencha todos os campos!";
        } else if (!validateEmail(registerEmail)) {
            registerMessage = "Email invalido!";
        } else if (!validatePhone(registerPhone)) {
            registerMessage = "Telemovel invalido!";
        } else if (registerPass != registerPassConfirm) {
            registerMessage = "As senhas nao coincidem!";
        } else {
            bool userExists = false;
            for (auto &acc : accounts) {
                if (acc.username == registerUser) {
                    userExists = true;
                    break;
                }
            }
            
            if (userExists) {
                registerMessage = "Usuario ja existe!";
            } else {
                Account newAcc;
                newAcc.username = registerUser;
                newAcc.password = registerPass;
                newAcc.email = registerEmail;
                newAcc.phone = registerPhone;
                newAcc.isAdmin = false;
                accounts.push_back(newAcc);
                saveAccounts();
                registerMessage = "Conta criada com sucesso!";
            }
        }
    }
    
    if (backClicked) {
        currentScreen = LOGIN;
    }
}

void drawMenuScreen() {
    int cx = getCenterX();
    int sh = GetScreenHeight();
    
    DrawWolfLogo(cx - 75, 120, 150);
    
    Vector2 buildSize = MeasureTextEx(arialFont, "Build", 50, 1);
    Vector2 computerSize = MeasureTextEx(arialFont, "Computer", 50, 1);
    float totalWidth = buildSize.x + computerSize.x;
    float startX = (GetScreenWidth() - totalWidth) / 2;
    
    DrawTextEx(arialFont, "Build", Vector2{startX, 300}, 50, 1, RED);
    DrawTextEx(arialFont, "Computer", Vector2{startX + buildSize.x, 300}, 50, 1, WHITE);
    
    string welcome = "Bem-vindo, " + currentUser + (isCurrentUserAdmin ? " (Admin)" : "");
    Vector2 welcomeSize = MeasureTextEx(arialFont, welcome.c_str(), 20, 1);
    DrawTextEx(arialFont, welcome.c_str(), Vector2{(GetScreenWidth() - welcomeSize.x) / 2, 360}, 20, 1, GRAY);
    
    vector<Card> cards;
    if (isCurrentUserAdmin) {
        cards.push_back(Card(Rectangle{0, 0, 280, 300}, "Como montar", "Guia passo a passo", RED, false, COMO_MONTAR));
        cards.push_back(Card(Rectangle{0, 0, 280, 300}, "Escolher pecas", "Catalogo completo", RED, false, ESCOLHER_PECAS));
        cards.push_back(Card(Rectangle{0, 0, 280, 300}, "Ver precos", "Calcule o custo", RED, false, VER_PRECOS));
        cards.push_back(Card(Rectangle{0, 0, 280, 300}, "Admin Panel", "Gerir componentes", RED, false, ADMIN_PANEL));
    } else {
        cards.push_back(Card(Rectangle{0, 0, 280, 300}, "Como montar", "Guia passo a passo", RED, false, COMO_MONTAR));
        cards.push_back(Card(Rectangle{0, 0, 280, 300}, "Escolher pecas", "Catalogo completo", RED, false, ESCOLHER_PECAS));
        cards.push_back(Card(Rectangle{0, 0, 280, 300}, "Ver precos", "Calcule o custo", RED, false, VER_PRECOS));
    }
    
    float marginBottom = 100.0f;
    float targetY = sh - cards[0].bounds.height - marginBottom;
    for (auto &c : cards) c.bounds.y = targetY;
    
    float gap = 40.0f;
    float totalCardsWidth = 0.0f;
    for (const auto &c : cards) totalCardsWidth += c.bounds.width;
    totalCardsWidth += gap * (cards.size() - 1);
    
    float startCardX = (GetScreenWidth() - totalCardsWidth) / 2.0f;
    float x = startCardX;
    for (auto &c : cards) {
        c.bounds.x = x;
        x += c.bounds.width + gap;
    }
    
    const char* icons[] = {"G", "P", "$", "A"};
    for (size_t i = 0; i < cards.size(); i++) {
        DrawCard(cards[i], icons[i]);
        
        Vector2 mousePos = GetMousePosition();
        Rectangle btnBounds = {
            cards[i].bounds.x + 20,
            cards[i].bounds.y + cards[i].bounds.height - 60,
            cards[i].bounds.width - 40,
            40
        };
        
        if (CheckCollisionPointRec(mousePos, btnBounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            currentScreen = (Screen)cards[i].targetScreen;
            pecasScrollOffset = 0;
            adminScrollOffset = 0;
            adminSelectedIndex = -1;
            if (cards[i].targetScreen == COMO_MONTAR) {
                initPCSlots();
            }
        }
    }
    
    Rectangle logoutBtn = {(float)(GetScreenWidth() - 150), 20, 130, 40};
    if (DrawButton(logoutBtn, "SAIR", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
        currentUser = "";
        isCurrentUserAdmin = false;
        currentScreen = LOGIN;
        buildAtual.clear();
    }
}

void drawEscolherPecasScreen() {
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 60, 60, 255}, WHITE)) {
        currentScreen = MENU;
    }
    
    DrawTextEx(arialFont, "ESCOLHER PECAS", Vector2{50, 120}, 32, 1, RED);
    
    vector<string> categorias = {"Todos", "CPU", "GPU", "RAM", "Motherboard", "PSU", "Case", "Cooler", "SSD", "HDD"};
    int filterY = 170;
    int filterX = 50;
    
    for (const auto &cat : categorias) {
        Rectangle btn = {(float)filterX, (float)filterY, 120, 35};
        Color btnColor = (filtroCategoria == cat) ? Fade(RED, 0.5f) : Fade(WHITE, 0.1f);
        
        DrawRectangleRounded(btn, 0.2f, 20, btnColor);
        DrawRectangleRoundedLines(btn, 0.2f, 20, (filtroCategoria == cat) ? RED : GRAY);
        
        Vector2 textSize = MeasureTextEx(arialFont, cat.c_str(), 16, 1);
        DrawTextEx(arialFont, cat.c_str(), 
                  Vector2{btn.x + (btn.width - textSize.x) / 2, btn.y + (btn.height - textSize.y) / 2}, 
                  16, 1, WHITE);
        
        Vector2 mousePos = GetMousePosition();
        if (CheckCollisionPointRec(mousePos, btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            filtroCategoria = cat;
            pecasScrollOffset = 0;
        }
        
        filterX += 130;
        if (filterX > GetScreenWidth() - 130) {
            filterX = 50;
            filterY += 45;
        }
    }
    
    int listY = 270 - pecasScrollOffset;
    
    for (auto &c : catalog) {
        if (filtroCategoria != "Todos" && c.tipo != filtroCategoria) continue;
        
        if (listY > 240 && listY < GetScreenHeight() - 100) {
            Rectangle compRect = {50, (float)listY, (float)(GetScreenWidth() - 100), 80};
            
            Vector2 mousePos = GetMousePosition();
            bool isHovered = CheckCollisionPointRec(mousePos, compRect);
            
            DrawRectangleRounded(compRect, 0.1f, 20, isHovered ? Fade(RED, 0.3f) : Fade(WHITE, 0.1f));
            DrawRectangleRoundedLines(compRect, 0.1f, 20, isHovered ? RED : GRAY);
            
            DrawTextEx(arialFont, TextFormat("[%s]", c.tipo.c_str()), Vector2{60, (float)(listY + 10)}, 16, 1, RED);
            DrawTextEx(arialFont, c.nome.c_str(), Vector2{60, (float)(listY + 35)}, 18, 1, WHITE);
            DrawTextEx(arialFont, c.specs.c_str(), Vector2{60, (float)(listY + 58)}, 14, 1, GRAY);
            
            string precoStr = formatBR(c.preco);
            DrawTextEx(arialFont, precoStr.c_str(), 
                      Vector2{compRect.x + compRect.width - 280, (float)(listY + 25)}, 20, 1, GREEN);
            
            bool jaAdicionado = false;
            for (const auto &b : buildAtual) {
                if (b.id == c.id) {
                    jaAdicionado = true;
                    break;
                }
            }
            
            Rectangle addBtn = {compRect.x + compRect.width - 130, compRect.y + 15, 110, 50};
            const char* btnText = jaAdicionado ? "Remover" : "Adicionar";
            Color btnColor = jaAdicionado ? Fade(RED, 0.5f) : Fade(GREEN, 0.5f);
            Color btnHover = jaAdicionado ? Fade(RED, 0.7f) : Fade(GREEN, 0.7f);
            
            bool btnHovered = CheckCollisionPointRec(mousePos, addBtn);
            DrawRectangleRounded(addBtn, 0.2f, 20, btnHovered ? btnHover : btnColor);
            DrawRectangleRoundedLines(addBtn, 0.2f, 20, WHITE);
            
            Vector2 btnTextSize = MeasureTextEx(arialFont, btnText, 16, 1);
            DrawTextEx(arialFont, btnText, 
                      Vector2{addBtn.x + (addBtn.width - btnTextSize.x) / 2, 
                             addBtn.y + (addBtn.height - btnTextSize.y) / 2}, 16, 1, WHITE);
            
            if (btnHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (jaAdicionado) {
                    buildAtual.erase(
                        remove_if(buildAtual.begin(), buildAtual.end(), 
                                 [&c](const Component &b) { return b.id == c.id; }), 
                        buildAtual.end()
                    );
                    feedbackMessage = "Removido da build!";
                } else {
                    buildAtual.push_back(c);
                    feedbackMessage = "Adicionado a build!";
                }
                feedbackTimer = 120;
            }
        }
        listY += 90;
    }
    
    float scrollWheel = GetMouseWheelMove();
    if (scrollWheel != 0) {
        pecasScrollOffset -= (int)(scrollWheel * 40);
        if (pecasScrollOffset < 0) pecasScrollOffset = 0;
    }
    
    DrawRectangle(0, GetScreenHeight() - 60, GetScreenWidth(), 60, {20, 20, 20, 255});
    DrawLine(0, GetScreenHeight() - 60, GetScreenWidth(), GetScreenHeight() - 60, RED);
    
    DrawTextEx(arialFont, TextFormat("Componentes na build: %d", (int)buildAtual.size()), 
               Vector2{20, (float)(GetScreenHeight() - 45)}, 18, 1, WHITE);
    DrawTextEx(arialFont, TextFormat("Total: %s", formatBR(calcularTotal()).c_str()), 
               Vector2{20, (float)(GetScreenHeight() - 20)}, 18, 1, GREEN);
    
    if (feedbackTimer > 0) {
        Color msgColor = (feedbackMessage.find("Adicionado") != string::npos || 
                         feedbackMessage.find("Removido") != string::npos) ? GREEN : RED;
        Vector2 msgSize = MeasureTextEx(arialFont, feedbackMessage.c_str(), 20, 1);
        DrawTextEx(arialFont, feedbackMessage.c_str(), 
                  Vector2{(GetScreenWidth() - msgSize.x) / 2, 150}, 20, 1, msgColor);
        feedbackTimer--;
    }
}

void drawVerPrecosScreen() {
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 60, 60, 255}, WHITE)) {
        currentScreen = MENU;
    }
    
    DrawTextEx(arialFont, "VER PRECOS DA BUILD", Vector2{50, 120}, 32, 1, RED);
    
    if (buildAtual.empty()) {
        DrawTextEx(arialFont, "Nenhum componente selecionado ainda!", 
                  Vector2{50, 200}, 24, 1, GRAY);
        DrawTextEx(arialFont, "Va para 'Escolher Pecas' e adicione componentes.", 
                  Vector2{50, 240}, 18, 1, GRAY);
        return;
    }
    
    int listY = 180;
    
    DrawRectangle(40, listY - 10, GetScreenWidth() - 80, 40, {30, 30, 30, 255});
    DrawTextEx(arialFont, "Componente", Vector2{50, (float)listY}, 18, 1, RED);
    DrawTextEx(arialFont, "Especificacoes", Vector2{400, (float)listY}, 18, 1, RED);
    DrawTextEx(arialFont, "Preco", Vector2{(float)(GetScreenWidth() - 200), (float)listY}, 18, 1, RED);
    
    listY += 50;
    
    for (const auto &c : buildAtual) {
        DrawRectangleRounded({40, (float)listY, (float)(GetScreenWidth() - 80), 60}, 0.1f, 20, Fade(WHITE, 0.1f));
        DrawRectangleRoundedLines({40, (float)listY, (float)(GetScreenWidth() - 80), 60}, 0.1f, 20, GRAY);
        
        DrawTextEx(arialFont, TextFormat("[%s]", c.tipo.c_str()), Vector2{50, (float)(listY + 10)}, 14, 1, RED);
        DrawTextEx(arialFont, c.nome.c_str(), Vector2{50, (float)(listY + 30)}, 16, 1, WHITE);
        DrawTextEx(arialFont, c.specs.c_str(), Vector2{400, (float)(listY + 20)}, 14, 1, GRAY);
        
        string precoStr = formatBR(c.preco);
        DrawTextEx(arialFont, precoStr.c_str(), 
                  Vector2{(float)(GetScreenWidth() - 200), (float)(listY + 20)}, 18, 1, GREEN);
        
        listY += 70;
    }
    
    DrawRectangle(0, GetScreenHeight() - 100, GetScreenWidth(), 100, {20, 20, 20, 255});
    DrawLine(0, GetScreenHeight() - 100, GetScreenWidth(), GetScreenHeight() - 100, RED);
    
    DrawTextEx(arialFont, "TOTAL DA BUILD:", 
               Vector2{50, (float)(GetScreenHeight() - 70)}, 28, 1, WHITE);
    
    string totalStr = formatBR(calcularTotal());
    Vector2 totalSize = MeasureTextEx(arialFont, totalStr.c_str(), 36, 1);
    DrawTextEx(arialFont, totalStr.c_str(), 
               Vector2{GetScreenWidth() - totalSize.x - 50, (float)(GetScreenHeight() - 65)}, 36, 1, GREEN);
    
    Rectangle clearBtn = {50, (float)(GetScreenHeight() - 35), 180, 30};
    if (DrawButton(clearBtn, "Limpar Build", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
        buildAtual.clear();
        feedbackMessage = "Build limpa!";
        feedbackTimer = 120;
    }
}

void drawComoMontarScreen() {
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Sair", {40, 40, 40, 255}, {60, 60, 60, 255}, WHITE)) {
        currentScreen = MENU;
    }
    
    DrawTextEx(arialFont, "MONTAR PC - Arraste as pecas", Vector2{50, 120}, 32, 1, RED);
    
    DrawRectangleRounded({90, 190, 200, 420}, 0.05f, 20, {40, 40, 40, 255});
    DrawRectangleRoundedLines({90, 190, 200, 420}, 0.05f, 20, RED);
    
    for (auto &slot : pcSlots) {
        if (slot.tipo == "Case") continue;
        
        Color slotColor = slot.filled ? Fade(GREEN, 0.3f) : Fade(RED, 0.2f);
        DrawRectangleRounded(slot.bounds, 0.1f, 20, slotColor);
        DrawRectangleRoundedLines(slot.bounds, 0.1f, 20, slot.filled ? GREEN : GRAY);
        
        if (slot.filled) {
            Vector2 nameSize = MeasureTextEx(arialFont, slot.component.nome.c_str(), 10, 1);
            DrawTextEx(arialFont, slot.component.nome.c_str(), 
                      Vector2{slot.bounds.x + (slot.bounds.width - nameSize.x) / 2, 
                             slot.bounds.y + slot.bounds.height / 2 - 5}, 10, 1, WHITE);
        } else {
            Vector2 labelSize = MeasureTextEx(arialFont, slot.label.c_str(), 12, 1);
            DrawTextEx(arialFont, slot.label.c_str(), 
                      Vector2{slot.bounds.x + (slot.bounds.width - labelSize.x) / 2, 
                             slot.bounds.y + slot.bounds.height / 2 - 6}, 12, 1, GRAY);
        }
    }
    
    DrawTextEx(arialFont, "Componentes Disponiveis:", Vector2{350, 180}, 20, 1, WHITE);
    
    Vector2 mousePos = GetMousePosition();
    int listY = 220;
    
    for (size_t i = 0; i < buildAtual.size(); i++) {
        Component &c = buildAtual[i];
        Rectangle compRect = {350, (float)listY, 400, 50};
        
        bool isHovered = CheckCollisionPointRec(mousePos, compRect);
        Color bgColor = isHovered ? Fade(RED, 0.3f) : Fade(WHITE, 0.1f);
        
        DrawRectangleRounded(compRect, 0.1f, 20, bgColor);
        DrawRectangleRoundedLines(compRect, 0.1f, 20, isHovered ? RED : GRAY);
        
        DrawTextEx(arialFont, TextFormat("[%s]", c.tipo.c_str()), Vector2{360, (float)(listY + 5)}, 14, 1, RED);
        DrawTextEx(arialFont, c.nome.c_str(), Vector2{360, (float)(listY + 25)}, 14, 1, WHITE);
        
        if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            draggedComponent = &c;
            dragOffset = {mousePos.x - compRect.x, mousePos.y - compRect.y};
        }
        
        listY += 60;
    }
    
    if (draggedComponent != nullptr) {
        Rectangle dragRect = {mousePos.x - dragOffset.x, mousePos.y - dragOffset.y, 400, 50};
        DrawRectangleRounded(dragRect, 0.1f, 20, Fade(YELLOW, 0.5f));
        DrawRectangleRoundedLines(dragRect, 0.1f, 20, YELLOW);
        DrawTextEx(arialFont, draggedComponent->nome.c_str(), 
                  Vector2{dragRect.x + 10, dragRect.y + 15}, 14, 1, WHITE);
        
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            bool placed = false;
            for (auto &slot : pcSlots) {
                if (slot.tipo == "Case") continue;
                
                if (CheckCollisionPointRec(mousePos, slot.bounds)) {
                    if (slot.tipo == draggedComponent->tipo || 
                        (slot.tipo == "Storage" && (draggedComponent->tipo == "SSD" || draggedComponent->tipo == "HDD"))) {
                        slot.filled = true;
                        slot.component = *draggedComponent;
                        
                        for (size_t j = 0; j < buildAtual.size(); j++) {
                            if (&buildAtual[j] == draggedComponent) {
                                buildAtual.erase(buildAtual.begin() + j);
                                break;
                            }
                        }
                        
                        placed = true;
                        feedbackMessage = "Componente instalado!";
                        feedbackTimer = 120;
                        break;
                    }
                }
            }
            
            if (!placed) {
                feedbackMessage = "Coloque no slot correto!";
                feedbackTimer = 120;
            }
            
            draggedComponent = nullptr;
        }
    }
    
    Rectangle clearBtn = {800, (float)(GetScreenHeight() - 80), 150, 40};
    if (DrawButton(clearBtn, "Limpar Tudo", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
        for (auto &slot : pcSlots) {
            if (slot.filled) {
                buildAtual.push_back(slot.component);
                slot.filled = false;
            }
        }
        feedbackMessage = "Build limpa!";
        feedbackTimer = 120;
    }
    
    int totalSlots = 0, filledSlots = 0;
    for (auto &slot : pcSlots) {
        if (slot.tipo != "Case") {
            totalSlots++;
            if (slot.filled) filledSlots++;
        }
    }
    
    DrawTextEx(arialFont, TextFormat("Progresso: %d/%d componentes instalados", filledSlots, totalSlots),
               Vector2{350, (float)(GetScreenHeight() - 80)}, 18, 1, filledSlots == totalSlots ? GREEN : YELLOW);
    
    if (feedbackTimer > 0) {
        Color msgColor = (feedbackMessage.find("instalado") != string::npos || 
                         feedbackMessage.find("limpa") != string::npos) ? GREEN : RED;
        Vector2 msgSize = MeasureTextEx(arialFont, feedbackMessage.c_str(), 20, 1);
        DrawTextEx(arialFont, feedbackMessage.c_str(), 
                  Vector2{(GetScreenWidth() - msgSize.x) / 2, 150}, 20, 1, msgColor);
        feedbackTimer--;
    }
}

void drawAdminPanelScreen() {
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 60, 60, 255}, WHITE)) {
        currentScreen = MENU;
    }
    
    DrawTextEx(arialFont, "PAINEL DE ADMINISTRACAO", Vector2{50, 120}, 32, 1, RED);
    
    Rectangle addBtn = {50, 180, 200, 50};
    Rectangle editBtn = {270, 180, 200, 50};
    Rectangle deleteBtn = {490, 180, 200, 50};
    
    if (DrawButton(addBtn, "Adicionar", {0, 100, 0, 255}, {0, 140, 0, 255}, WHITE)) {
        currentScreen = ADMIN_ADD;
        adminId = adminTipo = adminNome = adminSpecs = adminPreco = adminMessage = "";
        adminFocusField = 0;
    }
    
    if (DrawButton(editBtn, "Editar", {0, 0, 100, 255}, {0, 0, 140, 255}, WHITE)) {
        currentScreen = ADMIN_EDIT;
        adminSelectedIndex = -1;
        adminMessage = "";
    }
    
    if (DrawButton(deleteBtn, "Eliminar", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
        currentScreen = ADMIN_DELETE;
        adminSelectedIndex = -1;
        adminMessage = "";
    }
    
    DrawTextEx(arialFont, TextFormat("Total de componentes: %d", (int)catalog.size()), 
               Vector2{50, 260}, 20, 1, WHITE);
    
    int listY = 300 - adminScrollOffset;
    
    for (size_t i = 0; i < catalog.size(); i++) {
        if (listY > 280 && listY < GetScreenHeight() - 50) {
            const auto &c = catalog[i];
            DrawRectangleRounded({50, (float)listY, (float)(GetScreenWidth() - 100), 70}, 0.1f, 20, Fade(WHITE, 0.1f));
            DrawRectangleRoundedLines({50, (float)listY, (float)(GetScreenWidth() - 100), 70}, 0.1f, 20, GRAY);
            
            DrawTextEx(arialFont, TextFormat("[%s] ID: %s", c.tipo.c_str(), c.id.c_str()), 
                      Vector2{60, (float)(listY + 10)}, 14, 1, RED);
            DrawTextEx(arialFont, c.nome.c_str(), Vector2{60, (float)(listY + 30)}, 16, 1, WHITE);
            DrawTextEx(arialFont, c.specs.c_str(), Vector2{60, (float)(listY + 50)}, 12, 1, GRAY);
            
            string precoStr = formatBR(c.preco);
            float precoWidth = MeasureTextEx(arialFont, precoStr.c_str(), 18, 1).x;
            DrawTextEx(arialFont, precoStr.c_str(), 
                      Vector2{GetScreenWidth() - precoWidth - 60, (float)(listY + 25)}, 18, 1, GREEN);
        }
        listY += 80;
    }
    
    float scrollWheel = GetMouseWheelMove();
    if (scrollWheel != 0) {
        adminScrollOffset -= (int)(scrollWheel * 40);
        if (adminScrollOffset < 0) adminScrollOffset = 0;
    }
}

void drawAdminAddScreen() {
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 60, 60, 255}, WHITE)) {
        currentScreen = ADMIN_PANEL;
    }
    
    DrawTextEx(arialFont, "ADICIONAR COMPONENTE", Vector2{50, 120}, 32, 1, RED);
    
    int cx = getCenterX();
    
    drawTextBox("ID:", adminId, {(float)(cx - 300), 200, 600, 40}, adminFocusField == 0);
    drawTextBox("Tipo:", adminTipo, {(float)(cx - 300), 270, 600, 40}, adminFocusField == 1);
    drawTextBox("Nome:", adminNome, {(float)(cx - 300), 340, 600, 40}, adminFocusField == 2);
    drawTextBox("Especificacoes:", adminSpecs, {(float)(cx - 300), 410, 600, 40}, adminFocusField == 3);
    drawTextBox("Preco (R$):", adminPreco, {(float)(cx - 300), 480, 600, 40}, adminFocusField == 4);
    
    if (!adminMessage.empty()) {
        Color msgColor = (adminMessage.find("sucesso") != string::npos) ? GREEN : RED;
        Vector2 msgSize = MeasureTextEx(arialFont, adminMessage.c_str(), 18, 1);
        DrawTextEx(arialFont, adminMessage.c_str(), 
                  Vector2{(GetScreenWidth() - msgSize.x) / 2, 550}, 18, 1, msgColor);
    }
    
    Rectangle saveBtn = {(float)(cx - 100), 590, 200, 50};
    if (DrawButton(saveBtn, "Guardar", {0, 100, 0, 255}, {0, 140, 0, 255}, WHITE)) {
        if (adminId.empty() || adminTipo.empty() || adminNome.empty() || adminPreco.empty()) {
            adminMessage = "Preencha todos os campos!";
        } else {
            try {
                Component novo;
                novo.id = adminId;
                novo.tipo = adminTipo;
                novo.nome = adminNome;
                novo.specs = adminSpecs;
                novo.preco = stod(adminPreco);
                
                catalog.push_back(novo);
                saveComponents();
                
                adminMessage = "Componente adicionado com sucesso!";
                adminId = adminTipo = adminNome = adminSpecs = adminPreco = "";
            } catch (...) {
                adminMessage = "Erro: Preco invalido!";
            }
        }
    }
}

void drawAdminEditScreen() {
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 60, 60, 255}, WHITE)) {
        currentScreen = ADMIN_PANEL;
    }
    
    DrawTextEx(arialFont, "EDITAR COMPONENTE", Vector2{50, 120}, 32, 1, RED);
    
    if (adminSelectedIndex == -1) {
        DrawTextEx(arialFont, "Selecione um componente para editar:", Vector2{50, 180}, 20, 1, WHITE);
        
        int listY = 220 - adminScrollOffset;
        Vector2 mousePos = GetMousePosition();
        
        for (size_t i = 0; i < catalog.size(); i++) {
            if (listY > 200 && listY < GetScreenHeight() - 50) {
                Rectangle compRect = {50, (float)listY, (float)(GetScreenWidth() - 100), 60};
                bool isHovered = CheckCollisionPointRec(mousePos, compRect);
                
                DrawRectangleRounded(compRect, 0.1f, 20, isHovered ? Fade(BLUE, 0.3f) : Fade(WHITE, 0.1f));
                DrawRectangleRoundedLines(compRect, 0.1f, 20, isHovered ? BLUE : GRAY);
                
                DrawTextEx(arialFont, catalog[i].nome.c_str(), Vector2{60, (float)(listY + 20)}, 16, 1, WHITE);
                
                if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    adminSelectedIndex = i;
                    adminId = catalog[i].id;
                    adminTipo = catalog[i].tipo;
                    adminNome = catalog[i].nome;
                    adminSpecs = catalog[i].specs;
                    adminPreco = to_string(catalog[i].preco);
                    adminFocusField = 0;
                }
            }
            listY += 70;
        }
        
        float scrollWheel = GetMouseWheelMove();
        if (scrollWheel != 0) {
            adminScrollOffset -= (int)(scrollWheel * 40);
            if (adminScrollOffset < 0) adminScrollOffset = 0;
        }
    } else {
        int cx = getCenterX();
        
        drawTextBox("ID:", adminId, {(float)(cx - 300), 200, 600, 40}, adminFocusField == 0);
        drawTextBox("Tipo:", adminTipo, {(float)(cx - 300), 270, 600, 40}, adminFocusField == 1);
        drawTextBox("Nome:", adminNome, {(float)(cx - 300), 340, 600, 40}, adminFocusField == 2);
        drawTextBox("Especificacoes:", adminSpecs, {(float)(cx - 300), 410, 600, 40}, adminFocusField == 3);
        drawTextBox("Preco (R$):", adminPreco, {(float)(cx - 300), 480, 600, 40}, adminFocusField == 4);
        
        if (!adminMessage.empty()) {
            Color msgColor = (adminMessage.find("sucesso") != string::npos) ? GREEN : RED;
            Vector2 msgSize = MeasureTextEx(arialFont, adminMessage.c_str(), 18, 1);
            DrawTextEx(arialFont, adminMessage.c_str(), 
                      Vector2{(GetScreenWidth() - msgSize.x) / 2, 550}, 18, 1, msgColor);
        }
        
        Rectangle saveBtn = {(float)(cx - 210), 590, 200, 50};
        Rectangle cancelBtn = {(float)(cx + 10), 590, 200, 50};
        
        if (DrawButton(saveBtn, "Guardar", {0, 100, 0, 255}, {0, 140, 0, 255}, WHITE)) {
            try {
                catalog[adminSelectedIndex].id = adminId;
                catalog[adminSelectedIndex].tipo = adminTipo;
                catalog[adminSelectedIndex].nome = adminNome;
                catalog[adminSelectedIndex].specs = adminSpecs;
                catalog[adminSelectedIndex].preco = stod(adminPreco);
                
                saveComponents();
                adminMessage = "Componente editado com sucesso!";
                adminSelectedIndex = -1;
            } catch (...) {
                adminMessage = "Erro: Preco invalido!";
            }
        }
        
        if (DrawButton(cancelBtn, "Cancelar", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
            adminSelectedIndex = -1;
        }
    }
}

void drawAdminDeleteScreen() {
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 60, 60, 255}, WHITE)) {
        currentScreen = ADMIN_PANEL;
    }
    
    DrawTextEx(arialFont, "ELIMINAR COMPONENTE", Vector2{50, 120}, 32, 1, RED);
    DrawTextEx(arialFont, "Selecione um componente para eliminar:", Vector2{50, 180}, 20, 1, WHITE);
    
    int listY = 220 - adminScrollOffset;
    Vector2 mousePos = GetMousePosition();
    
    for (size_t i = 0; i < catalog.size(); i++) {
        if (listY > 200 && listY < GetScreenHeight() - 50) {
            Rectangle compRect = {50, (float)listY, (float)(GetScreenWidth() - 180), 60};
            bool isHovered = CheckCollisionPointRec(mousePos, compRect);
            
            DrawRectangleRounded(compRect, 0.1f, 20, isHovered ? Fade(RED, 0.3f) : Fade(WHITE, 0.1f));
            DrawRectangleRoundedLines(compRect, 0.1f, 20, isHovered ? RED : GRAY);
            
            DrawTextEx(arialFont, catalog[i].nome.c_str(), Vector2{60, (float)(listY + 20)}, 16, 1, WHITE);
            
            Rectangle delBtn = {(float)(GetScreenWidth() - 150), (float)(listY + 10), 100, 40};
            if (DrawButton(delBtn, "Eliminar", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
                catalog.erase(catalog.begin() + i);
                saveComponents();
                adminMessage = "Componente eliminado!";
                break;
            }
        }
        listY += 70;
    }
    
    float scrollWheel = GetMouseWheelMove();
    if (scrollWheel != 0) {
        adminScrollOffset -= (int)(scrollWheel * 40);
        if (adminScrollOffset < 0) adminScrollOffset = 0;
    }
    
    if (!adminMessage.empty()) {
        Vector2 msgSize = MeasureTextEx(arialFont, adminMessage.c_str(), 18, 1);// CONTINUAÇÃO DO CÓDIGO - drawAdminDeleteScreen (final)

        DrawTextEx(arialFont, adminMessage.c_str(), 
                  Vector2{(GetScreenWidth() - msgSize.x) / 2, (float)(GetScreenHeight() - 120)}, 18, 1, GREEN);
    }
}

// ==================== INPUT HANDLING ====================

void handleTextInput(string &target, int maxLength) {
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (target.length() < (size_t)maxLength)) {
            target += (char)key;
        }
        key = GetCharPressed();
    }
    
    if (IsKeyPressed(KEY_BACKSPACE) && !target.empty()) {
        target.pop_back();
    }
}

void handleLoginInput() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        int cx = getCenterX();
        int cy = getCenterY();
        
        Rectangle userBox = {(float)(cx - 200), (float)(cy + 80), 400, 40};
        Rectangle passBox = {(float)(cx - 200), (float)(cy + 160), 400, 40};
        
        if (CheckCollisionPointRec(mousePos, userBox)) {
            loginFocusUser = true;
        } else if (CheckCollisionPointRec(mousePos, passBox)) {
            loginFocusUser = false;
        }
    }
    
    if (IsKeyPressed(KEY_TAB)) {
        loginFocusUser = !loginFocusUser;
    }
    
    if (IsKeyPressed(KEY_ENTER)) {
        for (auto &acc : accounts) {
            if (acc.username == loginUser && acc.password == loginPass) {
                currentUser = acc.username;
                isCurrentUserAdmin = acc.isAdmin;
                currentScreen = MENU;
                loginUser = loginPass = "";
                return;
            }
        }
    }
    
    if (loginFocusUser) {
        handleTextInput(loginUser, 30);
    } else {
        handleTextInput(loginPass, 30);
    }
}

void handleRegisterInput() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        int cx = getCenterX();
        int cy = getCenterY();
        
        Rectangle boxes[5] = {
            {(float)(cx - 250), (float)(cy - 120), 500, 38},
            {(float)(cx - 250), (float)(cy - 60), 500, 38},
            {(float)(cx - 250), (float)cy, 500, 38},
            {(float)(cx - 250), (float)(cy + 60), 500, 38},
            {(float)(cx - 250), (float)(cy + 120), 500, 38}
        };
        
        for (int i = 0; i < 5; i++) {
            if (CheckCollisionPointRec(mousePos, boxes[i])) {
                registerFocusField = i;
                break;
            }
        }
    }
    
    if (IsKeyPressed(KEY_TAB)) {
        registerFocusField = (registerFocusField + 1) % 5;
    }
    
    switch (registerFocusField) {
        case 0: handleTextInput(registerUser, 30); break;
        case 1: handleTextInput(registerEmail, 50); break;
        case 2: handleTextInput(registerPhone, 20); break;
        case 3: handleTextInput(registerPass, 30); break;
        case 4: handleTextInput(registerPassConfirm, 30); break;
    }
}

void handleAdminInput() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        int cx = getCenterX();
        
        Rectangle boxes[5] = {
            {(float)(cx - 300), 200, 600, 40},
            {(float)(cx - 300), 270, 600, 40},
            {(float)(cx - 300), 340, 600, 40},
            {(float)(cx - 300), 410, 600, 40},
            {(float)(cx - 300), 480, 600, 40}
        };
        
        for (int i = 0; i < 5; i++) {
            if (CheckCollisionPointRec(mousePos, boxes[i])) {
                adminFocusField = i;
                break;
            }
        }
    }
    
    if (IsKeyPressed(KEY_TAB)) {
        adminFocusField = (adminFocusField + 1) % 5;
    }
    
    switch (adminFocusField) {
        case 0: handleTextInput(adminId, 20); break;
        case 1: handleTextInput(adminTipo, 30); break;
        case 2: handleTextInput(adminNome, 100); break;
        case 3: handleTextInput(adminSpecs, 200); break;
        case 4: 
            {
                int key = GetCharPressed();
                while (key > 0) {
                    if (((key >= '0' && key <= '9') || key == '.' || key == ',') && adminPreco.length() < 15) {
                        if (key == ',') key = '.';
                        adminPreco += (char)key;
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !adminPreco.empty()) {
                    adminPreco.pop_back();
                }
            }
            break;
    }
}

// ==================== MAIN ====================

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    InitWindow(screenWidth, screenHeight, "Build Computer - Sistema de Montagem de PC");
    SetTargetFPS(60);
    
    arialFont = GetFontDefault();
    
    loadAccounts();
    if (!loadComponents("componentes.realyb")) {
        cout << "Aviso: Arquivo de componentes nao encontrado. Criando arquivo padrao..." << endl;
        
        catalog.push_back({"CPU001", "CPU", "Intel Core i5-12400F", "6-Core, 12-Thread, 2.5GHz", 899.99});
        catalog.push_back({"CPU002", "CPU", "AMD Ryzen 5 5600X", "6-Core, 12-Thread, 3.7GHz", 1099.99});
        catalog.push_back({"GPU001", "GPU", "NVIDIA RTX 3060", "12GB GDDR6", 2499.99});
        catalog.push_back({"GPU002", "GPU", "AMD RX 6600", "8GB GDDR6", 1899.99});
        catalog.push_back({"RAM001", "RAM", "Corsair Vengeance 16GB", "DDR4 3200MHz", 399.99});
        catalog.push_back({"RAM002", "RAM", "Kingston Fury 32GB", "DDR4 3600MHz", 799.99});
        catalog.push_back({"MB001", "Motherboard", "ASUS Prime B550M", "AMD AM4, Micro-ATX", 699.99});
        catalog.push_back({"MB002", "Motherboard", "MSI B660M Mortar", "Intel LGA1700, Micro-ATX", 899.99});
        catalog.push_back({"PSU001", "PSU", "Corsair CV650", "650W 80+ Bronze", 449.99});
        catalog.push_back({"PSU002", "PSU", "EVGA 750 G5", "750W 80+ Gold", 699.99});
        catalog.push_back({"CASE001", "Case", "NZXT H510", "Mid Tower ATX", 499.99});
        catalog.push_back({"CASE002", "Case", "Cooler Master TD500", "Mid Tower RGB", 599.99});
        catalog.push_back({"COOL001", "Cooler", "Cooler Master Hyper 212", "Tower Air Cooler", 199.99});
        catalog.push_back({"COOL002", "Cooler", "Corsair H100i", "240mm AIO Liquid", 599.99});
        catalog.push_back({"SSD001", "SSD", "Samsung 980 500GB", "NVMe M.2", 349.99});
        catalog.push_back({"SSD002", "SSD", "WD Black SN850 1TB", "NVMe Gen4", 699.99});
        catalog.push_back({"HDD001", "HDD", "Seagate Barracuda 2TB", "7200 RPM SATA", 399.99});
        
        saveComponents();
    }
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground({15, 15, 15, 255});
        
        switch (currentScreen) {
            case LOGIN:
                drawLoginScreen();
                handleLoginInput();
                break;
                
            case REGISTER:
                drawRegisterScreen();
                handleRegisterInput();
                break;
                
            case MENU:
                drawMenuScreen();
                break;
                
            case ESCOLHER_PECAS:
                drawEscolherPecasScreen();
                break;
                
            case VER_PRECOS:
                drawVerPrecosScreen();
                break;
                
            case COMO_MONTAR:
                drawComoMontarScreen();
                break;
                
            case ADMIN_PANEL:
                drawAdminPanelScreen();
                break;
                
            case ADMIN_ADD:
                drawAdminAddScreen();
                handleAdminInput();
                break;
                
            case ADMIN_EDIT:
                drawAdminEditScreen();
                if (adminSelectedIndex != -1) {
                    handleAdminInput();
                }
                break;
                
            case ADMIN_DELETE:
                drawAdminDeleteScreen();
                break;
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}