/*
 * Online Examination System - 100% C++
 * UI      : Win32 API (owner-drawn buttons, double-buffered)
 * Backend : DSA Engine (HashMap, Stack, Priority Queue, Vector)
 * Compile : g++ -o OnlineExamSystem.exe OnlineExamSystem.cpp -mwindows -lgdi32 -static
 * Run     : OnlineExamSystem.exe
 */

#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>
#include <queue>
#include <cstdio>
#include <algorithm>

// ═══════════════════════════════════════════
//  COLORS
// ═══════════════════════════════════════════
#define CLR_HEADER     RGB(12, 65, 155)
#define CLR_BLUE       RGB(30, 120, 235)
#define CLR_DARK       RGB(35, 50, 68)
#define CLR_GREEN      RGB(34, 160, 85)
#define CLR_RED        RGB(210, 45, 35)
#define CLR_PURPLE     RGB(130, 55, 170)
#define CLR_GRAY       RGB(175, 185, 195)
#define CLR_BG         RGB(240, 242, 245)
#define CLR_WHITE      RGB(255, 255, 255)
#define CLR_ORANGE     RGB(243, 156, 18)
#define CLR_BLACK      RGB(0, 0, 0)
#define CLR_TIMER_BG   RGB(160, 15, 15)
#define CLR_SECTION    RGB(50, 70, 95)
#define CLR_FOOTER     RGB(30, 42, 58)
#define CLR_BTN_GRAY   RGB(115, 130, 148)

// Control IDs
#define ID_OPT_A       1001
#define ID_OPT_D       1004
#define ID_PAL_BASE    2000
#define ID_BTN_PREV    3001
#define ID_BTN_NEXT    3002
#define ID_BTN_SAVE    3003
#define ID_BTN_MARK    3004
#define ID_BTN_CLEAR   3005
#define ID_BTN_SUBMIT  3006
#define ID_BTN_EXIT    3007
#define ID_TIMER       1
#define HEADER_H       60
#define FOOTER_H       60
#define RPANEL_W       295

// ═══════════════════════════════════════════
//  DSA ENGINE
// ═══════════════════════════════════════════
enum QStatus { QS_NV=0, QS_NA=1, QS_ANS=2, QS_MK=3, QS_AM=4 };

struct Question {
    int id; std::string text;
    std::string opts[4];
    int correct, marks, neg;
};

struct StudentResult {
    std::string name; int score;
    bool operator<(const StudentResult& o) const { return score < o.score; }
};

class ExamEngine {
public:
    std::vector<Question> questions;
    std::unordered_map<int,int> resp;
    std::unordered_map<int,QStatus> state;
    std::stack<int> hist;

    void init() {
        auto add = [&](int id, const char* t, const char* a, const char* b, const char* c, const char* d,
                       int cor, int mk, int ng) {
            Question q; q.id=id; q.text=t; q.opts[0]=a; q.opts[1]=b; q.opts[2]=c; q.opts[3]=d;
            q.correct=cor; q.marks=mk; q.neg=ng;
            questions.push_back(q);
        };
        add(1,"What is the capital of India?","Mumbai","New Delhi","Kolkata","Chennai",2,4,1);
        add(2,"What is 15 + 27?","40","42","44","38",2,4,1);
        add(3,"Which planet is known as the Red Planet?","Venus","Jupiter","Mars","Saturn",3,4,1);
        add(4,"What is the chemical formula of water?","CO2","H2O","O2","NaCl",2,4,1);
        add(5,"Who wrote 'Jana Gana Mana'?","Bankim Chandra","Rabindranath Tagore","Mahatma Gandhi","Subhash C. Bose",2,4,1);
        add(6,"How many sides does a hexagon have?","5","6","7","8",2,4,1);
        add(7,"Which gas do plants absorb for photosynthesis?","Oxygen","Nitrogen","Carbon Dioxide","Hydrogen",3,4,1);
        add(8,"What is the SI unit of force?","Joule","Watt","Newton","Pascal",3,4,1);
        add(9,"In which year did India gain independence?","1942","1945","1947","1950",3,4,1);
        add(10,"What is the square root of 144?","10","11","12","14",3,4,1);

        for (auto& q : questions) { state[q.id]=QS_NV; resp[q.id]=0; }
    }

