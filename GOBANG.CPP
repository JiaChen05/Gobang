/* 
    ���������壬Ŀǰ�_�l�H���1/3���H�ɆΙC�p�� 
    ��δ���AI���ܣ��Լ��W·�B�������ܡ� 
    ��������δ��ɣ����H�ϲ������OӋ���幦�ܣ�����@������]��˼�� 
    ��Ҫ�O�����壬һ�΃H����һ���� 
    AI�Q������Ѓ��������^��λ�ã������Дࡣ 
    �W·���܄t�]���ÿ��� 
*/  
#include <afxwin.h>		// 10TCPServer	
#include <afxcmn.h>
#include <windows.h>  
#include "resource.h"  
#include <winsock2.h>	// Ϊ��ʹ��Winsock API����
#include <stdio.h>


// ������������WS2_32������
#pragma comment(lib,"WS2_32.lib")
//-----------------------------------------------------------  
#define player1 1   //���x���1  
#define player2 2  
#define VS_COM 1    //���x�[��ʽ������X  
#define VS_PLAYER 2 //�ΙC����  
#define VS_CONNECT 3    //�B������  
#define BkColor RGB(0,150,250)  //���x��P�ɫ  
#define board_width 20      //��P�񌒶�  
// ���������¼�֪ͨ��Ϣ
#define WM_SOCKET WM_USER + 1

	
//-----------------------------------------------------------  
static int chessman[15][15],    //��P��У��Д��Ƿ�������  
           who_play,            //�Д�F���l��  
           play_mode;           //�Д��[��ʽ  
