#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <time.h>

#define WIDTH 31
#define HEIGHT 15
#define COOKIE '.'    
#define WALL '#'
#define PACMAN 'C'
#define GHOST 'G'
#define POWER_PELLET 'O'

typedef struct {
    int x, y;
    int score;
    int life;
    int cookies_left;
    int startX, startY;
    int dx, dy; // 팩맨 이동 방향 추가
} GameStatus;

typedef struct {
    int x, y;
    int startX, startY;
} Ghost;

int frightened_timer = 0; // 겁먹은 상태 타이머

void gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void hideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.bVisible = FALSE;
    cursorInfo.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

void setColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void showStartScreen() {
    printf("########################################\n");
    printf("#                                      #\n");
    printf("#             팩맨 게임                #\n");
    printf("#                                      #\n");
    printf("#         'S'를 눌러 시작하기          #\n");
    printf("#         'Q'를 눌러 종료하기          #\n");
    printf("#                                      #\n");
    printf("#    조작법: W,A,S,D로 이동            #\n");
    printf("#    파워필렛: 고스트 겁먹게 만들기    #\n");
    printf("#                                      #\n");
    printf("########################################\n");
}

void createGameMap(char map[HEIGHT][WIDTH], GameStatus* status) {
    const char final_map[HEIGHT][WIDTH + 1] = {
        "##########.#####################",
        "...............O..........#....#",
        "#.#####.#########.#####.#.#.###",
        "#.#...#.#.......#.#...#.#....#",
        "#.#.#.#.#.#####.#.#.#.#.####.#",
        "#.#.#.#.#.#...#.#.#.#.#......#",
        "#.#.#.#.#.#.#.#.#.#.#.########",
        "#...#...#...#.#.#.#.#....O...#",
        "###.#.#######.#.#.#.########.##",
        "....#.........#.#.#............",
        "#.###########.#.#.#.#######..##",
        "#.............#...........#..#",
        "#.#############.###########..#",
        "#.........O...............#..#",
        "##########.###################",
    };

    status->cookies_left = 0;
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            map[i][j] = final_map[i][j];
            if (map[i][j] == COOKIE) {
                status->cookies_left++;
            }
        }
    }
}

void drawGameScreen(char map[HEIGHT][WIDTH], GameStatus status, Ghost ghosts[4]) {
    WORD ghostColorsNormal[4] = { 0x0C, 0x0D, 0x0A, 0x09 };
    WORD frightenedColor = 0x01; // 파란색

    gotoxy(0, 0);
    setColor(0x07);
    printf("점수: %d  목숨: %d  남은 쿠키: %d  겁먹은타이머: %d\n", status.score, status.life, status.cookies_left, frightened_timer);

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            int drawn = 0;
            // 팩맨
            if (i == status.y && j == status.x) {
                setColor(0x0E);
                putchar(PACMAN);
                drawn = 1;
            }
            else {
                // 고스트
                for (int g = 0; g < 4; g++) {
                    if (i == ghosts[g].y && j == ghosts[g].x) {
                        if (frightened_timer > 0) {
                            // 겁먹은 고스트 색상
                            setColor(frightenedColor);
                        }
                        else {
                            setColor(ghostColorsNormal[g]);
                        }
                        putchar(GHOST);
                        drawn = 1;
                        break;
                    }
                }
            }

            if (!drawn) {
                char c = map[i][j];
                if (c == WALL) {
                    setColor(0x08);
                }
                else if (c == COOKIE) {
                    setColor(0x0E);
                }
                else if (c == POWER_PELLET) {
                    // 파워필렛을 밝은색
                    setColor(0x0F);
                }
                else {
                    setColor(0x07);
                }
                putchar(c);
            }
        }
        putchar('\n');
    }
    setColor(0x07);
}

void applyTunnel(int* x, int* y) {
    if (*x < 0) *x = WIDTH - 1;
    else if (*x >= WIDTH) *x = 0;

    if (*y < 0) *y = HEIGHT - 1;
    else if (*y >= HEIGHT) *y = 0;
}