    int count() { return (int)questions.size(); }

    int calcScore() {
        int t=0;
        for (auto& q : questions) {
            QStatus s=state[q.id];
            if (s==QS_ANS||s==QS_AM) { if(resp[q.id]==q.correct) t+=q.marks; else t-=q.neg; }
        }
        std::priority_queue<StudentResult> pq;
        pq.push({"You",t}); pq.push({"CandA",12}); pq.push({"CandB",8});
        return t;
    }

    void summary(int c[5]) {
        for(int i=0;i<5;i++) c[i]=0;
        for(auto& q:questions) c[state[q.id]]++;
    }
};

// ═══════════════════════════════════════════
//  GLOBALS
// ═══════════════════════════════════════════
static HINSTANCE g_inst;
static HWND g_wnd;
static ExamEngine g_eng;
static int g_cur=1, g_tot=0, g_sec=3600;
static bool g_done=false;
static int g_sc=0, g_mx=0;
static HWND g_rad[4], g_pal[11];
static HWND g_prev, g_next, g_save, g_mark, g_clr, g_sub, g_exit;
static HFONT g_fH, g_fB, g_fS, g_fT, g_fN, g_fBig, g_fRes, g_fOpt;
static HBRUSH g_brW, g_brBg;
static COLORREF g_pc[11], g_pf[11];

// ═══════════════════════════════════════════
//  HELPERS
// ═══════════════════════════════════════════
static COLORREF dk(COLORREF c, int a) {
    return RGB((std::max)(0,(int)GetRValue(c)-a),(std::max)(0,(int)GetGValue(c)-a),(std::max)(0,(int)GetBValue(c)-a));
}
static COLORREF lt(COLORREF c, int a) {
    return RGB((std::min)(255,(int)GetRValue(c)+a),(std::min)(255,(int)GetGValue(c)+a),(std::min)(255,(int)GetBValue(c)+a));
}

static void fillRR(HDC hdc, int x, int y, int w, int h, int r, COLORREF fill, COLORREF brd, int bw) {
    HBRUSH br=CreateSolidBrush(fill); HPEN pn=CreatePen(PS_SOLID,bw,brd);
    HBRUSH ob=(HBRUSH)SelectObject(hdc,br); HPEN op=(HPEN)SelectObject(hdc,pn);
    RoundRect(hdc,x,y,x+w,y+h,r,r);
    SelectObject(hdc,ob); SelectObject(hdc,op);
    DeleteObject(br); DeleteObject(pn);
}

static void fillR(HDC hdc, int x, int y, int w, int h, COLORREF c) {
    RECT r={x,y,x+w,y+h}; HBRUSH b=CreateSolidBrush(c);
    FillRect(hdc,&r,b); DeleteObject(b);
}

static COLORREF sClr(int s) {
    switch(s){ case 1:return CLR_RED; case 2:return CLR_GREEN; case 3:case 4:return CLR_PURPLE; default:return CLR_GRAY; }
}

struct BS { COLORREF bg; };
static BS nBS(int id) {
    switch(id){
        case ID_BTN_PREV:case ID_BTN_NEXT:return{CLR_BLUE};
        case ID_BTN_SAVE:return{CLR_GREEN};
        case ID_BTN_MARK:return{CLR_PURPLE};
        case ID_BTN_CLEAR:return{CLR_BTN_GRAY};
        case ID_BTN_SUBMIT:return{CLR_RED};
        case ID_BTN_EXIT:return{CLR_BLUE};
        default:return{CLR_GRAY};
    }
}

