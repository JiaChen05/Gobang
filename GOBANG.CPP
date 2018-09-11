/* 
    對戰五子棋，目前開發僅完成1/3，僅可單機雙打。 
    尚未完成AI功能，以及網路連線對戰功能。 
    悔棋亦尚未完成，實際上不打算設計悔棋功能，因為這樣下棋沒意思。 
    若要設定悔棋，一次僅可退一步。 
    AI決定用陣列儲存已下過的位置，再做判斷。 
    網路功能則沒書好看。 
*/  
#include <afxwin.h>		// 10TCPServer	
#include <afxcmn.h>
#include <windows.h>  
#include "resource.h"  
#include <winsock2.h>	// 为了使用Winsock API函数
#include <stdio.h>


// 告诉连接器与WS2_32库连接
#pragma comment(lib,"WS2_32.lib")
//-----------------------------------------------------------  
#define player1 1   //定義玩家1  
#define player2 2  
#define VS_COM 1    //定義遊戲方式，對電腦  
#define VS_PLAYER 2 //單機對戰  
#define VS_CONNECT 3    //連線對戰  
#define BkColor RGB(0,150,250)  //定義棋盤顏色  
#define board_width 20      //棋盤格寬度  
// 定义网络事件通知消息
#define WM_SOCKET WM_USER + 1

	
//-----------------------------------------------------------  
static int chessman[15][15],    //棋盤陣列，判斷是否已下子  
           who_play,            //判斷現在誰玩  
           play_mode;           //判斷遊戲方式  
static BOOL play=FALSE;               //判斷是否可下子
static BOOL turn;      
static TCHAR *chess[3]={"","黑子","白子"};  
static int m_nClient=0;
SOCKET s_socket;
SOCKET usocket;
int PORT=6660;
int check_win;
//-----------------------------------------------------------  
struct playerchoice     //定義結構儲存棋子的位置  
{  
    int x;  
    int y;  
};  
/* 
struct SetChessman      //定義儲存下棋情況的結構 
{ 
    int index;          //紀錄現在為第幾步棋 
    int set[150][2];    //儲存每一步棋的位置 
    BOOL live_three;    //是否有活三 
    int four_next[2][2];    //有活三則儲存活四座標 
    BOOL dead_four;     //是否有死四 
    int five_next[2];   //有死四則儲存勝利座標 
}; 
*/  
//----------------------------------------------------------  
static playerchoice chess_dir[4];   //儲存方向  
//static SetChessman sc1,sc2;   //宣告結構儲存下棋情況  
//----------------------------------------------------------  
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);//視窗訊息處理程式  
void PaintBoard(HDC,RECT,POINT);//繪製棋盤的函式  
void PaintChessman(HWND,RECT,playerchoice,int);//繪製棋子的函式  
void ResetGame();//重新設定遊戲資訊  
int WhoIsWinner(int,int,int);   //判斷勝負  
int Server(HWND m_hWnd);
int Client(HWND m_hWnd);
int s_Deal(HWND hwnd,WPARAM wParam,LPARAM lParam,RECT rt_chessman);
int SendMSG(HWND hwnd,playerchoice pchoice);
//void PCAI(int,int);//AI函數  
//----------------------------------------------------------  