// 이동 함수 수정: dx, dy를 받아 자동으로 이동(여기서는 팩맨의 이동을 호출하는 시점만 변경)
void moveCharacter(char map[HEIGHT][WIDTH], GameStatus* status) {
    int new_x = status->x + status->dx;
    int new_y = status->y + status->dy;

    // 팩맨만 터널 적용
    applyTunnel(&new_x, &new_y);

    if (map[new_y][new_x] != WALL) {
        if (map[new_y][new_x] == COOKIE) {
            status->score += 10;
            status->cookies_left--;
            map[new_y][new_x] = ' ';
        }
        else if (map[new_y][new_x] == POWER_PELLET) {
            status->score += 50; // 파워필렛 점수
            frightened_timer = 100;
            map[new_y][new_x] = ' ';
        }
        status->x = new_x;
        status->y = new_y;
    }
}

void getNextGhostMove(char map[HEIGHT][WIDTH], int gx, int gy, int tx, int ty, int* nx, int* ny) {
    int visited[HEIGHT][WIDTH] = { 0 };
    int parentX[HEIGHT][WIDTH];
    int parentY[HEIGHT][WIDTH];

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            parentX[i][j] = -1;
            parentY[i][j] = -1;
        }
    }

    int directions[4][2] = { {0,-1},{0,1},{-1,0},{1,0} };
    int qx[HEIGHT * WIDTH], qy[HEIGHT * WIDTH];
    int front = 0, rear = 0;

    qx[rear] = gx; qy[rear++] = gy;
    visited[gy][gx] = 1;

    int found = 0;
    while (front < rear) {
        int cx = qx[front];
        int cy = qy[front++];
        if (cx == tx && cy == ty) {
            found = 1;
            break;
        }

        for (int i = 0; i < 4; i++) {
            int nx_ = cx + directions[i][0];
            int ny_ = cy + directions[i][1];

            // 고스트는 맵 밖 이동 불가
            if (nx_ < 0 || nx_ >= WIDTH || ny_ < 0 || ny_ >= HEIGHT)
                continue;

            if (!visited[ny_][nx_] && map[ny_][nx_] != WALL) {
                visited[ny_][nx_] = 1;
                parentX[ny_][nx_] = cx;
                parentY[ny_][nx_] = cy;
                qx[rear] = nx_; qy[rear++] = ny_;
            }
        }
    }

    if (!found) {
        *nx = -1;
        *ny = -1;
        return;
    }

    int tx_ = tx, ty_ = ty;
    while (parentX[ty_][tx_] != -1 && parentY[ty_][tx_] != -1) {
        int px_ = parentX[ty_][tx_];
        int py_ = parentY[ty_][tx_];
        if (px_ == gx && py_ == gy) {
            *nx = tx_;
            *ny = ty_;
            return;
        }
        tx_ = px_;
        ty_ = py_;
    }

    *nx = -1;
    *ny = -1;
}

void getGhostTarget(GameStatus* status, int ghostIndex, int* tx, int* ty) {
    int px = status->x;
    int py = status->y;

    // 겁먹지 않은 상태에서 고스트 목표
    switch (ghostIndex) {
    case 0:
        *tx = px;
        *ty = py;
        break;
    case 1:
        *tx = px;
        *ty = py - 2;
        break;
    case 2:
        *tx = px - 2;
        *ty = py;
        break;
    case 3:
        *tx = px + 2;
        *ty = py;
        break;
    }

    // 범위 벗어나면 해당 지점은 무시
    if (*tx < 0) *tx = 0;
    if (*tx >= WIDTH) *tx = WIDTH - 1;
    if (*ty < 0) *ty = 0;
    if (*ty >= HEIGHT) *ty = HEIGHT - 1;
}

void moveGhost(char map[HEIGHT][WIDTH], Ghost* ghost, GameStatus* status, int ghostIndex) {
    if (frightened_timer > 0) {
        // 겁먹은 고스트 랜덤 이동
        int directions[4][2] = { {0,-1},{0,1},{-1,0},{1,0} };
        int tryCount = 0;
        while (tryCount < 10) {
            int dir = rand() % 4;
            int rx = ghost->x + directions[dir][0];
            int ry = ghost->y + directions[dir][1];

            // 고스트는 맵 밖 이동 불가
            if (rx < 0 || rx >= WIDTH || ry < 0 || ry >= HEIGHT) {
                tryCount++;
                continue;
            }

            if (map[ry][rx] != WALL) {
                ghost->x = rx;
                ghost->y = ry;
                break;
            }
            tryCount++;
        }
    }
    else {
        // 겁먹지 않았을 때 BFS
        int tx, ty;
        getGhostTarget(status, ghostIndex, &tx, &ty);

        int nx, ny;
        getNextGhostMove(map, ghost->x, ghost->y, tx, ty, &nx, &ny);

        // 경로 없을 경우 랜덤 이동
        if (nx == -1 || ny == -1) {
            int directions[4][2] = { {0,-1},{0,1},{-1,0},{1,0} };
            int dir = rand() % 4;
            int rx = ghost->x + directions[dir][0];
            int ry = ghost->y + directions[dir][1];

            if (rx >= 0 && rx < WIDTH && ry >= 0 && ry < HEIGHT && map[ry][rx] != WALL) {
                ghost->x = rx;
                ghost->y = ry;
            }
        }
        else {
            ghost->x = nx;
            ghost->y = ny;
        }
    }
}

