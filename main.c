#include<stdio.h>
#include<stdlib.h>
#include<time.h>

#pragma warning(disable:4996)

//#define WIN_CONSOLE

#ifndef WIN_CONSOLE
#include <Windows.h>
#endif //WIN_CONSOLE

typedef enum CellState{
	UNTOUCHED = 0, TOUCHED, FLAGGED, QUESTION
}CellState;

typedef enum GameFlag{
	PREINIT = 0, INIT, LOSE, WIN
}Flag;

typedef struct{
	unsigned st  : 2;
	/*  Of type cellstate */
	unsigned num : 4;
	/*  0 for no mines in the adjacent 8 cells
		1-8 for 1-8 mine(s) in 8 cells
		9 for a mine
	*/
}CellInfo;

/* The maximum size of the board : (BOARDW - 2) by (BOARDH - 2). */
#define BOARDW 30
#define BOARDH 30

/* Infomation about the board. */
CellInfo board[BOARDW][BOARDH];

/* Game state. */
Flag state;

/* Whether question mark available. */
struct settings{
	unsigned flagmode : 1;
	unsigned performance : 1;
	unsigned boardwidth : 5;
	unsigned boardheight : 5;
	unsigned minenum : 5;
}settings;

/* Score related */
time_t laststart;
int oprcnt;

void init();/** To initialize the whole game. */
void resize(int w, int h, int b);/** To resize the board. */
void binit();/** To initialize the board. */
void mine(int x, int y);/** To create mines on the board. */
void touch(int x, int y);/** To perform touch on the board. */
void flag(int x, int y, int no_cancel);/** To perform flag on the board. */
void lclick(int x, int y);/** To perform a left-click on the board. */
void rclick(int x, int y);/** To perform a right-click on the board. */
void bclick(int x, int y);/** To perform a both-left-and-right click. */
void exec();/** To input and execute commands. */
void pchar1(CellInfo c); void pchar(CellInfo c);/** To print the char to display. */
void print();/** To print current board. */
void judge();/** To judge if the player has won. */

int main(){
#ifndef WIN_CONSOLE
	HWND cmd = GetConsoleWindow();
	HDC dc = GetDC(cmd);
	HDC dc1 = CreateCompatibleDC(dc);
	HBITMAP bitmap = (HBITMAP) LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(103/*IDB_BITMAP2*/), IMAGE_BITMAP, 65, 65, LR_CREATEDIBSECTION);
#endif
	char c;
	int num1, num2, num3;
	init();
	printf("Game start?(N/n for no, s for setup)\n");
	while(1){
		scanf(" %c", &c);
		if(c == 'N' || c == 'n')break;
		if(c == 's'){
			printf("Please input: width, height of the board and number of bombs.\n");
			scanf("%d %d %d", &num1, &num2, &num3);
			resize(num1, num2, num3);
			printf("Please input: whether question mark should be displayed when an r-click happens on a flagged square.");
			scanf("%d", &num1);
			settings.flagmode = (num1 != 0);
		}
		binit();
		print();
		while(state < LOSE){
            exec();
            judge();
            print();
		}
	}
	return 0;
}

/** To initialize the whole game. */
void init(){
	settings.boardwidth = settings.boardheight = 10;
	settings.minenum = 10;
	srand((unsigned)time(NULL));
	settings.flagmode = settings.performance = 1;
#ifdef WIN_CONSOLE
	system("color 70");
#endif
}

/** To resize the board. */
void resize(int w, int h, int b){
	if(w > 4 && w < BOARDW - 1 && h > 4 && h < BOARDH - 1){
		settings.boardwidth = w;
		settings.boardheight = h;
	} else {
		printf("Invalid size!");
		return;
	}
	if(b == 0)return;
	if(b > w * h / 8 && b < w * h / 4){
		settings.minenum = b;
	}  else {
		printf("Invalid bomb number!");
	}
}

/** To initialize the board. */
void binit(){
	int i, j;
    state = PREINIT;
	for(i = 0; i <= settings.boardheight + 1; i++){
		for(j = 0; j <= settings.boardwidth + 1; j++){
			board[i][j].num = 0;
			board[i][j].st = UNTOUCHED;
		}
	}
}

/** To create mines on the board. To do on the first click. */
void mine(int x, int y){
    int i, j, n, X, Y;
	laststart = time(NULL);
	state = INIT;
	for(n = 0; n < settings.minenum; ){
		i = rand() % (settings.boardwidth - 1) + 1;
		j = rand() % (settings.boardheight - 1) + 1;
		if(board[i][j].num == 9)continue;
		if(i == x && j == y)continue;/* No mine should be in the first cell touched. */
		board[i][j].num = 9;
		n++;
	}
	for(i = 1; i <= settings.boardheight; i++){
		for(j = 1; j <= settings.boardwidth; j++){
			if(board[i][j].num == 9)continue;
			n = 0;
			for(X = -1; X <= 1; X++){
				for(Y = -1; Y <= 1; Y++){
					if( (X!=0 || Y!=0) && board[i + X][j + Y].num == 9){
						n++;
					}
				}
			}
			board[i][j].num = n;
		}
	}
}

/** To perform touch on the board. */
void touch(int x, int y){
	int X, Y;
	if(x <= 0 || x > settings.boardheight || y <= 0 || y > settings.boardwidth)return;
	if(board[x][y].st != UNTOUCHED)return;
	board[x][y].st = TOUCHED;
	if(board[x][y].num == 0){
		for(X = -1; X <= 1; X++){
			for(Y = -1; Y <= 1; Y++){
				if( X!=0 || Y!=0 ){
					touch(x + X, y + Y);
				}
			}
		}
	}else if(board[x][y].num == 9){
		state = LOSE;
	}
}