int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,  
                   LPSTR lpCmdLine,int nShowCmd)  
{  
    static TCHAR AppName[]=TEXT("Gobang");  
    HWND hwnd;  
    MSG msg;  
    WNDCLASS wndclass={
		CS_SAVEBITS,
		WndProc,
		0,
		0,
		hInstance,//偷懶所以把視窗類別寫成這樣  
        LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON)),
		LoadCursor(NULL,IDC_ARROW),  
        (HBRUSH)GetStockObject(BLACK_BRUSH),
		AppName,
		AppName}; 
		
    if(!RegisterClass(&wndclass))  
    {  
        MessageBox(NULL,TEXT("注册视窗失败"),AppName,MB_ICONERROR);  
        return 0;  
    }  

    hwnd=CreateWindowEx(
		WS_EX_TOPMOST,
		AppName,
		"五子棋对战",  
            WS_CAPTION|WS_BORDER|WS_SYSMENU|WS_MINIMIZEBOX,  
            130,
			20,
			450,
			400,  
            NULL,
			LoadMenu(hInstance,MAKEINTRESOURCE(IDR_MENU)),
			hInstance,
			NULL);  
    ShowWindow(hwnd,nShowCmd);  
    UpdateWindow(hwnd);  
    while(GetMessage(&msg,NULL,0,0))  
    {  
        TranslateMessage(&msg);  
        DispatchMessage(&msg);  
    }  
    return msg.wParam;  
}  
LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)  
{  
    HDC hdc;  
    PAINTSTRUCT ps;  
    HMENU hMenu=GetMenu(hwnd); ;  
    static RECT rt_board,//儲存棋盤大小的變數  
                rt_chessman;//儲存棋子大小的變數  
    static int cxClient,cyClient;//儲存顯示區域大小的變數  
    static POINT pt_board;//儲存棋盤格線初始座標的變數  
    //static SOCKET sock;   //宣告socket  
    playerchoice pchoice; 
	double a,b;
    //int i,j;  
    TCHAR text[20];  
    RECT rt_text;  
    switch(msg)  
    {  
    case WM_CREATE: 
        ResetGame();  
		play=FALSE;
        return 0;  
    case WM_SIZE:  
        cxClient=LOWORD(lParam);//取得顯示區域大小  
        cyClient=HIWORD(lParam);  
        SetRect(&rt_board,0,0,310,315);//設定棋盤大小  
        SetRect(&rt_chessman,7,7,24,24);  
        pt_board.x=15;//設定第一調棋盤格線的起始座標位置  
        pt_board.y=15;  
        return 0;  
    case WM_PAINT:  
        hdc=BeginPaint(hwnd,&ps);  
        PaintBoard(hdc,rt_board,pt_board);//繪製棋盤  
        /*for(i=0;i<15;i++)  
        {  
            pchoice.x=i;  
            for(j=0;j<15;j++)  
            {  
                pchoice.y=j;  
                switch(chessman[i][j])  
                {  
                case player1:  
                    PaintChessman(hwnd,rt_chessman,pchoice,player1);  
                    break;  
                case player2:  
                    PaintChessman(hwnd,rt_chessman,pchoice,player2);  
                    break;  
                }  
            }  
        }  
		*/
        EndPaint(hwnd,&ps);  
        return 0;  

	 case WM_SOCKET:
		 s_Deal(hwnd,wParam,lParam,rt_chessman);
		 return 0;


    case WM_LBUTTONDOWN:  
        if(!play)  
        {  
            MessageBox(hwnd,TEXT("请先选择游戏开始^0^"),TEXT("欢迎^0^"),MB_OK|MB_ICONINFORMATION);  
            return 0;  
        }  
		if(!turn)  
        {  
            MessageBox(hwnd,TEXT("请等待对方下棋"),TEXT("欢迎^0^"),MB_OK|MB_ICONINFORMATION);  
            return 0;  
        } 
        pchoice.x=LOWORD(lParam)-pt_board.x;  
        pchoice.y=HIWORD(lParam)-pt_board.y; 
		a=pchoice.x*1.0/board_width;
		b=pchoice.y*1.0/board_width;
        pchoice.x=pchoice.x/board_width;//轉換為棋盤格交線位置  
		if(a-pchoice.x>0.5)
			pchoice.x++;
        pchoice.y=pchoice.y/board_width; 
		if(b-pchoice.y>0.5)
			pchoice.y++;
       
        switch(play_mode)  
        {  
        case VS_COM:  
            if(chessman[pchoice.y][pchoice.x]==0&&(pchoice.x>=0&&pchoice.x<=14)&&(pchoice.y>=0&&pchoice.y<=14))  
            {     
                PaintChessman(hwnd,rt_chessman,pchoice,1);//畫棋子  
                chessman[pchoice.y][pchoice.x]=1;  
                //PCAI(pchoice.y,pchoice.x);//AI  
            }  
            else  
            {   MessageBox(hwnd,TEXT("这边不能下哦..^_^"),TEXT("眼睛脱窗ㄚ^^"),MB_ICONERROR|MB_OK);    }  
            break;  
        case VS_PLAYER:  
            if(chessman[pchoice.y][pchoice.x]==0&&(pchoice.x>=0&&pchoice.x<=14)&&(pchoice.y>=0&&pchoice.y<=14))  
            {     
                PaintChessman(hwnd,rt_chessman,pchoice,who_play);//畫棋子  
                chessman[pchoice.y][pchoice.x]=who_play;  
                check_win=WhoIsWinner(pchoice.y,pchoice.x,who_play);  
                if(check_win==player1)  
                {  
                    MessageBox(hwnd,TEXT("黑子胜利"),TEXT("胜利者"),MB_OK|MB_ICONINFORMATION); 
					EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
					EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
                    play=0;  
                    return 0;  
                }  
                if(check_win==player2)  
                {  
                    MessageBox(hwnd,TEXT("白子胜利"),TEXT("胜利者"),MB_OK|MB_ICONINFORMATION);
					EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
					EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
                    play=0;  
                    return 0;  
                }  
                if(who_play==player1)  
                {   who_play=player2;   }  
                else  
                {   who_play=player1;   }  
            }  
            else  
            {   MessageBox(hwnd,TEXT("这边不能下哦..^_^"),TEXT("眼睛脱窗ㄚ^^"),MB_ICONERROR|MB_OK);    }  
            break;  
        case VS_CONNECT:

			 if(chessman[pchoice.y][pchoice.x]==0&&(pchoice.x>=0&&pchoice.x<=14)&&(pchoice.y>=0&&pchoice.y<=14))  
            {     
                PaintChessman(hwnd,rt_chessman,pchoice,who_play);//畫棋子
				SendMSG(hwnd,pchoice);
                chessman[pchoice.y][pchoice.x]=who_play;  
                check_win=WhoIsWinner(pchoice.y,pchoice.x,who_play);  
                if(check_win==player1)  
                {  
                    MessageBox(hwnd,TEXT("黑子胜利"),TEXT("胜利者"),MB_OK|MB_ICONINFORMATION);  
					EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
					EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
                    play=0;  
                    return 0;  
                }  
                if(check_win==player2)  
                {  
                    MessageBox(hwnd,TEXT("白子胜利"),TEXT("胜利者"),MB_OK|MB_ICONINFORMATION);  
					EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
					EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
                    play=0;  
                    return 0;  
                }
				turn=FALSE;
                
            }  
            else  
            {   MessageBox(hwnd,TEXT("这边不能下喔..^_^"),TEXT("眼睛脱窗ㄚ^^"),MB_ICONERROR|MB_OK);    }  
            break;  
			




            break;  
        }  
        return 0;  
    case WM_MOUSEMOVE:  
        pchoice.x=LOWORD(lParam)-pt_board.x;  
        pchoice.y=HIWORD(lParam)-pt_board.y; 
		a=pchoice.x*1.0/board_width;
		b=pchoice.y*1.0/board_width;
        pchoice.x=pchoice.x/board_width;//轉換為棋盤格交線位置  
		if(a-pchoice.x>0.5)
			pchoice.x++;
        pchoice.y=pchoice.y/board_width; 
		if(b-pchoice.y>0.5)
			pchoice.y++;
        if((pchoice.x<0||pchoice.x>14)||(pchoice.y<0||pchoice.y>14))  
            return 0;  
        SetRect(&rt_text,340,140,410,180);  
        hdc=GetDC(hwnd);  
        SetBkColor(hdc,BkColor);  
        wsprintf(text," ( %2d , %2d ) ",pchoice.x+1,pchoice.y+1);  
        DrawText(hdc,text,-1,&rt_text,DT_CENTER);  
        ReleaseDC(hwnd,hdc);  
        return 0;  
    case WM_DESTROY:  
        PostQuitMessage(0);  
        return 0;  
    case WM_COMMAND:  
        hMenu=GetMenu(hwnd);  
        switch(LOWORD(wParam))  
        {  
        case IDC_DOUBLE:  
            play_mode=VS_PLAYER; 
			InvalidateRect(hwnd,NULL,TRUE);  
            ResetGame();
			if(s_socket != INVALID_SOCKET)
			::closesocket(s_socket);
            if(usocket != INVALID_SOCKET)
			::closesocket(usocket);
			::WSACleanup();
            EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED);  
            //EnableMenuItem(hMenu,IDC_MULTI,MF_GRAYED);  
            EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_DOUBLE,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_SERVER,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_CONNECT,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_CUT,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
            play=TRUE;  
			turn=TRUE;
            return 0;
		case IDC_DOUBLEEND:
			EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED);  
            //EnableMenuItem(hMenu,IDC_MULTI,MF_GRAYED);  
            EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_DOUBLE,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_SERVER,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_CONNECT,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_CUT,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
			play=FALSE;
			turn=FALSE;
			return 0;

        case IDC_RESET:
			if(play_mode==VS_PLAYER)
			{
            EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED);  
            EnableMenuItem(hMenu,IDC_DOUBLE,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_SERVER,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_CONNECT,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_CUT,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
			InvalidateRect(hwnd,NULL,TRUE);  
            ResetGame(); 
			}
			else
			{
			pchoice.x=17;
            pchoice.y=17;
            SendMSG(hwnd,pchoice);
			who_play=player1;
		    EnableMenuItem(hMenu,IDC_RESET,MF_GRAYED);  
            EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_DOUBLE,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_SERVER,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_CONNECT,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_CUT,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_FAIL,MF_ENABLED);
            InvalidateRect(hwnd,NULL,TRUE);  
            ResetGame();
           
			}
            
              
			turn=TRUE;
			play=1;
            return 0;  
       
        case IDC_SERVER:  //等待对方连线
            play_mode=VS_CONNECT; 
			InvalidateRect(hwnd,NULL,TRUE);  
            ResetGame();
			EnableMenuItem(hMenu,IDC_RESET,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_DOUBLE,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_GRAYED);
			if(Server(hwnd)==1)
			{
			play=TRUE;
			who_play=player1;
			turn=TRUE;
			EnableMenuItem(hMenu,IDC_CUT,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_FAIL,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_SERVER,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_CONNECT,MF_GRAYED);
          
		
			}
			else
            MessageBox(hwnd,TEXT("创建服务器失败！"),  
                TEXT("错误"),MB_OK|MB_ICONINFORMATION); 
            return 0;  
        case IDC_CONNECT: // 连线到对方电脑
			 InvalidateRect(hwnd,NULL,TRUE);  
             ResetGame();
             play_mode=VS_CONNECT;
			 EnableMenuItem(hMenu,IDC_RESET,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_DOUBLE,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_GRAYED);
            if(s_socket != INVALID_SOCKET)
			::closesocket(s_socket);			
            if(Client(hwnd)==1)
			{
			play=TRUE;
			who_play=player2;
			turn=FALSE;
			EnableMenuItem(hMenu,IDC_CUT,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_FAIL,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_CONNECT,MF_GRAYED);
			EnableMenuItem(hMenu,IDC_SERVER,MF_GRAYED); 
			hdc=GetDC(hwnd);
			SetRect(&rt_text,340,210,410,230);  
            SetTextColor(hdc,RGB(255,255,255));  
            SetBkColor(hdc,BkColor);  
            DrawText(hdc,"白子",-1,&rt_text,DT_CENTER);  
            
			}
            return 0;  
        case IDC_EXIT:  
            PostMessage(hwnd,WM_CLOSE,wParam,lParam);  
            return 0;  
        case IDC_VISION:  
            MessageBox(hwnd,TEXT("测试版：1.0"),TEXT("版本资讯"),MB_OK|MB_ICONINFORMATION);  
            return 0; 
		case IDC_CUT:
			if(s_socket != INVALID_SOCKET)
			{
			::closesocket(s_socket);
			s_socket = INVALID_SOCKET;
			}
            m_nClient=0;
            if(usocket != INVALID_SOCKET)
			::closesocket(usocket);
			
            MessageBox(hwnd,TEXT("连接已断开"),TEXT(""),MB_OK|MB_ICONINFORMATION);
			EnableMenuItem(hMenu,IDC_SERVER,MF_ENABLED); 
			EnableMenuItem(hMenu,IDC_CONNECT,MF_ENABLED); 
			EnableMenuItem(hMenu,IDC_CUT,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_DOUBLE,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_GRAYED);
            EnableMenuItem(hMenu,IDC_RESET,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
			play=FALSE;
            ::WSACleanup();
			return 0;
		case IDC_FAIL:
			pchoice.x=18;
            pchoice.y=18;
			play=0;
            SendMSG(hwnd,pchoice);
			if(who_play==player1)
            MessageBox(hwnd,TEXT("黑子认输，白子胜利"),TEXT("胜利者"),MB_OK|MB_ICONINFORMATION);
			else
            MessageBox(hwnd,TEXT("白子认输，黑子胜利"),TEXT("胜利者"),MB_OK|MB_ICONINFORMATION);
			EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
			EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_SERVER,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_CONNECT,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_CUT,MF_ENABLED); 
			EnableMenuItem(hMenu,IDC_DOUBLE,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_GRAYED);
			return 0;

        }  
        return 0;  
    }  
    return DefWindowProc(hwnd,msg,wParam,lParam);  
}  
void PaintBoard(HDC hdc,RECT rt,POINT pt)//繪製棋盤  
{  
    HBRUSH hbr;  
    RECT rt2; 
	RECT rt_text1; 
    int i;  
    TCHAR *szText[5]={"黑子先下","现在为","鼠标",  
        "我是","本机IP"};  
    hbr=CreateSolidBrush(RGB(255,230,150));//定義棋盤使用畫刷顏色  
    FillRect(hdc,&rt,hbr);//繪製棋盤  
    DeleteObject(hbr);  
    hbr=CreateSolidBrush(BkColor);  
    SelectObject(hdc,hbr);  
    SetBkColor(hdc,BkColor);  
    SetTextColor(hdc,RGB(0,0,0));  
    SetRect(&rt2,330,6,420,30);  
    RoundRect(hdc,rt2.left,rt2.top,rt2.right,rt2.bottom,30,30);  
    //FillRect(hdc,&rt2,hbr);  
    DrawText(hdc,szText[0],-1,&rt2,DT_CENTER);  
    SetTextColor(hdc,RGB(255,255,255));  
    for(i=1;i<5;i++)  
    {  
        SetRect(&rt2,330,70*i-25,420,30+70*i);  
        RoundRect(hdc,rt2.left,rt2.top,rt2.right,rt2.bottom,70,70);  
        //FillRect(hdc,&rt2,hbr);  
        SetRect(&rt2,330,70*i-20,420,30+70*i);  
        DrawText(hdc,szText[i],-1,&rt2,DT_CENTER);  
    }  
    DeleteObject(hbr);  
    hbr=CreateSolidBrush(RGB(0,0,0));//定義棋盤使用畫刷顏色  
    SelectObject(hdc,hbr);  
    for(i=0;i<9;i++)//畫棋盤的九個點  
    {  
        SetRect(&rt2,(72+board_width*(i%3))+60*(i%3),(72+board_width*(i/3))+60*(i/3),(72+board_width*(i%3))+7+60*(i%3),(72+board_width*(i/3))+7+60*(i/3));  
        Ellipse(hdc,rt2.left,rt2.top,rt2.right,rt2.bottom);  
    }  
    DeleteObject(hbr);  
    for(i=0;i<15;i++)//畫橫線  
    {  
        MoveToEx(hdc,pt.x,pt.y+board_width*i,NULL);  
        LineTo(hdc,rt.right-14,pt.y+board_width*i);  
    }  
    for(i=0;i<15;i++)//畫縱線  
    {  
        MoveToEx(hdc,pt.x+board_width*i,pt.y,NULL);  
        LineTo(hdc,pt.x+board_width*i,rt.bottom-20);  
    }  
	
	if(play){

   if(who_play==player1)
   {
	 
	SetRect(&rt_text1,340,210,410,230);  
        SetTextColor(hdc,RGB(0,0,0));  
              SetBkColor(hdc,BkColor);  
              DrawText(hdc,"黑子",-1,&rt_text1,DT_CENTER); 
   }
   else
   {

			//	hdc=GetDC(hwnd);
			SetRect(&rt_text1,340,210,410,230);  
            SetTextColor(hdc,RGB(255,255,255));  
            SetBkColor(hdc,BkColor);  
            DrawText(hdc,"白子",-1,&rt_text1,DT_CENTER); 
	}
	}
}  