// ═══════════════════════════════════════════
//  STATE
// ═══════════════════════════════════════════
static void updPal() {
    for(int i=1;i<=g_tot;i++){
        g_pc[i]=sClr(g_eng.state[i]);
        g_pf[i]=(g_eng.state[i]==QS_NV)?CLR_DARK:CLR_WHITE;
        InvalidateRect(g_pal[i],NULL,TRUE);
    }
}

static void loadQ(int n) {
    g_cur=n; Question& q=g_eng.questions[n-1];
    if(g_eng.state[q.id]==QS_NV) g_eng.state[q.id]=QS_NA;
    const char* lb[]={"A","B","C","D"};
    for(int i=0;i<4;i++){
        std::string s="    ("; s+=lb[i]; s+=")   "; s+=q.opts[i];
        SetWindowTextA(g_rad[i],s.c_str());
        SendMessage(g_rad[i],BM_SETCHECK,(g_eng.resp[q.id]==i+1)?BST_CHECKED:BST_UNCHECKED,0);
    }
    g_eng.hist.push(n); updPal(); InvalidateRect(g_wnd,NULL,TRUE);
}

static int getSel() {
    for(int i=0;i<4;i++) if(SendMessage(g_rad[i],BM_GETCHECK,0,0)==BST_CHECKED) return i+1;
    return 0;
}
static void svSt() {
    int o=getSel(); g_eng.resp[g_cur]=o;
    if(o>0&&g_eng.state[g_cur]==QS_NA) g_eng.state[g_cur]=QS_ANS;
}

static void doNext(){svSt();if(g_cur<g_tot)loadQ(g_cur+1);}
static void doPrev(){svSt();if(g_cur>1)loadQ(g_cur-1);}
static void doSave(){int o=getSel();g_eng.resp[g_cur]=o;g_eng.state[g_cur]=(o>0)?QS_ANS:QS_NA;
    if(g_cur<g_tot)loadQ(g_cur+1);else{updPal();InvalidateRect(g_wnd,NULL,TRUE);}}
static void doMark(){int o=getSel();g_eng.resp[g_cur]=o;g_eng.state[g_cur]=(o>0)?QS_AM:QS_MK;
    if(g_cur<g_tot)loadQ(g_cur+1);else{updPal();InvalidateRect(g_wnd,NULL,TRUE);}}
static void doClear(){for(int i=0;i<4;i++)SendMessage(g_rad[i],BM_SETCHECK,BST_UNCHECKED,0);
    g_eng.resp[g_cur]=0;g_eng.state[g_cur]=QS_NA;updPal();InvalidateRect(g_wnd,NULL,TRUE);}

static void showRes() {
    g_done=true; KillTimer(g_wnd,ID_TIMER);
    for(int i=0;i<4;i++)ShowWindow(g_rad[i],SW_HIDE);
    for(int i=1;i<=g_tot;i++)ShowWindow(g_pal[i],SW_HIDE);
    ShowWindow(g_prev,SW_HIDE);ShowWindow(g_next,SW_HIDE);ShowWindow(g_save,SW_HIDE);
    ShowWindow(g_mark,SW_HIDE);ShowWindow(g_clr,SW_HIDE);ShowWindow(g_sub,SW_HIDE);
    RECT rc;GetClientRect(g_wnd,&rc);
    MoveWindow(g_exit,(rc.right-240)/2,rc.bottom/2+120,240,50,TRUE);
    ShowWindow(g_exit,SW_SHOW);InvalidateRect(g_wnd,NULL,TRUE);
}