static BOOL play=FALSE;               //�Д��Ƿ������
static BOOL turn;      
static TCHAR *chess[3]={"","����","����"};  
static int m_nClient=0;
SOCKET s_socket;
SOCKET usocket;
int PORT=6660;
int check_win;
//-----------------------------------------------------------  
struct playerchoice     //���x�Y���������ӵ�λ��  
{  
    int x;  
    int y;  
};  
/* 
struct SetChessman      //���x����������r�ĽY�� 
{ 
    int index;          //�o䛬F�ڞ�ڎײ��� 
    int set[150][2];    //����ÿһ�����λ�� 
    BOOL live_three;    //�Ƿ��л��� 
    int four_next[2][2];    //�л����t����������� 
    BOOL dead_four;     //�Ƿ������� 
    int five_next[2];   //�����Ąt����������� 
}; 
*/  
//----------------------------------------------------------  
static playerchoice chess_dir[4];   //���淽��  
//static SetChessman sc1,sc2;   //����Y������������r  
//----------------------------------------------------------  
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);//ҕ��ӍϢ̎����ʽ  
void PaintBoard(HDC,RECT,POINT);//�L�u��P�ĺ�ʽ  
void PaintChessman(HWND,RECT,playerchoice,int);//�L�u���ӵĺ�ʽ  
void ResetGame();//�����O���[���YӍ  
int WhoIsWinner(int,int,int);   //�Д���ؓ  
int Server(HWND m_hWnd);
int Client(HWND m_hWnd);
int s_Deal(HWND hwnd,WPARAM wParam,LPARAM lParam,RECT rt_chessman);
int SendMSG(HWND hwnd,playerchoice pchoice);
//void PCAI(int,int);//AI����  
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
		hInstance,//͵�����԰�ҕ��e�����@��  
        LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON)),
		LoadCursor(NULL,IDC_ARROW),  
        (HBRUSH)GetStockObject(BLACK_BRUSH),
		AppName,
		AppName}; 
		
    if(!RegisterClass(&wndclass))  
    {  
        MessageBox(NULL,TEXT("ע���Ӵ�ʧ��"),AppName,MB_ICONERROR);  
        return 0;  
    }  

    hwnd=CreateWindowEx(
		WS_EX_TOPMOST,
		AppName,
		"�������ս",  
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
    static RECT rt_board,//������P��С��׃��  
                rt_chessman;//�������Ӵ�С��׃��  
    static int cxClient,cyClient;//�����@ʾ�^���С��׃��  
    static POINT pt_board;//������P�񾀳�ʼ���˵�׃��  
    //static SOCKET sock;   //����socket  
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
        cxClient=LOWORD(lParam);//ȡ���@ʾ�^���С  
        cyClient=HIWORD(lParam);  
        SetRect(&rt_board,0,0,310,315);//�O����P��С  
        SetRect(&rt_chessman,7,7,24,24);  
        pt_board.x=15;//�O����һ�{��P�񾀵���ʼ����λ��  
        pt_board.y=15;  
        return 0;  
    case WM_PAINT:  
        hdc=BeginPaint(hwnd,&ps);  
        PaintBoard(hdc,rt_board,pt_board);//�L�u��P  
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
            MessageBox(hwnd,TEXT("����ѡ����Ϸ��ʼ^0^"),TEXT("��ӭ^0^"),MB_OK|MB_ICONINFORMATION);  
            return 0;  
        }  
		if(!turn)  
        {  
            MessageBox(hwnd,TEXT("��ȴ��Է�����"),TEXT("��ӭ^0^"),MB_OK|MB_ICONINFORMATION);  
            return 0;  
        } 
        pchoice.x=LOWORD(lParam)-pt_board.x;  
        pchoice.y=HIWORD(lParam)-pt_board.y; 
		a=pchoice.x*1.0/board_width;
		b=pchoice.y*1.0/board_width;
        pchoice.x=pchoice.x/board_width;//�D�Q����P�񽻾�λ��  
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
                PaintChessman(hwnd,rt_chessman,pchoice,1);//������  
                chessman[pchoice.y][pchoice.x]=1;  
                //PCAI(pchoice.y,pchoice.x);//AI  
            }  
            else  
            {   MessageBox(hwnd,TEXT("��߲�����Ŷ..^_^"),TEXT("�۾��Ѵ���^^"),MB_ICONERROR|MB_OK);    }  
            break;  
        case VS_PLAYER:  
            if(chessman[pchoice.y][pchoice.x]==0&&(pchoice.x>=0&&pchoice.x<=14)&&(pchoice.y>=0&&pchoice.y<=14))  
            {     
                PaintChessman(hwnd,rt_chessman,pchoice,who_play);//������  
                chessman[pchoice.y][pchoice.x]=who_play;  
                check_win=WhoIsWinner(pchoice.y,pchoice.x,who_play);  
                if(check_win==player1)  
                {  
                    MessageBox(hwnd,TEXT("����ʤ��"),TEXT("ʤ����"),MB_OK|MB_ICONINFORMATION); 
					EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
					EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
                    play=0;  
                    return 0;  
                }  
                if(check_win==player2)  
                {  
                    MessageBox(hwnd,TEXT("����ʤ��"),TEXT("ʤ����"),MB_OK|MB_ICONINFORMATION);
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
            {   MessageBox(hwnd,TEXT("��߲�����Ŷ..^_^"),TEXT("�۾��Ѵ���^^"),MB_ICONERROR|MB_OK);    }  
            break;  
        case VS_CONNECT:

			 if(chessman[pchoice.y][pchoice.x]==0&&(pchoice.x>=0&&pchoice.x<=14)&&(pchoice.y>=0&&pchoice.y<=14))  
            {     
                PaintChessman(hwnd,rt_chessman,pchoice,who_play);//������
				SendMSG(hwnd,pchoice);
                chessman[pchoice.y][pchoice.x]=who_play;  
                check_win=WhoIsWinner(pchoice.y,pchoice.x,who_play);  
                if(check_win==player1)  
                {  
                    MessageBox(hwnd,TEXT("����ʤ��"),TEXT("ʤ����"),MB_OK|MB_ICONINFORMATION);  
					EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
					EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
                    play=0;  
                    return 0;  
                }  
                if(check_win==player2)  
                {  
                    MessageBox(hwnd,TEXT("����ʤ��"),TEXT("ʤ����"),MB_OK|MB_ICONINFORMATION);  
					EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
					EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
                    play=0;  
                    return 0;  
                }
				turn=FALSE;
                
            }  
            else  
            {   MessageBox(hwnd,TEXT("��߲������..^_^"),TEXT("�۾��Ѵ���^^"),MB_ICONERROR|MB_OK);    }  
            break;  
			




            break;  
        }  
        return 0;  
    case WM_MOUSEMOVE:  
        pchoice.x=LOWORD(lParam)-pt_board.x;  
        pchoice.y=HIWORD(lParam)-pt_board.y; 
		a=pchoice.x*1.0/board_width;
		b=pchoice.y*1.0/board_width;
        pchoice.x=pchoice.x/board_width;//�D�Q����P�񽻾�λ��  
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
       
        case IDC_SERVER:  //�ȴ��Է�����
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
            MessageBox(hwnd,TEXT("����������ʧ�ܣ�"),  
                TEXT("����"),MB_OK|MB_ICONINFORMATION); 
            return 0;  
        case IDC_CONNECT: // ���ߵ��Է�����
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
            DrawText(hdc,"����",-1,&rt_text,DT_CENTER);  
            
			}
            return 0;  
        case IDC_EXIT:  
            PostMessage(hwnd,WM_CLOSE,wParam,lParam);  
            return 0;  
        case IDC_VISION:  
            MessageBox(hwnd,TEXT("���԰棺1.0"),TEXT("�汾��Ѷ"),MB_OK|MB_ICONINFORMATION);  
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
			
            MessageBox(hwnd,TEXT("�����ѶϿ�"),TEXT(""),MB_OK|MB_ICONINFORMATION);
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
            MessageBox(hwnd,TEXT("�������䣬����ʤ��"),TEXT("ʤ����"),MB_OK|MB_ICONINFORMATION);
			else
            MessageBox(hwnd,TEXT("�������䣬����ʤ��"),TEXT("ʤ����"),MB_OK|MB_ICONINFORMATION);
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
void PaintBoard(HDC hdc,RECT rt,POINT pt)//�L�u��P  
{  
    HBRUSH hbr;  
    RECT rt2; 
	RECT rt_text1; 
    int i;  
    TCHAR *szText[5]={"��������","����Ϊ","���",  
        "����","����IP"};  
    hbr=CreateSolidBrush(RGB(255,230,150));//���x��Pʹ�î�ˢ�ɫ  
    FillRect(hdc,&rt,hbr);//�L�u��P  
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
    hbr=CreateSolidBrush(RGB(0,0,0));//���x��Pʹ�î�ˢ�ɫ  
    SelectObject(hdc,hbr);  
    for(i=0;i<9;i++)//����P�ľł��c  
    {  
        SetRect(&rt2,(72+board_width*(i%3))+60*(i%3),(72+board_width*(i/3))+60*(i/3),(72+board_width*(i%3))+7+60*(i%3),(72+board_width*(i/3))+7+60*(i/3));  
        Ellipse(hdc,rt2.left,rt2.top,rt2.right,rt2.bottom);  
    }  
    DeleteObject(hbr);  
    for(i=0;i<15;i++)//���M��  
    {  
        MoveToEx(hdc,pt.x,pt.y+board_width*i,NULL);  
        LineTo(hdc,rt.right-14,pt.y+board_width*i);  
    }  
    for(i=0;i<15;i++)//���v��  
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
              DrawText(hdc,"����",-1,&rt_text1,DT_CENTER); 
   }
   else
   {

			//	hdc=GetDC(hwnd);
			SetRect(&rt_text1,340,210,410,230);  
            SetTextColor(hdc,RGB(255,255,255));  
            SetBkColor(hdc,BkColor);  
            DrawText(hdc,"����",-1,&rt_text1,DT_CENTER); 
	}
	}
}  