void PaintChessman(HWND hwnd,RECT rt,playerchoice pt,int who)//畫棋子  
{  
    HDC hdc;  
    HBRUSH hbr;  
    hdc=GetDC(hwnd);  
    RECT rt_text;  
    if(who==player1)  
    {  
        SetRect(&rt_text,340,70,410,110);  
        SetTextColor(hdc,RGB(255,255,255));  
        SetBkColor(hdc,BkColor);  
        DrawText(hdc,"白子",-1,&rt_text,DT_CENTER);  
        hbr=CreateSolidBrush(RGB(0,0,0));//黑棋  
        SelectObject(hdc,hbr);  
    }  
    else  
    {  
        SetRect(&rt_text,340,70,410,110);  
        SetTextColor(hdc,RGB(0,0,0));  
        SetBkColor(hdc,BkColor);  
        DrawText(hdc,"黑子",-1,&rt_text,DT_CENTER);  
        hbr=CreateSolidBrush(RGB(255,255,255));//白棋  
        SelectObject(hdc,hbr);  
    }  

		

    Ellipse(hdc,rt.left+board_width*pt.x,rt.top+board_width*pt.y,         
                rt.right+board_width*pt.x,rt.bottom+board_width*pt.y);  
    DeleteObject(hbr);  
    ReleaseDC(hwnd,hdc);  
}  
void ResetGame()    //初始化遊戲資料  
{  
    int i,j;  
    play=TRUE;  
    who_play=player1;  
    for(i=0;i<15;i++)  
            for(j=0;j<15;j++)  
                chessman[i][j]=0;//陣列設為0，表示尚未放置棋子  
    chess_dir[0].x=1;  
    chess_dir[0].y=0;  
    chess_dir[1].x=1;  
    chess_dir[1].y=1;  
    chess_dir[2].x=0;  
    chess_dir[2].y=1;  
    chess_dir[3].x=-1;  
    chess_dir[3].y=1;  
    /*sc1.index=0; 
    sc1.live_three=FALSE; 
    sc1.dead_four=FALSE; 
    sc2.index=0; 
    sc2.live_three=FALSE; 
    sc2.dead_four=FALSE;*/  
}  
int WhoIsWinner(int i,int j,int who)//判斷勝負  
{  
    int x,LTable,RTable;  
    playerchoice dir;  
    for(x=0;x<4;x++)  
    {  
        LTable=0;  
        RTable=0;  
        dir.x=i;  
        dir.y=j;  
        while(chessman[i][j]==chessman[dir.x][dir.y]&&chessman[i][j]==who)  
        {  
            LTable++;  
            dir.x=dir.x+chess_dir[x].x;  
            dir.y=dir.y+chess_dir[x].y;  
            if(dir.x<0||dir.y<0||dir.x>14||dir.y>14)  
                break;  
        }  
        dir.x=i;  
        dir.y=j;  
        while(chessman[i][j]==chessman[dir.x][dir.y]&&chessman[i][j]==who)  
        {  
            RTable++;  
            dir.x=dir.x-chess_dir[x].x;  
            dir.y=dir.y-chess_dir[x].y;  
            if(dir.x<0||dir.y<0||dir.x>14||dir.y>14)  
                break;  
        }  
        if(LTable+RTable>5)  
        {  
            return who;  
        }  
    }  
    return 0;  
}  
void PCAI(int pi,int pj)//電腦AI，傳入參數為玩家最後下的位置  
{  
}  