static void doSubmit() {
    svSt(); int c[5]; g_eng.summary(c);
    char m[512]; sprintf(m,"SUBMISSION SUMMARY\n=============================\n\n"
        "  Not Visited          :  %d\n  Not Answered        :  %d\n"
        "  Answered               :  %d\n  Marked for Review  :  %d\n"
        "  Answered & Marked :  %d\n\n=============================\n"
        "Are you sure you want to submit?",c[0],c[1],c[2],c[3],c[4]);
    if(MessageBoxA(g_wnd,m,"Confirm Submission",MB_YESNO|MB_ICONWARNING)==IDYES){
        g_sc=g_eng.calcScore();g_mx=g_tot*4;showRes();}
}

// ═══════════════════════════════════════════
//  CONTROLS
// ═══════════════════════════════════════════
static void mkCtrls(HWND w) {
    for(int i=0;i<4;i++){
        DWORD s=WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON; if(!i)s|=WS_GROUP;
        g_rad[i]=CreateWindowA("BUTTON","",s,0,0,10,10,w,(HMENU)(UINT_PTR)(ID_OPT_A+i),g_inst,NULL);
        SendMessage(g_rad[i],WM_SETFONT,(WPARAM)g_fOpt,TRUE);
    }
    for(int i=1;i<=g_tot;i++){
        char b[4];sprintf(b,"%d",i); g_pc[i]=CLR_GRAY;g_pf[i]=CLR_DARK;
        g_pal[i]=CreateWindowA("BUTTON",b,WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,0,0,10,10,w,
            (HMENU)(UINT_PTR)(ID_PAL_BASE+i),g_inst,NULL);
    }
    g_mark=CreateWindowA("BUTTON","Mark for Review",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,0,0,10,10,w,(HMENU)ID_BTN_MARK,g_inst,NULL);
    g_clr =CreateWindowA("BUTTON","Clear Response", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,0,0,10,10,w,(HMENU)ID_BTN_CLEAR,g_inst,NULL);
    g_prev=CreateWindowA("BUTTON","<  Previous",    WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,0,0,10,10,w,(HMENU)ID_BTN_PREV,g_inst,NULL);
    g_next=CreateWindowA("BUTTON","Next  >",        WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,0,0,10,10,w,(HMENU)ID_BTN_NEXT,g_inst,NULL);
    g_save=CreateWindowA("BUTTON","Save & Next >>", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,0,0,10,10,w,(HMENU)ID_BTN_SAVE,g_inst,NULL);
    g_sub =CreateWindowA("BUTTON","Submit Exam",    WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,0,0,10,10,w,(HMENU)ID_BTN_SUBMIT,g_inst,NULL);
    g_exit=CreateWindowA("BUTTON","Close Application",WS_CHILD|BS_OWNERDRAW,          0,0,10,10,w,(HMENU)ID_BTN_EXIT,g_inst,NULL);
}

static void layout(HWND w) {
    RECT rc;GetClientRect(w,&rc);
    int W=rc.right,H=rc.bottom;
    int rY=HEADER_H+200;
    for(int i=0;i<4;i++) MoveWindow(g_rad[i],50,rY+i*48,W-RPANEL_W-80,42,TRUE);
    int pX=W-RPANEL_W+20,pY=HEADER_H+240;
    for(int i=1;i<=g_tot;i++){int c=(i-1)%5,r=(i-1)/5;MoveWindow(g_pal[i],pX+c*52,pY+r*50,44,42,TRUE);}
    int fy=H-FOOTER_H+11,bh=40;
    MoveWindow(g_mark,18,fy,158,bh,TRUE); MoveWindow(g_clr,186,fy,148,bh,TRUE);
    int cx=(W-RPANEL_W)/2-165;
    MoveWindow(g_prev,cx,fy,128,bh,TRUE); MoveWindow(g_next,cx+138,fy,112,bh,TRUE);
    MoveWindow(g_save,cx+260,fy,165,bh,TRUE); MoveWindow(g_sub,W-188,fy,168,bh,TRUE);
}

