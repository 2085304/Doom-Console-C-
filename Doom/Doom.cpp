#include <iostream>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>
#include <algorithm>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#endif

using namespace std;

const int MAP_WIDTH = 24;
const int MAP_HEIGHT = 24;
const int SCREEN_WIDTH = 80;
const int SCREEN_HEIGHT = 24;
const float FOV = 3.14159f / 3.0f;
const float DEPTH = 16.0f;

// Carte du niveau (1 = mur, 0 = vide)
int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1},
    {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,1},
    {1,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,0,1},
    {1,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,0,1},
    {1,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

struct Enemy {
    float x, y;
    int health;
    bool alive;
    
    Enemy(float px, float py) : x(px), y(py), health(100), alive(true) {}
};

class Game {
private:
    float playerX, playerY;
    float playerA;
    int playerHealth;
    int ammo;
    vector<Enemy> enemies;
    bool running;
    char screen[SCREEN_HEIGHT][SCREEN_WIDTH + 1];
    
public:
    Game() : playerX(2.0f), playerY(2.0f), playerA(0.0f), 
             playerHealth(100), ammo(50), running(true) {
        // Initialiser les ennemis
        enemies.push_back(Enemy(10.0f, 10.0f));
        enemies.push_back(Enemy(15.0f, 15.0f));
        enemies.push_back(Enemy(18.0f, 5.0f));
        enemies.push_back(Enemy(5.0f, 18.0f));
        enemies.push_back(Enemy(20.0f, 20.0f));
    }
    
    void clearScreen() {
        #ifdef _WIN32
        system("cls");
        #else
        system("clear");
        #endif
    }
    
    int kbhit() {
        #ifdef _WIN32
        return _kbhit();
        #else
        struct timeval tv = {0, 0};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
        #endif
    }
    
    char getKey() {
        #ifdef _WIN32
        return _getch();
        #else
        return getchar();
        #endif
    }
    
    void render() {
        // Initialiser l'écran
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                screen[y][x] = ' ';
            }
            screen[y][SCREEN_WIDTH] = '\0';
        }
        
        // Raycasting pour les murs
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            float rayAngle = (playerA - FOV / 2.0f) + ((float)x / (float)SCREEN_WIDTH) * FOV;
            float distanceToWall = 0.0f;
            bool hitWall = false;
            bool boundary = false;
            
            float eyeX = sin(rayAngle);
            float eyeY = cos(rayAngle);
            
            while (!hitWall && distanceToWall < DEPTH) {
                distanceToWall += 0.1f;
                
                int testX = (int)(playerX + eyeX * distanceToWall);
                int testY = (int)(playerY + eyeY * distanceToWall);
                
                if (testX < 0 || testX >= MAP_WIDTH || testY < 0 || testY >= MAP_HEIGHT) {
                    hitWall = true;
                    distanceToWall = DEPTH;
                } else if (worldMap[testY][testX] == 1) {
                    hitWall = true;
                }
            }
            
            // Calculer la hauteur du mur à dessiner
            int ceiling = (float)(SCREEN_HEIGHT / 2.0) - SCREEN_HEIGHT / ((float)distanceToWall);
            int floor = SCREEN_HEIGHT - ceiling;
            
            char wallShade = ' ';
            if (distanceToWall <= DEPTH / 4.0f)      wallShade = '#';
            else if (distanceToWall < DEPTH / 3.0f)  wallShade = 'X';
            else if (distanceToWall < DEPTH / 2.0f)  wallShade = 'x';
            else if (distanceToWall < DEPTH)         wallShade = '.';
            else                                      wallShade = ' ';
            
            for (int y = 0; y < SCREEN_HEIGHT; y++) {
                if (y <= ceiling) {
                    screen[y][x] = ' ';
                } else if (y > ceiling && y <= floor) {
                    screen[y][x] = wallShade;
                } else {
                    float b = 1.0f - (((float)y - SCREEN_HEIGHT / 2.0f) / ((float)SCREEN_HEIGHT / 2.0f));
                    if (b < 0.25)      screen[y][x] = '#';
                    else if (b < 0.5)  screen[y][x] = 'x';
                    else if (b < 0.75) screen[y][x] = '.';
                    else if (b < 0.9)  screen[y][x] = '-';
                    else               screen[y][x] = ' ';
                }
            }
        }
        
        // Dessiner les ennemis
        for (auto& enemy : enemies) {
            if (!enemy.alive) continue;
            
            float enemyDx = enemy.x - playerX;
            float enemyDy = enemy.y - playerY;
            float enemyDistance = sqrt(enemyDx * enemyDx + enemyDy * enemyDy);
            
            float enemyAngle = atan2(enemyDy, enemyDx) - playerA + 3.14159f / 2.0f;
            
            // Normaliser l'angle
            if (enemyAngle < -3.14159f) enemyAngle += 2.0f * 3.14159f;
            if (enemyAngle > 3.14159f) enemyAngle -= 2.0f * 3.14159f;
            
            bool inFOV = fabs(enemyAngle) < FOV / 2.0f;
            
            if (inFOV && enemyDistance < DEPTH) {
                int enemyScreenX = (int)((FOV / 2.0f + enemyAngle) / FOV * (float)SCREEN_WIDTH);
                int enemySize = (int)((float)SCREEN_HEIGHT / enemyDistance);
                
                int enemyTop = SCREEN_HEIGHT / 2 - enemySize / 2;
                int enemyBottom = SCREEN_HEIGHT / 2 + enemySize / 2;
                
                if (enemyScreenX >= 0 && enemyScreenX < SCREEN_WIDTH) {
                    for (int y = max(0, enemyTop); y < min(SCREEN_HEIGHT, enemyBottom); y++) {
                        screen[y][enemyScreenX] = 'E';
                    }
                }
            }
        }
        
        // Afficher l'écran
        clearScreen();
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            cout << screen[y] << endl;
        }
        
        // Afficher les stats
        cout << "====================================================================================" << endl;
        cout << "Santé: " << playerHealth << " | Munitions: " << ammo << " | Ennemis: ";
        int aliveCount = 0;
        for (auto& e : enemies) if (e.alive) aliveCount++;
        cout << aliveCount << endl;
        cout << "Contrôles: W/S=Avancer/Reculer | A/D=Tourner | ESPACE=Tirer | Q=Quitter" << endl;
    }
    
    void update() {
        if (kbhit()) {
            char key = getKey();
            
            float moveSpeed = 0.3f;
            float rotSpeed = 0.1f;
            
            switch (key) {
                case 'w':
                case 'W': {
                    float newX = playerX + sin(playerA) * moveSpeed;
                    float newY = playerY + cos(playerA) * moveSpeed;
                    if (worldMap[(int)newY][(int)newX] == 0) {
                        playerX = newX;
                        playerY = newY;
                    }
                    break;
                }
                case 's':
                case 'S': {
                    float newX = playerX - sin(playerA) * moveSpeed;
                    float newY = playerY - cos(playerA) * moveSpeed;
                    if (worldMap[(int)newY][(int)newX] == 0) {
                        playerX = newX;
                        playerY = newY;
                    }
                    break;
                }
                case 'a':
                case 'A':
                    playerA -= rotSpeed;
                    break;
                case 'd':
                case 'D':
                    playerA += rotSpeed;
                    break;
                case ' ':
                    shoot();
                    break;
                case 'q':
                case 'Q':
                    running = false;
                    break;
            }
        }
        
        // IA des ennemis
        for (auto& enemy : enemies) {
            if (!enemy.alive) continue;
            
            float dx = playerX - enemy.x;
            float dy = playerY - enemy.y;
            float dist = sqrt(dx * dx + dy * dy);
            
            if (dist < 8.0f && dist > 1.0f) {
                float moveX = dx / dist * 0.02f;
                float moveY = dy / dist * 0.02f;
                
                if (worldMap[(int)(enemy.y + moveY)][(int)(enemy.x + moveX)] == 0) {
                    enemy.x += moveX;
                    enemy.y += moveY;
                }
            }
            
            if (dist < 1.5f && rand() % 100 < 5) {
                playerHealth -= 5;
            }
        }
    }
    
    void shoot() {
        if (ammo <= 0) return;
        ammo--;
        
        // Chercher un ennemi dans la ligne de mire
        for (auto& enemy : enemies) {
            if (!enemy.alive) continue;
            
            float dx = enemy.x - playerX;
            float dy = enemy.y - playerY;
            float distance = sqrt(dx * dx + dy * dy);
            
            float angle = atan2(dy, dx) - playerA + 3.14159f / 2.0f;
            
            if (angle < -3.14159f) angle += 2.0f * 3.14159f;
            if (angle > 3.14159f) angle -= 2.0f * 3.14159f;
            
            if (fabs(angle) < 0.1f && distance < 10.0f) {
                enemy.health -= 50;
                if (enemy.health <= 0) {
                    enemy.alive = false;
                }
                break;
            }
        }
    }
    
    bool isRunning() {
        return running && playerHealth > 0;
    }
    
    bool hasWon() {
        for (auto& e : enemies) {
            if (e.alive) return false;
        }
        return true;
    }
};

int main() {
    srand(time(NULL));
    
    #ifndef _WIN32
    // Désactiver le mode canonique sur Linux
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    #endif
    
    Game game;
    
    cout << "=== CONSOLE DOOM ===" << endl;
    cout << "Éliminez tous les ennemis pour gagner!" << endl;
    cout << "Appuyez sur une touche pour commencer..." << endl;
    cin.get();
    
    while (game.isRunning()) {
        game.update();
        game.render();
        
        #ifdef _WIN32
        Sleep(50);
        #else
        usleep(50000);
        #endif
    }
    
    game.render();
    
    if (game.hasWon()) {
        cout << "\n=== VICTOIRE! Tous les ennemis sont éliminés! ===" << endl;
    } else {
        cout << "\n=== GAME OVER! Vous êtes mort... ===" << endl;
    }
    
    #ifndef _WIN32
    // Restaurer le mode terminal
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    #endif
    
    return 0;
}