/** To perform flag on the board. */
void flag(int x, int y, int no_cancel){
	if(x <= 0 || x > settings.boardheight || y <= 0 || y > settings.boardwidth)return;
	switch(board[x][y].st){
	case UNTOUCHED:
		board[x][y].st = FLAGGED;
		break;
	case FLAGGED:
		if(no_cancel)break;
		board[x][y].st = settings.flagmode ? UNTOUCHED : QUESTION;
		break;
    case QUESTION:
        board[x][y].st = UNTOUCHED;
		break;
	}
}

/** To perform a left-click on the board. */
void lclick(int x, int y){
	if(state >= LOSE)return;
	touch(x, y);
}

/** To perform a right-click on the board. */
void rclick(int x, int y){
	if(state >= LOSE)return;
	flag(x, y, 0);
}

/** To perform a both-left-and-right click. */
void bclick(int x, int y){
	int X, Y;
	int u=0, f=0;
	if(x <= 0 || x > settings.boardheight || y <= 0 || y > settings.boardwidth)return;
	if(board[x][y].st != TOUCHED)return;
    for(X = -1; X <= 1; X++)
        for(Y = -1; Y <= 1; Y++){
            if( X!=0 || Y!=0 ){
                if(x + X <= 0 || x + X > settings.boardheight ||
                    y + Y <= 0 || y + Y > settings.boardwidth)continue;
                switch(board[x + X][y + Y].st){
                case UNTOUCHED:
                    u++;
                    break;
                case FLAGGED:
                    f++;
                    break;
                }
            }
        }
    if(f == board[x][y].num){
        for(X = -1; X <= 1; X++){
            for(Y = -1; Y <= 1; Y++){
                if( X!=0 || Y!=0 ){
                    touch(x + X, y + Y);
                }
            }
		}
    }
    else if(u + f == board[x][y].num){
        for(X = -1; X <= 1; X++)
            for(Y = -1; Y <= 1; Y++){
                if( X!=0 || Y!=0 ){
                    flag(x + X, y + Y, 1);
                }
            }
    }
}

/** To input and execute commands. */
void exec(){
	char buffer[10];
	char c;
	int x, y;
	fflush(stdin);
	fgets(buffer, 10, stdin);
	sscanf(buffer, " %c %d %d", &c, &x, &y);
	if(state == PREINIT)mine(x, y);
	switch(c){
	case 'L':
	case 'l':
		lclick(x, y);
		return;
	case 'R':
	case 'r':
		rclick(x, y);
		return;
    case 'B':
    case 'b':
        bclick(x, y);
        return;
	}
}

/** To print current board. */
void print(){
	int i, j;
#ifdef WIN_CONSOLE
	system("cls");
#endif
	if(settings.performance){
		printf("┏");
		for(j = 0; j < settings.boardwidth - 1; j++)
			printf("━┳");
		printf("━┓\n");
	}
	for(i = 1; i <= settings.boardheight; i++){
		for(j = 1; j <= settings.boardwidth; j++){
			if(settings.performance){
				printf("┃");
				pchar1(board[i][j]);
			}else{
				pchar(board[i][j]);
			}
		}
		if(settings.performance){
			printf("┃\n");
			if(i != settings.boardheight){
				printf("┣");
				for(j = 0; j < settings.boardwidth - 1; j++)
					printf("━╋");
				printf("━┫\n");
			}
		}else{
			printf("\n");
		}
	}
	if(settings.performance){
		printf("┗");
		for(j = 0; j < settings.boardwidth - 1; j++)
			printf("━┻");
		printf("━┛\n");
	}
	if(state == WIN)
        printf("\nYou made it!\nPlay again?(n for no, s for setup)\n");
    else if(state == LOSE)
        printf("\nGame over!\nPlay again?(n for no, s for setup)\n");
    else
        printf("\nPlease input the next command:\n");
}

/** To print the char to display. */
void pchar1(CellInfo c){
	switch(c.st){
	case UNTOUCHED:
		if(state >= LOSE && c.num == 9){
			printf("◎");
        }else{
			printf("　");
		}
		break;
	case FLAGGED:
		if(state >= LOSE && c.num != 9){
            printf("Ｘ");
		}else{
			printf("★");
        }
        break;
	case TOUCHED:
		switch(c.num){
        case 0:
            printf("０");
            break;
        case 1:
            printf("１");
            break;
        case 2:
            printf("２");
            break;
        case 3:
            printf("３");
            break;
        case 4:
            printf("４");
            break;
        case 5:
            printf("５");
            break;
        case 6:
            printf("６");
            break;
		case 7:
            printf("７");
            break;
        case 8:
            printf("８");
            break;
        case 9:
			printf("●");
			break;
		}
        break;
    default:
        printf("？");
	}
	return;
}
void pchar(CellInfo c){
	switch(c.st){
	case UNTOUCHED:
		if(state >= LOSE && c.num == 9)
			putchar('#');
		else
			putchar('.');
        break;
	case FLAGGED:
		if(state >=LOSE && c.num != 9)
			putchar('X');
		else
			putchar('F');
        break;
	case TOUCHED:
		if(c.num != 9)
			putchar('0' + c.num);
		else
			putchar('*');
        break;
    default:
        putchar('?');
	}
}

void judge(){
    int i, j, n;
    if(state >= LOSE)return;
    n = 0;
    for(i = 1; i <= settings.boardheight; i++){
        for(j = 1; j <= settings.boardwidth; j++){
            if(board[i][j].st != TOUCHED)n++;
        }
    }
    if(n == settings.minenum)state = WIN;
}