void PaintChessman(HWND hwnd,RECT rt,playerchoice pt,int who)//������  
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
        DrawText(hdc,"����",-1,&rt_text,DT_CENTER);  
        hbr=CreateSolidBrush(RGB(0,0,0));//����  
        SelectObject(hdc,hbr);  
    }  
    else  
    {  
        SetRect(&rt_text,340,70,410,110);  
        SetTextColor(hdc,RGB(0,0,0));  
        SetBkColor(hdc,BkColor);  
        DrawText(hdc,"����",-1,&rt_text,DT_CENTER);  
        hbr=CreateSolidBrush(RGB(255,255,255));//����  
        SelectObject(hdc,hbr);  
    }  

		

    Ellipse(hdc,rt.left+board_width*pt.x,rt.top+board_width*pt.y,         
                rt.right+board_width*pt.x,rt.bottom+board_width*pt.y);  
    DeleteObject(hbr);  
    ReleaseDC(hwnd,hdc);  
}  
void ResetGame()    //��ʼ���[���Y��  
{  
    int i,j;  
    play=TRUE;  
    who_play=player1;  
    for(i=0;i<15;i++)  
            for(j=0;j<15;j++)  
                chessman[i][j]=0;//����O��0����ʾ��δ��������  
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
int WhoIsWinner(int i,int j,int who)//�Д���ؓ  
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
void PCAI(int pi,int pj)//��XAI�����녢������������µ�λ��  
{  
}  

int Server(HWND m_hWnd)
{
	// ��ʼ��WS2_32.dll
	WSADATA wsaData;
    //WORD sockVersion = MAKEWORD( 1, 1 );
	WORD sockVersion = MAKEWORD(2, 0);
	::WSAStartup(sockVersion, &wsaData);
	
	// �����׽���
	//s_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP);
	s_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s_socket == INVALID_SOCKET)
	{
		//printf("Failed socket() \n");
		MessageBox(m_hWnd,TEXT("Failed socket()"),TEXT("\n"),MB_OK|MB_ICONINFORMATION);
		::WSACleanup();
		return 0;
	}
	
	// ���sockaddr_in�ṹ
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	
	// ������׽��ֵ�һ�����ص�ַ
	if(::bind(s_socket, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf("Failed bind() \n");
		::WSACleanup();
		return 0;
	}
	// ����socketΪ����֪ͨ��Ϣ����
	::WSAAsyncSelect(s_socket, m_hWnd, WM_SOCKET, FD_ACCEPT|FD_CLOSE);
	
	//����
	if(::listen(s_socket, 2) == SOCKET_ERROR)
	{
		printf("Failed listen()");
		::WSACleanup();
		return 0;
	}
    //MessageBox(m_hWnd,TEXT("���ڼ���lal"),TEXT(":"),MB_OK|MB_ICONINFORMATION);

		// ������ȡ�ñ���IP��ַ�Ĺ��̣�������ʾ��״̬���ĵ�һ��������
	// ȡ�ñ�������	
	char szHost[256];
	::gethostname(szHost, 256);
	// ͨ����������ȡ�õ�ַ��Ϣ
	HOSTENT* pHost = gethostbyname(szHost);
	if(pHost != NULL)
	{  	
		CString sIP;
		
		// �õ���һ��IP��ַ
		in_addr *addr =(in_addr*) *(pHost->h_addr_list);
		
		// ��ʾ���û�
		sIP.Format(" ����IP��%s", inet_ntoa(addr[0]));
		//m_bar.SetText(sIP, 0, 0);
		//MessageBox(m_hWnd,TEXT(sIP),TEXT("���ڼ���lal"),MB_OK|MB_ICONINFORMATION);
	}
	  
		
	
	return 1;
}

int Client(HWND m_hWnd)
{

// ��ʼ��WS2_32.dll
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 0);
	::WSAStartup(sockVersion, &wsaData);
	
	// �����׽���
	usocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(usocket == INVALID_SOCKET)
	{
		//printf("Failed socket() \n");
		MessageBox(m_hWnd,TEXT("Failed socket()"),TEXT("\n"),MB_OK|MB_ICONINFORMATION);
		::WSACleanup();
		return 0;
	}

	// Ҳ�������������bind������һ�����ص�ַ
	// ����ϵͳ�����Զ�����

	// ��дԶ�̵�ַ��Ϣ
	sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(PORT);
	// ע�⣬����Ҫ��д����������10ServerDemo�������ڻ�����IP��ַ
	// �����ļ����û��������ֱ��ʹ��127.0.0.1 10.128.50.39����
	servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	

	if(::connect(usocket, (sockaddr*)&servAddr, sizeof(servAddr)) == -1)
	{
		MessageBox(m_hWnd,TEXT("Failed connect() "),TEXT("\n"),MB_OK|MB_ICONINFORMATION);	
		::WSACleanup();
		return 0;
	}
	MessageBox(m_hWnd,TEXT("������"),TEXT("�������ս "),MB_OK|MB_ICONINFORMATION);
    
	::WSAAsyncSelect(usocket, m_hWnd,
      WM_SOCKET, FD_CONNECT | FD_CLOSE | FD_WRITE | FD_READ);

	return 1;

	
}