int Server(HWND m_hWnd)
{
	// 初始化WS2_32.dll
	WSADATA wsaData;
    //WORD sockVersion = MAKEWORD( 1, 1 );
	WORD sockVersion = MAKEWORD(2, 0);
	::WSAStartup(sockVersion, &wsaData);
	
	// 创建套节字
	//s_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP);
	s_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s_socket == INVALID_SOCKET)
	{
		//printf("Failed socket() \n");
		MessageBox(m_hWnd,TEXT("Failed socket()"),TEXT("\n"),MB_OK|MB_ICONINFORMATION);
		::WSACleanup();
		return 0;
	}
	
	// 填充sockaddr_in结构
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	
	// 绑定这个套节字到一个本地地址
	if(::bind(s_socket, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf("Failed bind() \n");
		::WSACleanup();
		return 0;
	}
	// 设置socket为窗口通知消息类型
	::WSAAsyncSelect(s_socket, m_hWnd, WM_SOCKET, FD_ACCEPT|FD_CLOSE);
	
	//监听
	if(::listen(s_socket, 2) == SOCKET_ERROR)
	{
		printf("Failed listen()");
		::WSACleanup();
		return 0;
	}
    //MessageBox(m_hWnd,TEXT("正在监听lal"),TEXT(":"),MB_OK|MB_ICONINFORMATION);

		// 下面是取得本地IP地址的过程，将它显示在状态栏的第一个分栏中
	// 取得本机名称	
	char szHost[256];
	::gethostname(szHost, 256);
	// 通过本机名称取得地址信息
	HOSTENT* pHost = gethostbyname(szHost);
	if(pHost != NULL)
	{  	
		CString sIP;
		
		// 得到第一个IP地址
		in_addr *addr =(in_addr*) *(pHost->h_addr_list);
		
		// 显示给用户
		sIP.Format(" 本机IP：%s", inet_ntoa(addr[0]));
		//m_bar.SetText(sIP, 0, 0);
		//MessageBox(m_hWnd,TEXT(sIP),TEXT("正在监听lal"),MB_OK|MB_ICONINFORMATION);
	}
	  
		
	
	return 1;
}

