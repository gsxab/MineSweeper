#include<stdio.h>
#include<stdlib.h>
#include<time.h>

#include "resource.h"

#pragma warning(disable:4996)
#pragma warning(disable:4018) //All comparisons are safe enough for only range like 0<signed<unsigned and equality will be checked.

//#define CHAR_ONLY

#ifndef CHAR_ONLY
#define PX_PER_CELL 16
#include <Windows.h>
#endif //CHAR_ONLY

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
	unsigned minenum : 8;
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
#ifndef CHAR_ONLY
	void ppic(int x, int y);
#else
	void pchar1(CellInfo c); void pchar(CellInfo c);/** To print the char to display. */
#endif
void print();/** To print current board. */
void judge();/** To judge if the player has won. */

#ifndef CHAR_ONLY
HWND cmd;
HDC dc, dcb;
HBITMAP bitmap;
#endif

int main(){
	char c;
	int num1, num2, num3;
#ifndef CHAR_ONLY
	cmd = GetConsoleWindow();
	dc = GetDC(cmd);
	dcb = CreateCompatibleDC(dc);
	bitmap = (HBITMAP) LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP2), IMAGE_BITMAP, 65, 65, LR_CREATEDIBSECTION);
	SelectObject(dcb, bitmap);
#endif
	init();
	printf("Game start?(N/n for no, S/s for setup)\n");
	while(1){
		scanf(" %c", &c);
		if(c == 'N' || c == 'n')break;
		if(c == 'S' || c == 's'){
			printf("Please input: width, height of the board and number of bombs.\n");
			scanf("%d %d %d", &num1, &num2, &num3);
			resize(num1, num2, num3);
			printf("Please input: whether question mark should be displayed when an r-click happens on a flagged square.(y-Yes n-No default-Yes)");
			scanf(" %c", &c);
			settings.flagmode = !(c == 'N' || c == 'n');
#ifdef CHAR_ONLY
			printf("Please input: whether the board should be displayed with borders.(y-Yes n-No,simply characters default-Yes)");
			scanf(" %c", &c);
			settings.performance = !(c == 'N' || c == 'n');
#endif
		}
		binit();
		print();
		while(state < LOSE){
            exec();
            judge();
            print();
		}
	}
#ifndef CHAR_ONLY
	DeleteObject(bitmap);
	DeleteDC(dcb);
	ReleaseDC(cmd, dc);
#endif
	return 0;
}

/** To initialize the whole game. */
void init(){
	settings.boardwidth = settings.boardheight = 10;
	settings.minenum = 10;
	srand((unsigned)time(NULL));
	settings.flagmode = settings.performance = 1;
	system("color 70");
}

/** To resize the board. */
void resize(int w, int h, int b){
	if(w > 4 && w < BOARDW - 1 && h > 4 && h < BOARDH - 1){
		settings.boardwidth = w;
		settings.boardheight = h;
	} else {
		printf("Invalid size! No change will be done.");
		return;
	}
	if(b == 0)b = settings.minenum;
	if(b >= w && b >= h && b <= w * h / 4){
		settings.minenum = b;
	}  else {
		printf("Invalid bomb number! Default(width*height/5) has been set.");
		settings.minenum = w * h / 5;
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
		i = rand() % settings.boardwidth + 1;
		j = rand() % settings.boardheight + 1;
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
		board[x][y].st = settings.flagmode ? QUESTION : UNTOUCHED;
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
#ifndef CHAR_ONLY
void print(){
	int i, j;
	system("cls");
	for(i = 1; i <= settings.boardheight; i++){
		printf("\n");
	}
	for(i = 1; i <= settings.boardheight; i++){
		for(j = 1; j <= settings.boardwidth; j++){
			ppic(i,j);
		}
	}
	if(state == WIN){
        printf("\nYou made it!\nPlay again?(n for no, s for setup)\n");
		printf("Time:%lf seconds.", difftime(laststart,time(NULL)));
	}else if(state == LOSE){
        printf("\nGame over!\nPlay again?(n for no, s for setup)\n");
	}else{
        printf("\nPlease input the next command:\n");
	}
}
#else
void print(){
	int i, j;
	system("cls");
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
#endif

/** To print the char to display. */
#ifndef CHAR_ONLY
#define DRAW(ix, iy) BitBlt(dc, (y-1)*PX_PER_CELL+1, (x-1)*PX_PER_CELL+1, PX_PER_CELL, PX_PER_CELL, dcb, ix*PX_PER_CELL + 1, iy*PX_PER_CELL + 1, SRCCOPY)
void ppic(int x, int y){
	switch(board[x][y].st){
	case UNTOUCHED:
		if(state >= LOSE && board[x][y].num == 9){
			if(state == LOSE)DRAW(0, 2);
			else DRAW(3, 2);
        }else{
			DRAW(0, 3);
		}
		break;
	case FLAGGED:
		if(state >= LOSE && board[x][y].num != 9){
			DRAW(2, 2);
		}else{
			DRAW(3, 2);
        }
        break;
	case QUESTION:
		DRAW(3, 3);
		break;
	case TOUCHED:
		switch(board[x][y].num){
        case 0:
			DRAW(2, 3);
            break;
        case 1:
			DRAW(0, 0);
            break;
        case 2:
			DRAW(1, 0);
            break;
        case 3:
			DRAW(2, 0);
            break;
        case 4:
			DRAW(3, 0);
            break;
        case 5:
			DRAW(0, 1);
            break;
        case 6:
			DRAW(1, 1);
            break;
		case 7:
			DRAW(2, 1);
            break;
        case 8:
			DRAW(3, 1);
            break;
        case 9:
			DRAW(1, 2);
			break;
		}
        break;
	}
}
#else
void pchar1(CellInfo c){
	switch(c.st){
	case UNTOUCHED:
		if(state >= LOSE && c.num == 9){
			if(state == LOSE)printf("◎");
			else printf("★");
        }else{
			printf("　");
		}
		break;
	case FLAGGED:
		if(state == LOSE && c.num != 9){
            printf("Ｘ");
		}else{
			printf("★");
        }
        break;
	case QUESTION:
		printf("？");
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
	}
	return;
}
void pchar(CellInfo c){
	switch(c.st){
	case UNTOUCHED:
		if(state >= LOSE && c.num == 9){
			if(state == LOSE)putchar('#');
			else putchar('F');
		}else
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
    case QUESTION:
        putchar('?');
		break;
	}
}
#endif

void judge(){
    int i, j, n;
    if(state >= LOSE)return;
    n = 0;
    for(i = 1; i <= settings.boardheight; i++){
        for(j = 1; j <= settings.boardwidth; j++){
            if(board[i][j].st != TOUCHED)n++;
        }
    }
    if(n == settings.minenum){
		state = WIN;
		for(i = 1; i <= settings.boardheight; i++){
			for(j = 1; j <= settings.boardwidth; j++){
				flag(i, j, 1);
			}
		}
	}
}