int s_Deal(HWND hwnd,WPARAM wParam,LPARAM lParam,RECT rt_chessman){
	HMENU hMenu=GetMenu(hwnd);
		// ȡ�����¼��������׽��־��
	SOCKET s1 = wParam;
    sockaddr_in remoteAddr; 
	int nAddrLen = sizeof(remoteAddr);
	playerchoice pch;
	HDC hdc;  
    RECT rt_text;
	
	// �鿴�Ƿ����LAL
	if(WSAGETSELECTERROR(lParam))
	{
		//RemoveClient(s);
		::closesocket(s1);
		return 0;
	}
	// �����������¼�
	switch(WSAGETSELECTEVENT(lParam))
	{
	case FD_ACCEPT:		// �����е��׽��ּ�⵽�����ӽ���
		{
			if(m_nClient < 1)
			{
				// �������������µ��׽���client�������ӵ��׽���
			
				//SOCKET client = ::accept(s1, (SOCKADDR*)&remoteAddr, &nAddrLen);
				usocket=::accept(s1, (SOCKADDR*)&remoteAddr, &nAddrLen);
				// �����µ��׽���Ϊ����֪ͨ��Ϣ����
				int i = ::WSAAsyncSelect(usocket, 
					hwnd, WM_SOCKET, FD_READ|FD_WRITE|FD_CLOSE);
				//AddClient(client);
				if(usocket == INVALID_SOCKET)
				{
		        	//printf("Failed accept()");
					MessageBox(hwnd,TEXT("Failed accept()"),TEXT("��ӭ^0^"),MB_OK|MB_ICONINFORMATION);
				}
                else{
                CString rIP;
				rIP.Format("���ܵ�һ�����ӣ�%s \r\n", inet_ntoa(remoteAddr.sin_addr));
		        MessageBox(hwnd,TEXT(rIP),TEXT("��ӭ^0^"),MB_OK|MB_ICONINFORMATION);
	         	//printf(" ���ܵ�һ�����ӣ�%s \r\n", inet_ntoa(remoteAddr.sin_addr));
				}
				m_nClient++;
				hdc=GetDC(hwnd);
			   SetRect(&rt_text,340,210,410,230);  
               SetTextColor(hdc,RGB(0,0,0));  
                SetBkColor(hdc,BkColor);  
                DrawText(hdc,"����",-1,&rt_text,DT_CENTER);  

			}
			else
			{
				MessageBox(hwnd,TEXT("���ӿͻ�̫�࣡"),TEXT("��ӭ^0^"),MB_OK|MB_ICONINFORMATION);
				//MessageBox("���ӿͻ�̫�࣡");
			}
		}
		break;

	case FD_CLOSE:		// ��⵽�׽��ֶ�Ӧ�����ӱ��رա�
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
			MessageBox(hwnd,TEXT("�Է��ѶϿ�����.."),TEXT("�������ս"),MB_OK|MB_ICONINFORMATION);
			EnableMenuItem(hMenu,IDC_SERVER,MF_ENABLED); 
			EnableMenuItem(hMenu,IDC_CONNECT,MF_ENABLED); 
			EnableMenuItem(hMenu,IDC_CUT,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_DOUBLE,MF_ENABLED);
			EnableMenuItem(hMenu,IDC_DOUBLEEND,MF_GRAYED);
            EnableMenuItem(hMenu,IDC_RESET,MF_GRAYED); 
			EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
		}
		break;

	case FD_READ:		// �׽��ֽ��ܵ��Է����͹��������ݰ�
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
               if(pch.x==18&&pch.y==18) //�Է�����
			   {       
				   if(who_play==player1)
                   MessageBox(hwnd,TEXT("�������䣬����ʤ��"),TEXT("ʤ����"),MB_OK|MB_ICONINFORMATION);
				   else
                   MessageBox(hwnd,TEXT("�������䣬����ʤ��"),TEXT("ʤ����"),MB_OK|MB_ICONINFORMATION);
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
			   if(pch.x==17&&pch.y==17) //�Է���������һ��
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
                    MessageBox(hwnd,TEXT("����ʤ��"),TEXT("ʤ����"),MB_OK|MB_ICONINFORMATION); 
					EnableMenuItem(hMenu,IDC_FAIL,MF_GRAYED);
					EnableMenuItem(hMenu,IDC_RESET,MF_ENABLED); 
                    play=0;  
                    return 0;  
                }  
                if(check_win==player2)  
                {  
                    MessageBox(hwnd,TEXT("����ʤ��"),TEXT("ʤ����"),MB_OK|MB_ICONINFORMATION);  
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
		//MessageBox(hwnd,TEXT("���ͳ�"),TEXT("�gӭ���R^0^"),MB_OK|MB_ICONINFORMATION);
	return 1;
	}

	return 0;

}