// ═══════════════════════════════════════════
//  PAINTING
// ═══════════════════════════════════════════
static void paintExam(HDC dc, int W, int H) {
    // White question background
    fillR(dc, 0, HEADER_H, W-RPANEL_W, H-HEADER_H-FOOTER_H, CLR_WHITE);

    // ── HEADER — solid deep blue ──
    fillR(dc, 0, 0, W, HEADER_H, CLR_HEADER);
    // Golden accent line at bottom
    fillR(dc, 0, HEADER_H-3, W, 3, RGB(230,180,40));

    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, g_fH);
    SetTextColor(dc, CLR_WHITE);
    RECT tR={25,0,550,HEADER_H};
    DrawTextA(dc,"Online Examination System",-1,&tR,DT_LEFT|DT_VCENTER|DT_SINGLELINE);

    // ── TIMER — WHITE text on DARK RED rounded box ──
    char tb[32]; sprintf(tb," %02d : %02d : %02d ",g_sec/3600,(g_sec%3600)/60,g_sec%60);
    int tmX=W-300, tmY=11, tmW=275, tmH=HEADER_H-22;
    // Timer background
    fillRR(dc, tmX, tmY, tmW, tmH, 14, CLR_TIMER_BG, RGB(255,100,100), 2);
    // Timer text — LARGE WHITE BOLD
    SelectObject(dc, g_fT);
    SetTextColor(dc, CLR_WHITE);
    RECT tmR={tmX, tmY, tmX+tmW, tmY+tmH};
    DrawTextA(dc, tb, -1, &tmR, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    // Flashing effect when < 5 min
    if (g_sec <= 300 && (g_sec % 2 == 0)) {
        SetTextColor(dc, RGB(255,220,100));
        DrawTextA(dc, tb, -1, &tmR, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }

    // "Time Remaining" label
    SelectObject(dc, g_fS);
    SetTextColor(dc, RGB(180,200,230));
    RECT tlR={tmX-145,0,tmX-5,HEADER_H};
    DrawTextA(dc,"Time Remaining",-1,&tlR,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);

    // ── SECTION BAR ──
    fillR(dc, 0, HEADER_H, W-RPANEL_W, 34, CLR_SECTION);
    SelectObject(dc, g_fS);
    SetTextColor(dc, RGB(190,210,235));
    RECT sT={22,HEADER_H+6,W-RPANEL_W,HEADER_H+30};
    DrawTextA(dc,"Section: General Paper   |   MCQ   |   Marks: +4 / -1",-1,&sT,DT_LEFT|DT_SINGLELINE);

    // ── QUESTION NUMBER ──
    char qb[64]; sprintf(qb,"Question %d of %d",g_cur,g_tot);
    SelectObject(dc, g_fN); SetTextColor(dc, CLR_DARK);
    RECT qnR={35,HEADER_H+44,W-RPANEL_W,HEADER_H+66};
    DrawTextA(dc,qb,-1,&qnR,DT_LEFT|DT_SINGLELINE);

    // ── QUESTION TEXT BOX — rounded rectangle ──
    Question& q = g_eng.questions[g_cur-1];
    // Shadow
    fillRR(dc, 33, HEADER_H+73, W-RPANEL_W-53, 118, 10, RGB(210,215,225), RGB(210,215,225), 1);
    // Box
    fillRR(dc, 30, HEADER_H+70, W-RPANEL_W-50, 118, 10, RGB(248,250,255), RGB(180,190,210), 1);
    // Text
    SelectObject(dc, g_fB); SetTextColor(dc, RGB(25,25,50));
    RECT qTR={50,HEADER_H+82,W-RPANEL_W-40,HEADER_H+182};
    DrawTextA(dc,q.text.c_str(),-1,&qTR,DT_LEFT|DT_WORDBREAK);

    // ── RIGHT PANEL ──
    fillR(dc, W-RPANEL_W, HEADER_H, RPANEL_W, H-HEADER_H-FOOTER_H, CLR_BG);
    // Left border
    fillR(dc, W-RPANEL_W, HEADER_H, 3, H-HEADER_H-FOOTER_H, CLR_BLUE);

    // Legend title
    int rpX=W-RPANEL_W+15;
    SelectObject(dc, g_fN); SetTextColor(dc, CLR_HEADER);
    RECT ltR={rpX,HEADER_H+10,W-15,HEADER_H+32};
    DrawTextA(dc,"Question Status",-1,&ltR,DT_CENTER|DT_SINGLELINE);
    // Underline
    fillR(dc, rpX+25, HEADER_H+36, RPANEL_W-55, 2, CLR_BLUE);

    // Legend items
    COLORREF lc[]={CLR_GRAY, CLR_RED, CLR_GREEN, CLR_PURPLE, CLR_PURPLE};
    const char* ll[]={"Not Visited","Not Answered","Answered","Marked for Review","Answered & Marked"};
    int cn[5]; g_eng.summary(cn);

    int ly=HEADER_H+50;
    for(int i=0;i<5;i++){
        // Colored pill
        fillRR(dc, rpX+12, ly+2, 32, 22, 8, lc[i], dk(lc[i],25), 1);
        // Count
        char cb[8]; sprintf(cb,"%d",cn[i]);
        SelectObject(dc,g_fS); SetTextColor(dc,CLR_WHITE);
        RECT cr={rpX+12,ly+2,rpX+44,ly+24};
        DrawTextA(dc,cb,-1,&cr,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        // Label
        SetTextColor(dc,CLR_DARK);
        RECT lr={rpX+52,ly,W-15,ly+26};
        DrawTextA(dc,ll[i],-1,&lr,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        ly+=32;
    }

    // Palette title
    fillRR(dc, rpX, HEADER_H+215, RPANEL_W-30, 22, 6, CLR_DARK, CLR_DARK, 1);
    SetTextColor(dc,CLR_WHITE);
    RECT ptR={rpX,HEADER_H+215,rpX+RPANEL_W-30,HEADER_H+237};
    DrawTextA(dc,"Question Palette",-1,&ptR,DT_CENTER|DT_VCENTER|DT_SINGLELINE);

    // ── FOOTER ──
    fillR(dc, 0, H-FOOTER_H, W, FOOTER_H, CLR_FOOTER);
    // Top accent
    fillR(dc, 0, H-FOOTER_H, W, 2, RGB(55,80,115));
}

static void paintRes(HDC dc, int W, int H) {
    // Soft background
    fillR(dc, 0, 0, W, H, RGB(235,240,250));
    // Decorative circle
    HBRUSH cb=CreateSolidBrush(RGB(220,228,245));
    HPEN cp=CreatePen(PS_SOLID,3,RGB(190,200,225));
    SelectObject(dc,cb); SelectObject(dc,cp);
    int cx=W/2,cy=H/2-20;
    Ellipse(dc,cx-140,cy-140,cx+140,cy+140);
    DeleteObject(cb); DeleteObject(cp);

    SetBkMode(dc,TRANSPARENT);
    SelectObject(dc,g_fRes); SetTextColor(dc,CLR_DARK);
    RECT t1={0,H/2-165,W,H/2-105};
    DrawTextA(dc,"Examination Complete",-1,&t1,DT_CENTER|DT_SINGLELINE);

    SetTextColor(dc,g_sc>=0?CLR_GREEN:CLR_RED);
    RECT t2={0,H/2-95,W,H/2-55};
    DrawTextA(dc,g_sc>=0?"PASSED":"NEEDS IMPROVEMENT",-1,&t2,DT_CENTER|DT_SINGLELINE);

    char sb[64]; sprintf(sb,"Your Score:  %d / %d",g_sc,g_mx);
    SelectObject(dc,g_fBig);
    RECT t3={0,H/2-35,W,H/2+15};
    DrawTextA(dc,sb,-1,&t3,DT_CENTER|DT_SINGLELINE);

    char pb[64]; sprintf(pb,"Percentage:  %.1f%%",g_sc*100.0/g_mx);
    SelectObject(dc,g_fB); SetTextColor(dc,CLR_DARK);
    RECT t4={0,H/2+30,W,H/2+60};
    DrawTextA(dc,pb,-1,&t4,DT_CENTER|DT_SINGLELINE);
}

static void drawBtn(DRAWITEMSTRUCT* d) {
    HDC dc=d->hDC; RECT rc=d->rcItem; int id=d->CtlID;
    bool pr=(d->itemState&ODS_SELECTED)!=0;

    if(id>ID_PAL_BASE && id<=ID_PAL_BASE+g_tot) {
        // ── Palette button — rounded, solid color ──
        int qi=id-ID_PAL_BASE;
        COLORREF bg=pr?dk(g_pc[qi],30):g_pc[qi];
        fillRR(dc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, 10, bg, dk(bg,30), 1);
        // Orange highlight for current
        if(qi==g_cur) {
            HPEN hp=CreatePen(PS_SOLID,3,CLR_ORANGE);
            HPEN op=(HPEN)SelectObject(dc,hp);
            HBRUSH ob=(HBRUSH)SelectObject(dc,GetStockObject(NULL_BRUSH));
            RoundRect(dc,rc.left+1,rc.top+1,rc.right-1,rc.bottom-1,10,10);
            SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(hp);
        }
        // Number
        SetBkMode(dc,TRANSPARENT); SelectObject(dc,g_fN); SetTextColor(dc,g_pf[qi]);
        char nb[4]; sprintf(nb,"%d",qi);
        DrawTextA(dc,nb,-1,&rc,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    } else {
        // ── Nav button — rounded, solid color ──
        BS st=nBS(id);
        COLORREF bg=pr?dk(st.bg,35):st.bg;
        int w=rc.right-rc.left, h=rc.bottom-rc.top;
        fillRR(dc, rc.left, rc.top, w, h, 12, bg, dk(bg,25), 1);
        // Top highlight
        if(!pr) {
            HPEN hp=CreatePen(PS_SOLID,1,lt(bg,30));
            HPEN op=(HPEN)SelectObject(dc,hp);
            MoveToEx(dc,rc.left+6,rc.top+2,NULL); LineTo(dc,rc.right-6,rc.top+2);
            SelectObject(dc,op); DeleteObject(hp);
        }
        // Text with shadow
        SetBkMode(dc,TRANSPARENT);
        HFONT f=(id==ID_BTN_SUBMIT||id==ID_BTN_EXIT)?g_fN:g_fS;
        SelectObject(dc,f);
        char tx[64]; GetWindowTextA(d->hwndItem,tx,64);
        // Shadow
        RECT sh={rc.left+1,rc.top+1,rc.right+1,rc.bottom+1};
        SetTextColor(dc,dk(bg,60));
        DrawTextA(dc,tx,-1,&sh,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        // Main
        SetTextColor(dc,CLR_WHITE);
        DrawTextA(dc,tx,-1,&rc,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }
}

// ═══════════════════════════════════════════
//  WNDPROC
// ═══════════════════════════════════════════
static LRESULT CALLBACK WP(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    switch(m) {
    case WM_CREATE:
        g_fH  =CreateFontA(-22,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        g_fB  =CreateFontA(-16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        g_fS  =CreateFontA(-13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        g_fT  =CreateFontA(-26,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Consolas");
        g_fN  =CreateFontA(-14,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        g_fBig=CreateFontA(-30,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        g_fRes=CreateFontA(-36,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        g_fOpt=CreateFontA(-15,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Segoe UI");
        g_brW=CreateSolidBrush(CLR_WHITE); g_brBg=CreateSolidBrush(CLR_BG);
        g_eng.init(); g_tot=g_eng.count();
        mkCtrls(w); SetTimer(w,ID_TIMER,1000,NULL);
        PostMessage(w,WM_USER+1,0,0); return 0;

    case WM_USER+1: layout(w); loadQ(1); return 0;
    case WM_SIZE: if(!g_done&&g_tot>0) layout(w); return 0;

    case WM_TIMER:
        if(wp==ID_TIMER&&!g_done){
            g_sec--;
            if(g_sec<=0){MessageBoxA(w,"Time's up!","Time Over",MB_OK|MB_ICONWARNING);
                svSt();g_sc=g_eng.calcScore();g_mx=g_tot*4;showRes();}
            InvalidateRect(w,NULL,FALSE);
        } return 0;

    case WM_ERASEBKGND: return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc=BeginPaint(w,&ps);
        RECT rc; GetClientRect(w,&rc);
        HDC mem=CreateCompatibleDC(hdc);
        HBITMAP bmp=CreateCompatibleBitmap(hdc,rc.right,rc.bottom);
        HBITMAP ob=(HBITMAP)SelectObject(mem,bmp);
        FillRect(mem,&rc,g_brW);
        if(g_done) paintRes(mem,rc.right,rc.bottom);
        else       paintExam(mem,rc.right,rc.bottom);
        BitBlt(hdc,0,0,rc.right,rc.bottom,mem,0,0,SRCCOPY);
        SelectObject(mem,ob); DeleteObject(bmp); DeleteDC(mem);
        EndPaint(w,&ps); return 0;
    }

    case WM_DRAWITEM:
        if(((DRAWITEMSTRUCT*)lp)->CtlType==ODT_BUTTON){drawBtn((DRAWITEMSTRUCT*)lp);return TRUE;}
        break;

    case WM_CTLCOLORSTATIC: {
        HDC h=(HDC)wp; SetBkColor(h,CLR_WHITE); SetTextColor(h,RGB(25,25,50));
        return (LRESULT)g_brW;
    }

    case WM_COMMAND: {
        int id=LOWORD(wp);
        if(id>ID_PAL_BASE&&id<=ID_PAL_BASE+g_tot){svSt();loadQ(id-ID_PAL_BASE);return 0;}
        switch(id){
            case ID_BTN_PREV:doPrev();break; case ID_BTN_NEXT:doNext();break;
            case ID_BTN_SAVE:doSave();break; case ID_BTN_MARK:doMark();break;
            case ID_BTN_CLEAR:doClear();break; case ID_BTN_SUBMIT:doSubmit();break;
            case ID_BTN_EXIT:PostQuitMessage(0);break;
        } return 0;
    }

    case WM_CLOSE: if(g_done) PostQuitMessage(0); else doSubmit(); return 0;

    case WM_DESTROY:
        KillTimer(w,ID_TIMER);
        DeleteObject(g_fH);DeleteObject(g_fB);DeleteObject(g_fS);DeleteObject(g_fT);
        DeleteObject(g_fN);DeleteObject(g_fBig);DeleteObject(g_fRes);DeleteObject(g_fOpt);
        DeleteObject(g_brW);DeleteObject(g_brBg);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(w,m,wp,lp);
}

// ═══════════════════════════════════════════
//  ENTRY POINT
// ═══════════════════════════════════════════
int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int) {
    g_inst=hI;
    WNDCLASSEXA wc={};
    wc.cbSize=sizeof(wc); wc.lpfnWndProc=WP; wc.hInstance=hI;
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName="OESClass"; wc.hIcon=LoadIcon(NULL,IDI_APPLICATION);
    RegisterClassExA(&wc);

    g_wnd=CreateWindowExA(0,"OESClass","Online Examination System",
        WS_OVERLAPPEDWINDOW,0,0,
        GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),
        NULL,NULL,hI,NULL);
    ShowWindow(g_wnd,SW_SHOWMAXIMIZED);
    UpdateWindow(g_wnd);

    MSG msg;
    while(GetMessage(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
    return (int)msg.wParam;
}