int Client(HWND m_hWnd)
{

// 初始化WS2_32.dll
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 0);
	::WSAStartup(sockVersion, &wsaData);
	
	// 创建套节字
	usocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(usocket == INVALID_SOCKET)
	{
		//printf("Failed socket() \n");
		MessageBox(m_hWnd,TEXT("Failed socket()"),TEXT("\n"),MB_OK|MB_ICONINFORMATION);
		::WSACleanup();
		return 0;
	}

	// 也可以在这里调用bind函数绑定一个本地地址
	// 否则系统将会自动安排

	// 填写远程地址信息
	sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(PORT);
	// 注意，这里要填写服务器程序（10ServerDemo程序）所在机器的IP地址
	// 如果你的计算机没有联网，直接使用127.0.0.1 10.128.50.39即可
	servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	

	if(::connect(usocket, (sockaddr*)&servAddr, sizeof(servAddr)) == -1)
	{
		MessageBox(m_hWnd,TEXT("Failed connect() "),TEXT("\n"),MB_OK|MB_ICONINFORMATION);	
		::WSACleanup();
		return 0;
	}
	MessageBox(m_hWnd,TEXT("已连接"),TEXT("五子棋对战 "),MB_OK|MB_ICONINFORMATION);
    
	::WSAAsyncSelect(usocket, m_hWnd,
      WM_SOCKET, FD_CONNECT | FD_CLOSE | FD_WRITE | FD_READ);

	return 1;

	
}