void checkCollision(GameStatus* status, Ghost ghosts[4]) {
    for (int g = 0; g < 4; g++) {
        if (status->x == ghosts[g].x && status->y == ghosts[g].y) {
            if (frightened_timer > 0) {
                // 겁먹은 고스트 잡기
                status->score += 200;
                ghosts[g].x = ghosts[g].startX;
                ghosts[g].y = ghosts[g].startY;
            }
            else {
                // 고스트와 충돌 -> 팩맨 사망
                status->life--;
                status->x = status->startX;
                status->y = status->startY;
                // 방향도 초기화
                status->dx = 0;
                status->dy = 0;
            }
        }
    }
}

void startGame() {
    char map[HEIGHT][WIDTH];
    GameStatus status = { 7, 1, 0, 3, 0, 7, 1, 0, 0 };
    createGameMap(map, &status);
    hideCursor();

    Ghost ghosts[4] = {
        {10, 10, 10, 10},
        {15,  7, 15,  7},
        {20, 10, 20, 10},
        {25,  7, 25,  7}
    };

    srand((unsigned int)time(NULL));
    int frameCount = 0;
    int pacmanMoveCounter = 0; // 팩맨 이동속도 조절용 카운터

    while (status.life > 0 && status.cookies_left > 0) {
        drawGameScreen(map, status, ghosts);

        // 방향 전환용 키 입력
        if (_kbhit()) {
            char key = _getch();
            if (key == 'q') break;
            if (key == 'w') { status.dx = 0; status.dy = -1; }
            else if (key == 's') { status.dx = 0; status.dy = 1; }
            else if (key == 'a') { status.dx = -1; status.dy = 0; }
            else if (key == 'd') { status.dx = 1; status.dy = 0; }
        }

        // 팩맨 이동 빈도 조절(2프레임에 한 번씩 이동)
        pacmanMoveCounter++;
        if (pacmanMoveCounter % 2 == 0) {
            moveCharacter(map, &status);
        }

        frameCount++;
        if (frightened_timer > 0) {
            frightened_timer--;
        }

        if (frameCount % 5 == 0) {
            for (int g = 0; g < 4; g++) {
                moveGhost(map, &ghosts[g], &status, g);
            }
        }

        checkCollision(&status, ghosts);

        // 전체 게임 루프 속도: 필요하다면 Sleep(200) 등으로 늘릴 수도 있음
        Sleep(100);
    }

    system("cls");
    if (status.cookies_left == 0) {
        printf("\n축하합니다! 모든 쿠키를 다 모았습니다!\n");
        printf("최종 점수: %d\n", status.score);
    }
    else if (status.life <= 0) {
        printf("\n게임 오버! 목숨이 다했습니다.\n");
        printf("최종 점수: %d\n", status.score);
    }
    else {
        printf("\n게임을 종료합니다.\n");
        printf("최종 점수: %d\n", status.score);
    }
}

int main() {
    char key;

    while (1) {
        system("cls");
        showStartScreen();
        printf("옵션을 선택하세요: ");
        key = _getch();

        if (key == 'S' || key == 's') {
            printf("\n게임을 시작합니다...\n");
            Sleep(1000);
            system("cls");
            startGame();
            break;
        }
        else if (key == 'Q' || key == 'q') {
            printf("\n게임을 종료합니다. 안녕히 가세요!\n");
            return 0;
        }
        else {
            printf("\n잘못된 입력입니다. 'S'로 시작하거나 'Q'로 종료하세요.\n");
            Sleep(1000);
        }
    }

    return 0;
}