int s_Deal(HWND hwnd,WPARAM wParam,LPARAM lParam,RECT rt_chessman){
	HMENU hMenu=GetMenu(hwnd);
		// 取得有事件发生的套节字句柄
	SOCKET s1 = wParam;
    sockaddr_in remoteAddr; 
	int nAddrLen = sizeof(remoteAddr);
	playerchoice pch;
	HDC hdc;  
    RECT rt_text;
	
	// 查看是否出错LAL
	if(WSAGETSELECTERROR(lParam))
	{
		//RemoveClient(s);
		::closesocket(s1);
		return 0;
	}
	// 处理发生的事件
	switch(WSAGETSELECTEVENT(lParam))
	{
	case FD_ACCEPT:		// 监听中的套接字检测到有连接进入
		{
			if(m_nClient < 1)
			{
				// 接受连接请求，新的套节字client是新连接的套节字
			
				//SOCKET client = ::accept(s1, (SOCKADDR*)&remoteAddr, &nAddrLen);
				usocket=::accept(s1, (SOCKADDR*)&remoteAddr, &nAddrLen);
				// 设置新的套节字为窗口通知消息类型
				int i = ::WSAAsyncSelect(usocket, 
					hwnd, WM_SOCKET, FD_READ|FD_WRITE|FD_CLOSE);
				//AddClient(client);
				if(usocket == INVALID_SOCKET)
				{
		        	//printf("Failed accept()");
					MessageBox(hwnd,TEXT("Failed accept()"),TEXT("欢迎^0^"),MB_OK|MB_ICONINFORMATION);
				}
                else{
                CString rIP;
				rIP.Format("接受到一个连接：%s \r\n", inet_ntoa(remoteAddr.sin_addr));
		        MessageBox(hwnd,TEXT(rIP),TEXT("欢迎^0^"),MB_OK|MB_ICONINFORMATION);
	         	//printf(" 接受到一个连接：%s \r\n", inet_ntoa(remoteAddr.sin_addr));
				}
				m_nClient++;
				hdc=GetDC(hwnd);
			   SetRect(&rt_text,340,210,410,230);  
               SetTextColor(hdc,RGB(0,0,0));  
                SetBkColor(hdc,BkColor);  
                DrawText(hdc,"黑子",-1,&rt_text,DT_CENTER);  

			}
			else
			{
				MessageBox(hwnd,TEXT("连接客户太多！"),TEXT("欢迎^0^"),MB_OK|MB_ICONINFORMATION);
				//MessageBox("连接客户太多！");
			}
		}
		break;

	case FD_CLOSE:		// 检测到套接字对应的连接被关闭。
		{
			if(s_socket != INVALID_SOCKET)
			{
			::closesocket(s_socket);
			s_socket = INVALID_SOCKET;
			}
            m_nClient=0;
			if(usocket!=INVALID_SOCKET)
			::closesocket(usocket);
			::WSACleanup();
			play=FALSE;
			MessageBox(hwnd,TEXT("对方已断开连接.."),TEXT("五子棋对战"),MB_OK|MB_ICONINFORMATION);
			EnableMenuItem(hMenu,IDC_SERVER,MF_ENABLED); 
			EnableMenuItem(hMenu,IDC_CONNECT,MF_ENABLED); 
			EnableMenuItem(hMenu,IDC_CUT,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_DOUBLE,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_GRAYED);
            EnableMenuItem(hMenu,IDC_RESET,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
		}
		break;

	case FD_READ:		// 套接字接受到对方发送过来的数据包
		{
             
			 	char buff[10];
            	int nRecv1 = ::recv(usocket, buff, 10, 0);
				//int nRecv2 = ::recv(usocket, buff2, 2, 0);
				//if(!play) return 0;
			   if(buff[1]=='.')
			   {
				   pch.x=(int)buff[0]-48;
				   if(buff[3]=='.')
					   pch.y=(int)buff[2]-48;
				   else
					   pch.y=10+(int)buff[3]-48;
			   }
				else
				{
                   pch.x=10+(int)buff[1]-48;
				   if(buff[4]=='.')
					   pch.y=(int)buff[3]-48;
				   else
					   pch.y=10+(int)buff[4]-48;
			   }
               if(pch.x==18&&pch.y==18) //对方认输
			   {       
				   if(who_play==player1)
                   MessageBox(hwnd,TEXT("白子认输，黑子胜利"),TEXT("胜利者"),MB_OK|MB_ICONINFORMATION);
				   else
                   MessageBox(hwnd,TEXT("黑子认输，白子胜利"),TEXT("胜利者"),MB_OK|MB_ICONINFORMATION);
				   EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED); 
				   EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
				   EnableMenuItem(hMenu,IDC_SERVER,MF_GRAYED); 
				   EnableMenuItem(hMenu,IDC_CONNECT,MF_GRAYED); 
				   EnableMenuItem(hMenu,IDC_CUT,MF_ENABLED); 
				   EnableMenuItem(hMenu,IDC_DOUBLE,MF_ENABLED);
				   EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_GRAYED);
				   play=0;  
                   return 0; 
			   }
			   if(pch.x==17&&pch.y==17) //对方请求再来一局
			   {
                   
                EnableMenuItem(hMenu,IDC_RESET,MF_GRAYED);  
				EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_GRAYED);
				EnableMenuItem(hMenu,IDC_DOUBLE,MF_GRAYED);
				EnableMenuItem(hMenu,IDC_SERVER,MF_GRAYED);
				EnableMenuItem(hMenu,IDC_CONNECT,MF_GRAYED);
				EnableMenuItem(hMenu,IDC_CUT,MF_ENABLED);
				EnableMenuItem(hMenu,IDC_FAIL,MF_ENABLED);
				InvalidateRect(hwnd,NULL,TRUE);  
				ResetGame();
				play=TRUE;
				who_play=player2;
                turn=FALSE;
			
				return 0;  

			   }

                if(who_play==player1)  
                {   who_play=player2;   }  
                else  
                {   who_play=player1;   } 
				
				PaintChessman(hwnd,rt_chessman,pch,who_play);
				chessman[pch.y][pch.x]=who_play;  
                check_win=WhoIsWinner(pch.y,pch.x,who_play);  
                if(check_win==player1)  
                {  
                    MessageBox(hwnd,TEXT("黑子胜利"),TEXT("胜利者"),MB_OK|MB_ICONINFORMATION); 
					EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
					EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
                    play=0;  
                    return 0;  
                }  
                if(check_win==player2)  
                {  
                    MessageBox(hwnd,TEXT("白子胜利"),TEXT("胜利者"),MB_OK|MB_ICONINFORMATION);  
					EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
					EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
                    play=0;  
                    return 0;  
                } 
				if(who_play==player1)  
                {   who_play=player2;   }  
                else  
                {   who_play=player1;   }   
                turn=TRUE;

			

		}
		break;
	}
	return 0;
}
 

int SendMSG(HWND hwnd,playerchoice pchoice)
{
	char sx[6],sy[5];
    itoa(pchoice.x,sx,10);
	itoa(pchoice.y,sy,10);
    strcat(sx,".");
    strcat(sx,sy);
	strcat(sx,".");
	if((::send(usocket, sx, strlen(sx), 0) != -1))
	{
		//MessageBox(hwnd,TEXT("发送成"),TEXT("歡迎光臨^0^"),MB_OK|MB_ICONINFORMATION);
	return 1;
	}

	return 0;

}