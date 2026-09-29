/* ------------------------------------------------------------------
 * bplus.c -- a small BASIC-PLUS interpreter, enough to run
 *            UNDERGROUND (Gary Kleppe, 1979/80) on Windows.
 *
 * The point is NOT a general BASIC-PLUS implementation.  It runs the
 * recovered UNDERG.BAS / LOOK.BAS *as written*, so the source stays the
 * artifact and stays editable: change a line in any text editor, run the
 * exe, see the result.  No tokenising pass, no compile step, and errors
 * report the program's own line numbers.
 *
 *   Build:  gcc -O2 -o underg.exe bplus.c
 *   Run:    underg.exe [mainprog.bas [chained.bas ...]]
 *           defaults to  UNDERG.BAS  plus  LOOK.BAS
 *
 * BASIC-PLUS features that matter here -- the ones a port to a modern
 * BASIC would have to unpick by hand:
 *
 *   - postfix modifiers:  stmt IF cond  /  stmt FOR v=a TO b [STEP c]
 *     and they chain, outermost last:
 *         O%(X%)=X%(0%) IF O%(X%)=-1% FOR X%=0% TO 15%
 *   - a THEN clause runs to end of line, or to its matching ELSE
 *   - string comparison ignores trailing blanks, so "N     " = "N".
 *     The parser is built on this; without it nothing matches at all.
 *   - LSET assigns into a fixed-length field, keeping the target length
 *   - numeric truthiness, hence subtraction used as "not equal":
 *         IF X%(0%)-19% THEN ...
 *   - integer division truncates
 *   - GOSUB/RETURN and FOR/NEXT resume *mid-line*, e.g.
 *         1220 ... GOSUB3080 : IFZ$="N"THEN1000ELSE...
 *   - virtual arrays (DIM #n) live in a file, packed contiguously, two
 *     bytes per element, little-endian.  Confirmed against RSTS/E: V1's
 *     save file is 6 blocks, and M%(87,14)+O%(45)+X%(11) packed with no
 *     per-array alignment is 2756 bytes -> 6 blocks exactly.
 *   - CHAIN clears variables; state lives in the virtual array file whose
 *     name passes between programs through "core common"
 *     (SYS(CHR$(8%)+name) puts, SYS(CHR$(7%)) gets)
 *
 * V2 additionally uses DEF FN/FNEND, FIELD, GET..RECORD, MAT assignment
 * and XLATE; those are implemented here too, though V2 cannot start until
 * its lost GROUND.DAT is reconstructed.
 * ------------------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <setjmp.h>

#ifdef _WIN32
#define strncasecmp _strnicmp
#define strcasecmp  _stricmp
#endif

#define MAXLINES 2000
#define MAXPROGS 6
#define MAXCHAN  16
#define MAXFN    16

/* ============================================================ values == */

typedef struct { int isstr, isint; double num; char *s; int slen; } Val;

static char *xdup(const char *p,int n){ char *r=malloc(n+1); memcpy(r,p,n); r[n]=0; return r; }
static Val vnum(double d,int i){ Val v; v.isstr=0; v.num=d; v.isint=i; v.s=NULL; v.slen=0; return v; }
static Val vstr(const char*p,int n){ Val v; v.isstr=1; v.num=0; v.isint=0; v.s=xdup(p,n); v.slen=n; return v; }

/* =========================================================== storage == */

typedef struct Var { char name[10]; Val v; struct Var *next; } Var;
typedef struct Arr {
    char name[10]; int isstr, ndim, dim[2];
    Val *mem; int virt, chan; long base; struct Arr *next;
} Arr;
static Var *vars; static Arr *arrs;

typedef struct {
    FILE *f; int isterm, used;
    char name[300];
    unsigned char buf[512]; int buflen;
} Chan;
static Chan chans[MAXCHAN];
static char corecommon[300];

typedef struct { int num; char *text; } Line;
typedef struct { char name[10]; int ndim,d1,d2,chan; long base; } VDecl;
typedef struct { char name[64]; Line line[MAXLINES]; int n;
                 VDecl vd[8]; int nvd; } Prog;
static Prog progs[MAXPROGS]; static int nprogs, curprog;

typedef struct { char name[10], param[10]; int prog, lineidx, bodyoff; } Fn;
static Fn fns[MAXFN]; static int nfns;

static int pc, lineoff;
static int errline, errcode, errhandler, in_handler;
static jmp_buf err_jmp;

static struct { int prog, pc, off; } gstack[64]; static int gsp;
static struct { char var[10]; double limit, step; int prog, pc, off; } fstack[32]; static int fsp;
static int datline, datfield;

enum { FLOW_NEXT=0, FLOW_JUMP, FLOW_RESUME, FLOW_ENDLINE, FLOW_FNEND, FLOW_END };
static int flow, jump_line, resume_pc, resume_off;

static const char *g_line_base, *g_stmt_after;
static int inch=-1;   /* channel of the INPUT being executed */
static int keepcase;  /* BPKEEPCASE=1 -> keep input case verbatim */

static void rt_error(int code);
static Val expr(void);
static void exec_range(const char *p,const char *end);
static void exec_one(const char *p,const char *end);
static void install_vdims(int pi);

/* ============================================================= utils == */

static void fatal(const char *fmt,...){
    va_list ap; va_start(ap,fmt);
    fprintf(stderr,"\n?"); vfprintf(stderr,fmt,ap); va_end(ap);
    fprintf(stderr,"\n"); exit(1);
}
static int upc(int c){ return toupper((unsigned char)c); }
static int curlineno(void){ return progs[curprog].line[pc].num; }

/* BASIC-PLUS pads the shorter operand with blanks before comparing */
static int bp_cmp(const char*a,int na,const char*b,int nb){
    int n=na>nb?na:nb,i;
    for(i=0;i<n;i++){
        int ca=i<na?(unsigned char)a[i]:' ', cb=i<nb?(unsigned char)b[i]:' ';
        if(ca!=cb) return ca<cb?-1:1;
    }
    return 0;
}
static int find_line(int prog,int num){
    int lo=0,hi=progs[prog].n-1;
    while(lo<=hi){ int m=(lo+hi)/2;
        if(progs[prog].line[m].num==num) return m;
        if(progs[prog].line[m].num<num) lo=m+1; else hi=m-1; }
    return -1;
}

/* ========================================================== tokenizer == */

static const char *KW[] = {
    "INPUTLINE","RESTORE","RESUME","RETURN","RANDOM","UNLESS","RECORD","GOSUB",
    "FNEND","WHILE","UNTIL","PRINT","INPUT","CHAIN","CLOSE","ERROR","FIELD",
    "XLATE","GOTO","THEN","RND","ERL","ERR","ELSE","STEP","NEXT","DATA","READ","LINE","FILE",
    "KILL","LSET","RSET","OPEN","STOP","TIME","AND","NOT","DIM","LET","FOR",
    "END","MAT","GET","PUT","DEF","ON","IF","TO","AS","OR",NULL
};

typedef enum { T_EOL,T_KW,T_NUM,T_STR,T_ID,T_PUNC } Tk;
typedef struct {
    Tk kind; char text[16]; double num; int isint;
    const char *str; int slen; const char *start;
} Token;

static const char *cur,*limit;
static Token peeked; static int have_peek;

static void skipsp(void){ while(cur<limit&&(*cur==' '||*cur=='\t')) cur++; }

static int kw_at(const char *p,const char *end,char *out){
    int i,bestlen=0,best=-1;
    for(i=0;KW[i];i++){
        int n=(int)strlen(KW[i]),j,ok=1;
        if(p+n>end) continue;
        for(j=0;j<n;j++) if(upc(p[j])!=KW[i][j]){ ok=0; break; }
        if(ok&&n>bestlen){ bestlen=n; best=i; }
    }
    if(best<0) return 0;
    strcpy(out,KW[best]); return bestlen;
}

static Token lex(void){
    Token t; memset(&t,0,sizeof t);
    skipsp(); t.start=cur;
    if(cur>=limit){ t.kind=T_EOL; return t; }

    if(*cur=='"'||*cur=='\''){
        char q=*cur++; const char *s=cur;
        while(cur<limit&&*cur!=q) cur++;
        t.kind=T_STR; t.str=s; t.slen=(int)(cur-s);
        if(cur<limit) cur++;
        return t;
    }
    if(isdigit((unsigned char)*cur)||(*cur=='.'&&cur+1<limit&&isdigit((unsigned char)cur[1]))){
        char b[64]; int i=0;
        while(cur<limit&&(isdigit((unsigned char)*cur)||*cur=='.')){ if(i<63) b[i++]=*cur; cur++; }
        b[i]=0; t.kind=T_NUM; t.num=atof(b); t.isint=0;
        if(cur<limit&&*cur=='%'){ cur++; t.isint=1; t.num=(double)(long)t.num; }
        return t;
    }
    if(isalpha((unsigned char)*cur)){
        /* user function name: FN + letter [digit] [% $] -- check before keywords
           so FNR$ is not mistaken, but after FNEND which IS a keyword */
        if(cur+2<limit && upc(cur[0])=='F' && upc(cur[1])=='N' &&
           isalpha((unsigned char)cur[2]) &&
           !(cur+5<=limit && !strncasecmp(cur,"FNEND",5))){
            int i=0;
            t.text[i++]=(char)upc(*cur++); t.text[i++]=(char)upc(*cur++);
            t.text[i++]=(char)upc(*cur++);
            if(cur<limit&&isdigit((unsigned char)*cur)) t.text[i++]=*cur++;
            if(cur<limit&&(*cur=='%'||*cur=='$'))       t.text[i++]=*cur++;
            t.text[i]=0; t.kind=T_ID; return t;
        }
        { char kw[32]; int n=kw_at(cur,limit,kw);
          if(n){ t.kind=T_KW; strcpy(t.text,kw); cur+=n; return t; } }
        { int i=0;
          t.text[i++]=(char)upc(*cur++);
          if(cur<limit&&isdigit((unsigned char)*cur)) t.text[i++]=*cur++;
          if(cur<limit&&(*cur=='%'||*cur=='$'))       t.text[i++]=*cur++;
          t.text[i]=0; t.kind=T_ID; return t; }
    }
    t.kind=T_PUNC;
    if(cur+1<limit&&((cur[0]=='<'&&(cur[1]=='>'||cur[1]=='='))||(cur[0]=='>'&&cur[1]=='='))){
        t.text[0]=cur[0]; t.text[1]=cur[1]; t.text[2]=0; cur+=2;
    } else { t.text[0]=*cur++; t.text[1]=0; }
    return t;
}
static Token next(void){ if(have_peek){ have_peek=0; return peeked; } return lex(); }
static Token peek(void){ if(!have_peek){ peeked=lex(); have_peek=1; } return peeked; }
static int isp(Token t,const char*p){ return t.kind==T_PUNC&&!strcmp(t.text,p); }
static int isk(Token t,const char*p){ return t.kind==T_KW &&!strcmp(t.text,p); }
static void expect(const char*p){
    Token t=next();
    if(!isp(t,p)&&!isk(t,p)) fatal("Syntax error at line %d (wanted %s)",curlineno(),p);
}

/* ========================================================== variables == */

static Var *getvar(const char*name){
    Var *v; for(v=vars;v;v=v->next) if(!strcmp(v->name,name)) return v;
    v=calloc(1,sizeof *v); strcpy(v->name,name);
    v->v = strchr(name,'$') ? vstr("",0) : vnum(0,strchr(name,'%')!=NULL);
    v->next=vars; vars=v; return v;
}
static Arr *getarr(const char*name){
    Arr *a; for(a=arrs;a;a=a->next) if(!strcmp(a->name,name)) return a; return NULL;
}
static Arr *newarr(const char*name){
    Arr *a=getarr(name);
    if(!a){ a=calloc(1,sizeof *a); strcpy(a->name,name); a->next=arrs; arrs=a; }
    return a;
}
static void arr_alloc(Arr*a,int d1,int d2,int ndim){
    long n=(long)(d1+1)*(ndim==2?(d2+1):1),i;
    a->isstr=strchr(a->name,'$')!=NULL;
    a->ndim=ndim; a->dim[0]=d1; a->dim[1]=d2; a->virt=0;
    a->mem=calloc(n,sizeof(Val));
    for(i=0;i<n;i++) a->mem[i]= a->isstr? vstr("",0) : vnum(0,1);
}
static Arr *ensure_arr(const char *name,int nd){
    /* BASIC-PLUS auto-dimensions an undeclared array to (10) on first use.
       UNDERG relies on this: CHAIN clears variables, and the chain back from
       LOOK re-enters at 995 -- past the DIM of A$ at line 970. */
    Arr *a=getarr(name);
    if(!a || (!a->mem && !a->virt)){ a=newarr(name); arr_alloc(a,10,nd==2?10:0,nd); }
    return a;
}

static long arr_idx(Arr*a,int i,int j){
    if(a->ndim==2){ if(i<0||i>a->dim[0]||j<0||j>a->dim[1]) rt_error(15);
                    return (long)i*(a->dim[1]+1)+j; }
    if(i<0||i>a->dim[0]) rt_error(15);
    return i;
}
static int vget(Arr*a,long idx){
    Chan*c=&chans[a->chan]; unsigned char b[2];
    if(!c->f) rt_error(9);
    if(fseek(c->f,(a->base+idx)*2,SEEK_SET)) return 0;
    if(fread(b,1,2,c->f)!=2) return 0;
    return (short)(b[0]|(b[1]<<8));
}
static void vput(Arr*a,long idx,int val){
    Chan*c=&chans[a->chan]; unsigned char b[2];
    if(!c->f) rt_error(9);
    b[0]=val&0xFF; b[1]=(val>>8)&0xFF;
    fseek(c->f,(a->base+idx)*2,SEEK_SET); fwrite(b,1,2,c->f); fflush(c->f);
}

/* ========================================================= expressions == */

static double tonum(Val v){ if(v.isstr) rt_error(46); return v.num; }

static Val call_sys(Val a){
    if(a.isstr&&a.slen>=1){
        unsigned char fn=(unsigned char)a.s[0];
        if(fn==7) return vstr(corecommon,(int)strlen(corecommon));
        if(fn==8){ int n=a.slen-1; if(n>299) n=299;
                   memcpy(corecommon,a.s+1,n); corecommon[n]=0; return vstr("",0); }
    }
    return vstr("",0);                      /* terminal / FIP calls: no-ops */
}
static Val cvt2(Val s,int f){
    char *o=malloc(s.slen+1); int i,n=0;
    for(i=0;i<s.slen;i++){
        unsigned char c=(unsigned char)s.s[i];
        if(f&1) c&=0x7F;
        if((f&2)&&(c==' '||c=='\t')) continue;
        if((f&4)&&(c=='\r'||c=='\n'||c==12||c==27||c==127||c==0)) continue;
        if((f&32)&&c>='a'&&c<='z') c=(unsigned char)(c-32);
        if(f&64){ if(c=='[') c='('; else if(c==']') c=')'; }
        if((f&16)&&(c==' '||c=='\t')){ if(n&&o[n-1]==' ') continue; c=' '; }
        o[n++]=(char)c;
    }
    if(f&8){ int k=0; while(k<n&&(o[k]==' '||o[k]=='\t'))k++; memmove(o,o+k,n-k); n-=k; }
    if(f&128){ while(n&&(o[n-1]==' '||o[n-1]=='\t')) n--; }
    { Val r=vstr(o,n); free(o); return r; }
}
static Val do_xlate(Val s,Val tab){
    char *o=malloc(s.slen+1); int i,n=0;
    for(i=0;i<s.slen;i++){
        int c=(unsigned char)s.s[i];
        if(c<tab.slen){ unsigned char m=(unsigned char)tab.s[c]; if(m) o[n++]=(char)m; }
    }
    { Val r=vstr(o,n); free(o); return r; }
}
static Val call_userfn(Fn *f,Val arg,int hasarg);

static Val primary(void){
    Token t=peek();

    if(t.kind==T_ID){
        const char *p=t.start; int rem=(int)(limit-p);
        #define FN(l) (rem>=(int)sizeof(l)-1 && !strncasecmp(p,l,sizeof(l)-1))
        if(FN("CHR$(")){ cur=p+5; have_peek=0; Val a=expr(); expect(")");
                         char c=(char)(int)tonum(a); return vstr(&c,1); }
        if(FN("LEFT(")){ cur=p+5; have_peek=0; Val s=expr(); expect(","); Val n=expr(); expect(")");
                         int k=(int)tonum(n); if(k<0)k=0; if(k>s.slen)k=s.slen; return vstr(s.s,k); }
        if(FN("RIGHT(")){cur=p+6; have_peek=0; Val s=expr(); expect(","); Val n=expr(); expect(")");
                         int k=(int)tonum(n); if(k<1)k=1;
                         if(k>s.slen) return vstr("",0); return vstr(s.s+k-1,s.slen-k+1); }
        if(FN("MID(")){  cur=p+4; have_peek=0; Val s=expr(); expect(","); Val a=expr(); expect(",");
                         Val b=expr(); expect(")");
                         int st=(int)tonum(a),ln=(int)tonum(b); if(st<1)st=1;
                         if(st>s.slen||ln<=0) return vstr("",0);
                         if(st-1+ln>s.slen) ln=s.slen-st+1; return vstr(s.s+st-1,ln); }
        if(FN("LEN(")){  cur=p+4; have_peek=0; Val s=expr(); expect(")"); return vnum(s.slen,1); }
        if(FN("INSTR(")){cur=p+6; have_peek=0; Val a=expr(); expect(","); Val b=expr(); expect(",");
                         Val c=expr(); expect(")");
                         int st=(int)tonum(a),i; if(st<1)st=1;
                         if(c.slen==0) return vnum(st,1);
                         for(i=st-1;i+c.slen<=b.slen;i++)
                             if(!memcmp(b.s+i,c.s,c.slen)) return vnum(i+1,1);
                         return vnum(0,1); }
        if(FN("CVT$$(")){cur=p+6; have_peek=0; Val s=expr(); expect(","); Val f=expr(); expect(")");
                         return cvt2(s,(int)tonum(f)); }
        if(FN("SYS(")){  cur=p+4; have_peek=0; Val a=expr(); expect(")"); return call_sys(a); }
        if(FN("ASCII(")){cur=p+6; have_peek=0; Val s=expr(); expect(")");
                         return vnum(s.slen?(unsigned char)s.s[0]:0,1); }
        if(FN("INT(")){  cur=p+4; have_peek=0; Val a=expr(); expect(")"); return vnum(floor(tonum(a)),1); }
        if(FN("ABS(")){  cur=p+4; have_peek=0; Val a=expr(); expect(")"); return vnum(fabs(tonum(a)),a.isint); }
        if(FN("SGN(")){  cur=p+4; have_peek=0; Val a=expr(); expect(")");
                         double d=tonum(a); return vnum(d>0?1:d<0?-1:0,1); }
        if(FN("NUM$(")){ cur=p+5; have_peek=0; Val a=expr(); expect(")");
                         char b[64]; double d=tonum(a);
                         if(d==floor(d)) sprintf(b,"%s%ld ",d<0?"-":" ",labs((long)d));
                         else            sprintf(b,"%s%g ",d<0?"-":" ",fabs(d));
                         return vstr(b,(int)strlen(b)); }
        #undef FN
        /* user-defined function? */
        { int i; for(i=0;i<nfns;i++) if(!strcmp(fns[i].name,t.text)){
              next();
              if(isp(peek(),"(")){ next(); Val a=expr(); expect(")");
                                   return call_userfn(&fns[i],a,1); }
              return call_userfn(&fns[i],vnum(0,1),0); } }
    }

    t=next();
    if(t.kind==T_NUM) return vnum(t.num,t.isint);
    if(t.kind==T_STR) return vstr(t.str,t.slen);
    if(isp(t,"(")){ Val v=expr(); expect(")"); return v; }
    if(isp(t,"-")){ Val v=primary(); return vnum(-tonum(v),v.isint); }
    if(isp(t,"+")) return primary();
    if(isk(t,"NOT")){ Val v=primary(); return vnum(tonum(v)?0:-1,1); }
    if(isk(t,"RND")){
        if(isp(peek(),"(")){ next(); (void)expr(); expect(")"); }
        return vnum((double)rand()/((double)RAND_MAX+1.0),0); }
    if(isk(t,"ERL")) return vnum(errline,1);
    if(isk(t,"ERR")) return vnum(errcode,1);
    if(isk(t,"XLATE")){ expect("("); Val s=expr(); expect(","); Val tb=expr(); expect(")");
                        return do_xlate(s,tb); }
    if(isk(t,"TIME")){ expect("("); (void)expr(); expect(")");
                       { time_t now=time(NULL); struct tm *lt=localtime(&now);
                         return vnum(lt->tm_hour*3600+lt->tm_min*60+lt->tm_sec,1); } }

    if(t.kind==T_ID){
        char nm[10]; strcpy(nm,t.text);
        if(!strcmp(nm,"RND")){
            if(isp(peek(),"(")){ next(); (void)expr(); expect(")"); }
            return vnum((double)rand()/((double)RAND_MAX+1.0),0);
        }
        if(!strcmp(nm,"ERL")) return vnum(errline,1);
        if(!strcmp(nm,"ERR")) return vnum(errcode,1);
        if(isp(peek(),"(")){
            next();
            { Val i=expr(); int j=0,nd=1;
              if(isp(peek(),",")){ next(); Val jj=expr(); j=(int)tonum(jj); nd=2; }
              expect(")");
              { Arr *a=ensure_arr(nm,nd);
                long ix=arr_idx(a,(int)tonum(i),j);
                return a->virt ? vnum(vget(a,ix),1) : a->mem[ix]; } }
        }
        return getvar(nm)->v;
    }
    fatal("Syntax error at line %d",curlineno());
    return vnum(0,1);
}

static Val term(void){
    Val a=primary();
    for(;;){ Token t=peek();
        if(isp(t,"*")){ next(); Val b=primary(); a=vnum(tonum(a)*tonum(b),a.isint&&b.isint); }
        else if(isp(t,"/")){ next(); Val b=primary(); double d=tonum(b);
            if(d==0) rt_error(61);
            a=(a.isint&&b.isint)?vnum((double)(long)(tonum(a)/d),1):vnum(tonum(a)/d,0); }
        else if(isp(t,"^")){ next(); Val b=primary(); a=vnum(pow(tonum(a),tonum(b)),0); }
        else return a; }
}
static Val addsub(void){
    Val a=term();
    for(;;){ Token t=peek();
        if(isp(t,"+")){ next(); Val b=term();
            if(a.isstr||b.isstr){ char*buf=malloc(a.slen+b.slen+1);
                memcpy(buf,a.s,a.slen); memcpy(buf+a.slen,b.s,b.slen);
                a=vstr(buf,a.slen+b.slen); free(buf); }
            else a=vnum(tonum(a)+tonum(b),a.isint&&b.isint); }
        else if(isp(t,"-")){ next(); Val b=term(); a=vnum(tonum(a)-tonum(b),a.isint&&b.isint); }
        else return a; }
}
static Val relation(void){
    Val a=addsub(); Token t=peek();
    if(t.kind==T_PUNC&&(!strcmp(t.text,"=")||!strcmp(t.text,"<")||!strcmp(t.text,">")||
                        !strcmp(t.text,"<=")||!strcmp(t.text,">=")||!strcmp(t.text,"<>"))){
        char op[4]; strcpy(op,t.text); next();
        Val b=addsub(); int c;
        if(a.isstr||b.isstr) c=bp_cmp(a.s,a.slen,b.s,b.slen);
        else { double x=tonum(a),y=tonum(b); c=x<y?-1:x>y?1:0; }
        int r = !strcmp(op,"=")?c==0 : !strcmp(op,"<>")?c!=0 :
                !strcmp(op,"<")?c<0  : !strcmp(op,">")?c>0 :
                !strcmp(op,"<=")?c<=0 : c>=0;
        return vnum(r?-1:0,1);
    }
    return a;
}
static Val andx(void){
    Val a=relation();
    while(isk(peek(),"AND")){ next(); Val b=relation();
        a=vnum((double)(((long)tonum(a))&((long)tonum(b))),1); }
    return a;
}
static Val expr(void){
    Val a=andx();
    while(isk(peek(),"OR")){ next(); Val b=andx();
        a=vnum((double)(((long)tonum(a))|((long)tonum(b))),1); }
    return a;
}

/* ============================================================ output == */

static void outs(const char*s,int n){ fwrite(s,1,n,stdout); }
static void print_val(Val v){
    if(v.isstr){ outs(v.s,v.slen); return; }
    char b[64]; double d=v.num;
    if(d==floor(d)&&fabs(d)<1e15) sprintf(b,"%s%ld ",d<0?"-":" ",labs((long)d));
    else                          sprintf(b,"%s%g ",d<0?"-":" ",fabs(d));
    outs(b,(int)strlen(b));
}

/* ================================================== statement scanning == */

typedef struct { int kind; const char *at; } Mod;   /* 1=IF 2=UNLESS 3=FOR */

static const char *scan_stmt(const char *p,const char *end,Mod *mods,int *nmods,
                             const char **stmt_end,int skip_first_kw){
    int depth=0; const char *body_end=NULL;
    *nmods=0;
    while(p<end){
        if(*p=='"'||*p=='\''){ char q=*p++; while(p<end&&*p!=q)p++; if(p<end)p++; continue; }
        if(*p=='('){ depth++; p++; continue; }
        if(*p==')'){ depth--; p++; continue; }
        if(depth==0){
            if(*p==':'||*p=='\\'){ if(!body_end) body_end=p; *stmt_end=p; return body_end; }
            if(*p=='!'){ if(!body_end) body_end=p; *stmt_end=end; return body_end; }
            if(isalpha((unsigned char)*p)){
                char kw[32]; int n=kw_at(p,end,kw);
                if(n){
                    if(!strcmp(kw,"ELSE")){ if(!body_end) body_end=p; *stmt_end=p; return body_end; }
                    if(!strcmp(kw,"IF")||!strcmp(kw,"UNLESS")||!strcmp(kw,"FOR")){
                        if(skip_first_kw){ skip_first_kw=0; p+=n; continue; }
                        if(!strcmp(kw,"FOR")){        /* OPEN f$ FOR INPUT/OUTPUT */
                            const char *q=p+n; while(q<end&&*q==' ') q++;
                            if((end-q>=5 && !strncasecmp(q,"INPUT",5)) ||
                               (end-q>=6 && !strncasecmp(q,"OUTPUT",6))){ p+=n; continue; }
                        }
                        if(!body_end) body_end=p;
                        if(*nmods<8){ mods[*nmods].kind = !strcmp(kw,"IF")?1:
                                                          !strcmp(kw,"UNLESS")?2:3;
                                      mods[*nmods].at=p; (*nmods)++; }
                    }
                    p+=n; continue;
                }
            }
        }
        p++;
    }
    if(!body_end) body_end=end;
    *stmt_end=end;
    return body_end;
}

static const char *find_else(const char *p,const char *end){
    int depth=0;
    while(p<end){
        if(*p=='"'||*p=='\''){ char q=*p++; while(p<end&&*p!=q)p++; if(p<end)p++; continue; }
        if(isalpha((unsigned char)*p)){
            char kw[32]; int n=kw_at(p,end,kw);
            if(n){
                if(!strcmp(kw,"THEN")) depth++;
                else if(!strcmp(kw,"ELSE")){ if(depth==0) return p; depth--; }
                p+=n; continue;
            }
        }
        p++;
    }
    return NULL;
}

static void rt_error(int code){
    errcode=code; errline=curlineno();
    if(errhandler&&!in_handler) longjmp(err_jmp,1);
    fatal("Error %d at line %d",code,errline);
}

/* ========================================================== statements == */

static void do_print(const char *p,const char *end){
    const char *sp=cur,*sl=limit; int hp=have_peek; Token pk=peeked;
    int nl=1;
    cur=p; limit=end; have_peek=0;
    for(;;){
        Token t=peek();
        if(t.kind==T_EOL) break;
        if(isp(t,";")){ next(); nl=0; continue; }
        if(isp(t,",")){ next(); outs("\t",1); nl=0; continue; }
        { Val v=expr(); print_val(v); nl=1; }
    }
    if(nl) outs("\n",1);
    cur=sp; limit=sl; have_peek=hp; peeked=pk;
}

typedef struct { char name[10]; int has_sub,i,j; } LV;

static LV parse_lv(void){
    LV l; memset(&l,0,sizeof l);
    Token t=next();
    if(t.kind!=T_ID) fatal("Expected a variable at line %d",curlineno());
    strcpy(l.name,t.text);
    if(isp(peek(),"(")){
        next();
        { Val i=expr(); int nd=1;
          l.i=(int)tonum(i);
          if(isp(peek(),",")){ next(); Val j=expr(); l.j=(int)tonum(j); nd=2; }
          expect(")"); l.has_sub=1;
          ensure_arr(l.name,nd); }
    }
    return l;
}

static void assign(LV l,Val v,int lset){
    if(l.has_sub){
        Arr *a=getarr(l.name); long ix=arr_idx(a,l.i,l.j);
        if(a->virt){ vput(a,ix,(int)v.num); return; }
        if(a->isstr){
            if(lset){ int n=a->mem[ix].slen,k; char*b=malloc(n+1);
                      for(k=0;k<n;k++) b[k]= k<v.slen? v.s[k] : ' ';
                      free(a->mem[ix].s); a->mem[ix]=vstr(b,n); free(b); }
            else { free(a->mem[ix].s); a->mem[ix]=vstr(v.s,v.slen); }
        } else a->mem[ix]=vnum(strchr(a->name,'%')?(double)(long)v.num:v.num,
                               strchr(a->name,'%')!=NULL);
        return;
    }
    { Var *var=getvar(l.name);
      if(strchr(l.name,'$')){
          if(lset){ int n=var->v.slen,k; char*b=malloc(n+1);
                    for(k=0;k<n;k++) b[k]= k<v.slen? v.s[k] : ' ';
                    free(var->v.s); var->v=vstr(b,n); free(b); }
          else { free(var->v.s); var->v=vstr(v.s,v.slen); }
      } else var->v=vnum(strchr(l.name,'%')?(double)(long)v.num:v.num,
                         strchr(l.name,'%')!=NULL); }
}

static void open_channel(int ch,const char*name,int mustexist){
    if(ch<0||ch>=MAXCHAN) rt_error(9);
    if(chans[ch].f&&!chans[ch].isterm) fclose(chans[ch].f);
    memset(&chans[ch],0,sizeof(Chan));
    if(!strncasecmp(name,"KB:",3)){ chans[ch].isterm=1; chans[ch].used=1; return; }
    strncpy(chans[ch].name,name,299);
    chans[ch].f=fopen(name,"r+b");
    if(!chans[ch].f){
        if(mustexist) rt_error(5);
        chans[ch].f=fopen(name,"w+b");
        if(!chans[ch].f) rt_error(5);
    }
    chans[ch].used=1;
    /* RSTS/E allocates virtual-array files in 512-byte blocks; pad to match so
       a save file written here is byte-interchangeable with one from RSTS. */
    { long need=0; int i;
      for(i=0;i<progs[curprog].nvd;i++){
          VDecl *d=&progs[curprog].vd[i];
          if(d->chan!=ch) continue;
          { long e=(d->base+(long)(d->d1+1)*(d->ndim==2?(d->d2+1):1))*2;
            if(e>need) need=e; } }
      if(need){
          long blocks=(need+511)/512, want=blocks*512, have;
          fseek(chans[ch].f,0,SEEK_END); have=ftell(chans[ch].f);
          while(have<want){ fputc(0,chans[ch].f); have++; }
          fflush(chans[ch].f); }
    }
}

static int next_data_field(char *out,int max){
    Prog *P=&progs[curprog];
    while(datline<P->n){
        const char *t=P->line[datline].text; const char *p=t;
        while(*p==' ') p++;
        if(strncasecmp(p,"DATA",4)){ datline++; datfield=0; continue; }
        p+=4;
        { const char *q=p; int field=0; int n=(int)strlen(t);
          for(;;){
              const char *s=q;
              while(q<t+n&&*q!=',') q++;
              if(field==datfield){
                  int len=(int)(q-s); if(len>max-1) len=max-1;
                  memcpy(out,s,len); out[len]=0;
                  { int k=0; while(out[k]==' ')k++; if(k) memmove(out,out+k,strlen(out+k)+1); }
                  if(!out[0]) strcpy(out,"0");
                  datfield++;
                  if(q>=t+n){ datline++; datfield=0; } else q++;
                  return 1;
              }
              if(q>=t+n) break;
              q++; field++;
          } }
        datline++; datfield=0;
    }
    return 0;
}

static void do_mat_read(void){
    for(;;){
        Token t=next();
        if(t.kind!=T_ID) fatal("MAT READ wants an array at line %d",curlineno());
        { Arr *a=getarr(t.text); int i,j; char f[64];
          if(!a) fatal("MAT READ: no array %s at line %d",t.text,curlineno());
          if(a->ndim==2){
              for(i=1;i<=a->dim[0];i++) for(j=1;j<=a->dim[1];j++){
                  if(!next_data_field(f,sizeof f)) rt_error(57);
                  if(a->virt) vput(a,arr_idx(a,i,j),atoi(f));
                  else a->mem[arr_idx(a,i,j)]=vnum(atoi(f),1); }
          } else {
              for(i=1;i<=a->dim[0];i++){
                  if(!next_data_field(f,sizeof f)) rt_error(57);
                  if(a->virt) vput(a,arr_idx(a,i,0),atoi(f));
                  else a->mem[arr_idx(a,i,0)]=vnum(atoi(f),1); } } }
        if(isp(peek(),",")){ next(); continue; }
        break;
    }
}

static void do_mat_assign(void){
    Token d=next(); expect("="); Token s=next();
    Arr *A=getarr(d.text), *B=getarr(s.text);
    if(!A||!B) fatal("MAT: unknown array at line %d",curlineno());
    { int i,j;
      if(A->ndim==2){
          for(i=1;i<=A->dim[0]&&i<=B->dim[0];i++)
            for(j=1;j<=A->dim[1]&&j<=B->dim[1];j++){
                int v = B->virt? vget(B,arr_idx(B,i,j)) : (int)B->mem[arr_idx(B,i,j)].num;
                if(A->virt) vput(A,arr_idx(A,i,j),v); else A->mem[arr_idx(A,i,j)]=vnum(v,1); }
      } else {
          for(i=1;i<=A->dim[0]&&i<=B->dim[0];i++){
              int v = B->virt? vget(B,arr_idx(B,i,0)) : (int)B->mem[arr_idx(B,i,0)].num;
              if(A->virt) vput(A,arr_idx(A,i,0),v); else A->mem[arr_idx(A,i,0)]=vnum(v,1); } } }
}

static Val call_userfn(Fn *f,Val arg,int hasarg){
    int s_prog=curprog,s_pc=pc,s_off=lineoff,s_flow=flow;
    const char *s_base=g_line_base,*s_cur=cur,*s_lim=limit; int s_hp=have_peek; Token s_pk=peeked;
    Val saved; int hadparam=0;
    if(hasarg && f->param[0]){ Var *pv=getvar(f->param); saved=pv->v; hadparam=1;
                               { LV l; memset(&l,0,sizeof l); strcpy(l.name,f->param);
                                 assign(l,arg,0); } }
    curprog=f->prog; pc=f->lineidx;
    { const char *txt=progs[curprog].line[pc].text;
      g_line_base=txt; flow=FLOW_NEXT;
      exec_range(txt+f->bodyoff,txt+strlen(txt)); }
    { Val r=getvar(f->name)->v;
      if(hadparam){ Var *pv=getvar(f->param); pv->v=saved; }
      curprog=s_prog; pc=s_pc; lineoff=s_off; flow=s_flow;
      g_line_base=s_base; cur=s_cur; limit=s_lim; have_peek=s_hp; peeked=s_pk;
      return r; }
}

static void exec_with_mods(const char *body,const char *bend,Mod *mods,int nmods,
                           const char *stmt_end){
    if(nmods==0){ exec_one(body,bend); return; }
    { Mod m=mods[nmods-1];
      const char *sp=cur,*sl=stmt_end; int hp=have_peek; Token pk=peeked;
      char kw[32]; int n;
      if(m.kind==1||m.kind==2){
          n=kw_at(m.at,sl,kw);
          cur=m.at+n; limit=sl; have_peek=0;
          { Val c=expr();
            cur=sp; have_peek=hp; peeked=pk;
            if((m.kind==1&&tonum(c)!=0)||(m.kind==2&&tonum(c)==0))
                exec_with_mods(body,bend,mods,nmods-1,stmt_end); }
          return;
      }
      n=kw_at(m.at,sl,kw);
      cur=m.at+n; limit=sl; have_peek=0;
      { Token t=next(); char vn[10]; double a,b,st=1;
        strcpy(vn,t.text); expect("=");
        { Val av=expr(); a=tonum(av); }
        expect("TO");
        { Val bv=expr(); b=tonum(bv); }
        if(isk(peek(),"STEP")){ next(); Val sv=expr(); st=tonum(sv); }
        cur=sp; have_peek=hp; peeked=pk;
        { double i; LV l; memset(&l,0,sizeof l); strcpy(l.name,vn);
          for(i=a; st>0? i<=b : i>=b; i+=st){
              assign(l,vnum(i,1),0);
              exec_with_mods(body,bend,mods,nmods-1,stmt_end);
              if(flow!=FLOW_NEXT) return; } } }
    }
}

static void exec_range(const char *p,const char *end){
    while(p<end&&flow==FLOW_NEXT){
        while(p<end&&(*p==' '||*p=='\t'||*p==':'||*p=='\\')) p++;
        if(p>=end) break;
        if(*p=='!') break;
        { char kw[32]; int n=kw_at(p,end,kw);
          int lead_mod = n&&(!strcmp(kw,"IF")||!strcmp(kw,"UNLESS")||!strcmp(kw,"FOR"));
          Mod mods[8]; int nmods; const char *send;
          const char *bend=scan_stmt(p,end,mods,&nmods,&send,lead_mod);
          g_stmt_after=send;
          if(n&&(!strcmp(kw,"IF")||!strcmp(kw,"UNLESS"))){ exec_one(p,end); return; }
          exec_with_mods(p,bend,mods,nmods,send);
          if(flow!=FLOW_NEXT) return;
          p=send; }
    }
    if(flow==FLOW_ENDLINE) flow=FLOW_NEXT;
}

static void exec_one(const char *p,const char *end){
    const char *sp=cur,*sl=limit; int hp=have_peek; Token pk=peeked;
    const char *after=g_stmt_after;
    cur=p; limit=end; have_peek=0;
    Token t=peek();

    if(isp(t,"&")||isk(t,"PRINT")){ next(); do_print(cur,limit); goto done; }

    if(isk(t,"IF")||isk(t,"UNLESS")){
        int isu=isk(t,"UNLESS"); next();
        Val c=expr();
        int truth = isu ? (tonum(c)==0) : (tonum(c)!=0);
        Token th=peek();
        if(isk(th,"THEN")||isk(th,"GOTO")){
            next();
            { const char *body=cur, *els=find_else(body,limit);
              if(truth){
                  Token nt=peek();
                  if(nt.kind==T_NUM){ next(); flow=FLOW_JUMP; jump_line=(int)nt.num; goto done; }
                  cur=sp; limit=sl; have_peek=hp; peeked=pk;
                  exec_range(body,els?els:end); return;
              } else {
                  if(!els) goto done;
                  { char kw[32]; int n=kw_at(els,end,kw); const char *eb=els+n;
                    cur=eb; limit=end; have_peek=0;
                    { Token nt=peek();
                      if(nt.kind==T_NUM){ next(); flow=FLOW_JUMP; jump_line=(int)nt.num; goto done; } }
                    cur=sp; limit=sl; have_peek=hp; peeked=pk;
                    exec_range(eb,end); return; } } }
        }
        goto done;
    }

    if(isk(t,"ON")){
        next();
        if(isk(peek(),"ERROR")){
            next(); if(isk(peek(),"GOTO")) next();
            { Token nt=peek();
              if(nt.kind==T_NUM){ next(); errhandler=(int)nt.num; } else errhandler=0; }
            goto done;
        }
        { Val v=expr(); int k=(int)tonum(v),i=1,target=0,isgosub=0;
          if(isk(peek(),"GOSUB")) isgosub=1;
          next();
          for(;;){ Token nt=next();
              if(nt.kind!=T_NUM) break;
              if(i==k) target=(int)nt.num;
              i++;
              if(!isp(peek(),",")) break;
              next(); }
          if(target){
              if(isgosub){ gstack[gsp].prog=curprog; gstack[gsp].pc=pc;
                           gstack[gsp].off=(int)(after-g_line_base); gsp++; }
              flow=FLOW_JUMP; jump_line=target; }
          goto done; }
    }

    if(isk(t,"GOTO")){ next(); Val v=expr(); flow=FLOW_JUMP; jump_line=(int)tonum(v); goto done; }
    if(isk(t,"GOSUB")){ next(); Val v=expr();
        gstack[gsp].prog=curprog; gstack[gsp].pc=pc;
        gstack[gsp].off=(int)(after-g_line_base); gsp++;
        flow=FLOW_JUMP; jump_line=(int)tonum(v); goto done; }
    if(isk(t,"RETURN")){ next();
        if(gsp<=0) rt_error(20);
        gsp--; curprog=gstack[gsp].prog;
        resume_pc=gstack[gsp].pc; resume_off=gstack[gsp].off;
        flow=FLOW_RESUME; goto done; }

    if(isk(t,"FOR")){
        next(); Token v=next(); expect("=");
        Val a=expr(); expect("TO"); Val b=expr();
        double st=1;
        if(isk(peek(),"STEP")){ next(); Val s=expr(); st=tonum(s); }
        { LV l; memset(&l,0,sizeof l); strcpy(l.name,v.text); assign(l,a,0); }
        strcpy(fstack[fsp].var,v.text);
        fstack[fsp].limit=tonum(b); fstack[fsp].step=st;
        fstack[fsp].prog=curprog; fstack[fsp].pc=pc;
        fstack[fsp].off=(int)(after-g_line_base);
        fsp++;
        goto done;
    }
    if(isk(t,"NEXT")){
        next(); (void)next();
        if(fsp<=0) rt_error(20);
        { int k=fsp-1; Var *var=getvar(fstack[k].var);
          double nv=var->v.num+fstack[k].step;
          LV l; memset(&l,0,sizeof l); strcpy(l.name,fstack[k].var);
          assign(l,vnum(nv,1),0);
          if(fstack[k].step>0? nv<=fstack[k].limit : nv>=fstack[k].limit){
              curprog=fstack[k].prog; resume_pc=fstack[k].pc; resume_off=fstack[k].off;
              flow=FLOW_RESUME; goto done; }
          fsp--; }
        goto done;
    }

    if(isk(t,"DIM")){
        next();
        { int virt=0,ch=0; long base=0;
          if(isp(peek(),"#")){ next(); Val c=expr(); ch=(int)tonum(c); virt=1;
                               if(isp(peek(),",")) next(); }
          for(;;){
              Token nt=next(); if(nt.kind!=T_ID) break;
              { Arr *a=newarr(nt.text); int nd=1,dd2=0;
                expect("("); { Val d1=expr();
                if(isp(peek(),",")){ next(); Val d2=expr(); dd2=(int)tonum(d2); nd=2; }
                expect(")");
                if(virt){ a->isstr=0; a->ndim=nd; a->dim[0]=(int)tonum(d1); a->dim[1]=dd2;
                          a->virt=1; a->chan=ch; a->base=base; a->mem=NULL;
                          base += (long)(a->dim[0]+1)*(nd==2?(a->dim[1]+1):1); }
                else arr_alloc(a,(int)tonum(d1),dd2,nd); } }
              if(isp(peek(),",")){ next(); continue; }
              break; } }
        goto done;
    }
    if(isk(t,"MAT")){ next();
        if(isk(peek(),"READ")){ next(); do_mat_read(); } else do_mat_assign();
        goto done; }
    if(isk(t,"READ")){ next();
        for(;;){ LV l=parse_lv(); char f[64];
            if(!next_data_field(f,sizeof f)) rt_error(57);
            if(strchr(l.name,'$')) assign(l,vstr(f,(int)strlen(f)),0);
            else assign(l,vnum(atof(f),1),0);
            if(isp(peek(),",")){ next(); continue; } break; }
        goto done; }
    if(isk(t,"DATA")) goto done;
    if(isk(t,"RESTORE")){ next(); datline=0; datfield=0; goto done; }
    if(isk(t,"RANDOM")){ next(); srand((unsigned)time(NULL)); goto done; }

    if(isk(t,"OPEN")){
        next(); Val nm=expr(); int mustexist=0;
        if(isk(peek(),"FOR")){
            next(); have_peek=0; skipsp();
            if(limit-cur>=5 && !strncasecmp(cur,"INPUT",5)) { cur+=5; mustexist=1; }
            else if(limit-cur>=6 && !strncasecmp(cur,"OUTPUT",6)) { cur+=6; mustexist=0; }
        }
        if(isk(peek(),"AS")) next();
        if(isk(peek(),"FILE")) next();
        { Val c=expr(); char nb[300]; int L=nm.slen>299?299:nm.slen;
          memcpy(nb,nm.s,L); nb[L]=0;
          open_channel((int)tonum(c),nb,mustexist); }
        goto done;
    }
    if(isk(t,"CLOSE")){ next();
        for(;;){ Val c=expr(); int ch=(int)tonum(c);
            if(ch>=0&&ch<MAXCHAN){ if(chans[ch].f&&!chans[ch].isterm) fclose(chans[ch].f);
                                   memset(&chans[ch],0,sizeof(Chan)); }
            if(isp(peek(),",")){ next(); continue; } break; }
        goto done; }
    if(isk(t,"KILL")){ next(); Val nm=expr();
        { char nb[300]; int L=nm.slen>299?299:nm.slen; memcpy(nb,nm.s,L); nb[L]=0; remove(nb); }
        goto done; }

    if(isk(t,"GET")||isk(t,"PUT")){
        int isget=isk(t,"GET"); next();
        if(isp(peek(),"#")) next();
        { Val c=expr(); int ch=(int)tonum(c); long rec=1;
          if(isp(peek(),",")) next();
          if(isk(peek(),"RECORD")){ next(); Val r=expr(); rec=(long)tonum(r); }
          if(ch<0||ch>=MAXCHAN||!chans[ch].f) rt_error(9);
          if(isget){ fseek(chans[ch].f,(rec-1)*512,SEEK_SET);
                     chans[ch].buflen=(int)fread(chans[ch].buf,1,512,chans[ch].f);
                     if(chans[ch].buflen<512) memset(chans[ch].buf+chans[ch].buflen,0,
                                                    512-chans[ch].buflen); }
          else { fseek(chans[ch].f,(rec-1)*512,SEEK_SET);
                 fwrite(chans[ch].buf,1,512,chans[ch].f); fflush(chans[ch].f); } }
        goto done;
    }
    if(isk(t,"FIELD")){
        next(); if(isp(peek(),"#")) next();
        { Val c=expr(); int ch=(int)tonum(c),off=0;
          if(isp(peek(),",")) next();
          for(;;){
              Val len=expr(); int L=(int)tonum(len);
              if(isk(peek(),"AS")) next();
              { Token v=next(); LV l; memset(&l,0,sizeof l); strcpy(l.name,v.text);
                if(off+L>512) L=512-off;
                if(L<0) L=0;
                assign(l,vstr((char*)chans[ch].buf+off,L),0);
                off+=L; }
              if(isp(peek(),",")){ next(); continue; }
              break; } }
        goto done;
    }

    if(isk(t,"INPUTLINE")||isk(t,"INPUT")){
        int isline=isk(t,"INPUTLINE"); next();
        if(isk(peek(),"LINE")){ next(); isline=1; }
        { int ich=-1;
          if(isp(peek(),"#")){ next(); Val c=expr(); ich=(int)tonum(c);
                               if(isp(peek(),",")) next(); }
          inch=ich; }
        if(peek().kind==T_STR){ Token pr=next(); outs(pr.str,pr.slen);
                                if(isp(peek(),";")) next(); }
        { char buf[1024]; LV l=parse_lv();
          if(!fgets(buf,sizeof buf,stdin)){ flow=FLOW_END; goto done; }
          { int n=(int)strlen(buf); while(n&&(buf[n-1]=='\n'||buf[n-1]=='\r')) buf[--n]=0; }
          /* The original console was a KSR: upper case only.  Folding
             keyboard input matches the real terminal, and makes the Y/N
             prompt (3080) and the password (40) accept either case.
             Typed commands were already case-insensitive via
             CVT$$(A$,188%), whose bit 32 upper-cases.  BPKEEPCASE=1 off. */
          if(!keepcase && (inch<0 || (inch<MAXCHAN && chans[inch].isterm))){
              int k; for(k=0;buf[k];k++) buf[k]=(char)upc(buf[k]); }
          if(!isline){ int k=0; while(buf[k]==' ')k++; if(k) memmove(buf,buf+k,strlen(buf+k)+1); }
          if(strchr(l.name,'$')) assign(l,vstr(buf,(int)strlen(buf)),0);
          else assign(l,vnum(atof(buf),1),0); }
        goto done;
    }
    if(isk(t,"LSET")||isk(t,"RSET")){ next();
        { LV l=parse_lv(); expect("="); Val v=expr(); assign(l,v,1); } goto done; }

    if(isk(t,"CHAIN")){
        next(); Val nm=expr(); int ln=0;
        { Token nt=peek();
          if(nt.kind==T_NUM||nt.kind==T_ID||isp(nt,"(")){ Val v=expr(); ln=(int)tonum(v); } }
        { char base[300]; int i,L=nm.slen>299?299:nm.slen;
          memcpy(base,nm.s,L); base[L]=0;
          { char *b=strrchr(base,']'); if(b) memmove(base,b+1,strlen(b+1)+1); }
          { char *d=strrchr(base,'.'); if(d) *d=0; }
          for(i=0;i<nprogs;i++) if(!strcasecmp(progs[i].name,base)) break;
          if(i>=nprogs) fatal("CHAIN: %s is not loaded (line %d)",base,curlineno());
          vars=NULL; arrs=NULL; gsp=0; fsp=0; datline=0; datfield=0;
          curprog=i; install_vdims(i);
          if(ln){ int k=find_line(i,ln);
                  if(k<0) fatal("Statement not found: CHAIN %s %d",base,ln);
                  resume_pc=k; } else resume_pc=0;
          resume_off=0; flow=FLOW_RESUME; goto done; }
    }
    if(isk(t,"RESUME")){ next(); in_handler=0;
        { Token nt=peek();
          if(nt.kind==T_NUM){ next(); flow=FLOW_JUMP; jump_line=(int)nt.num; }
          else { flow=FLOW_JUMP; jump_line=errline; } }
        goto done; }
    if(isk(t,"END")||isk(t,"STOP")){ next(); flow=FLOW_END; goto done; }
    if(isk(t,"DEF")){ flow=FLOW_ENDLINE; goto done; }   /* body runs only when called */
    if(isk(t,"FNEND")){ next(); flow=FLOW_ENDLINE; goto done; }
    if(isk(t,"LET")) next();

    {   LV tgt[8]; int n=0;
        for(;;){ tgt[n++]=parse_lv();
            if(isp(peek(),",")){ next(); continue; } break; }
        expect("=");
        { Val v=expr(); int i; for(i=0;i<n;i++) assign(tgt[i],v,0); }
        goto done;
    }
done:
    cur=sp; limit=sl; have_peek=hp; peeked=pk;
}

/* ============================================================== loader == */

static void load_prog(const char *path){
    FILE *f=fopen(path,"rb"); char buf[8192];
    if(!f) fatal("cannot open %s",path);
    { Prog *P=&progs[nprogs];
      const char *b=path,*b2;
      if((b2=strrchr(b,'\\'))) b=b2+1;
      if((b2=strrchr(b,'/')))  b=b2+1;
      strncpy(P->name,b,63);
      { char *d=strrchr(P->name,'.'); if(d) *d=0; }
      P->n=0;
      while(fgets(buf,sizeof buf,f)){
          int n=(int)strlen(buf);
          while(n&&(buf[n-1]=='\n'||buf[n-1]=='\r')) buf[--n]=0;
          { char *p=buf; while(*p==' '||*p=='\t') p++;
            if(*p=='\\'){                 /* BASIC-PLUS statement continuation */
                if(P->n==0) continue;
                { char *old=P->line[P->n-1].text;
                  int lo=(int)strlen(old), ln=(int)strlen(p+1);
                  char *nw=malloc(lo+ln+2);
                  memcpy(nw,old,lo); nw[lo]=':'; memcpy(nw+lo+1,p+1,ln); nw[lo+1+ln]=0;
                  free(old); P->line[P->n-1].text=nw; }
                continue; }
            if(!isdigit((unsigned char)*p)) continue;
            { int num=atoi(p);
              while(isdigit((unsigned char)*p)) p++;
              while(*p==' ') p++;
              if(P->n>=MAXLINES) fatal("too many lines in %s",path);
              P->line[P->n].num=num;
              P->line[P->n].text=xdup(p,(int)strlen(p));
              P->n++; } }
      }
      fclose(f); nprogs++; }
}


/* A BASIC-PLUS virtual array is not a variable: DIM #n declares a static view
   onto a file, belonging to the program.  CHAIN clears variables, so UNDERG's
   re-entry at 995 never re-runs the DIM on line 10 -- yet the arrays are still
   there, because the declaration is part of the program.  So we record the
   declarations per program at load time and re-install them on entry. */
static void scan_vdims(Prog *P){
    int li;
    for(li=0; li<P->n; li++){
        const char *t=P->line[li].text, *p=t;
        int inq=0;
        for(; *p; p++){
            if(*p==34||*p==39){ char q=*p++; while(*p&&*p!=q) p++; if(!*p) break; continue; }
            if(!inq && (upc(p[0])=='D'&&upc(p[1])=='I'&&upc(p[2])=='M')){
                const char *q=p+3; while(*q==' ') q++;
                if(*q!='#') continue;
                cur=q; limit=t+strlen(t); have_peek=0;
                { Token tk=next(); (void)tk;                 /* '#' */
                  Val c=expr(); int ch=(int)c.num; long base=0;
                  if(isp(peek(),",")) next();
                  for(;;){
                      Token nt=next(); if(nt.kind!=T_ID) break;
                      { VDecl *d=&P->vd[P->nvd]; int nd=1,dd2=0;
                        if(P->nvd>=8) break;
                        expect("(");
                        { Val d1=expr();
                          if(isp(peek(),",")){ next(); Val d2=expr(); dd2=(int)d2.num; nd=2; }
                          expect(")");
                          strcpy(d->name,nt.text); d->ndim=nd;
                          d->d1=(int)d1.num; d->d2=dd2; d->chan=ch; d->base=base;
                          base += (long)(d->d1+1)*(nd==2?(d->d2+1):1);
                          P->nvd++; } }
                      if(isp(peek(),",")){ next(); continue; }
                      break;
                  } }
                break;
            }
        }
    }
}
static void install_vdims(int pi){
    int i;
    for(i=0;i<progs[pi].nvd;i++){
        VDecl *d=&progs[pi].vd[i];
        Arr *a=newarr(d->name);
        a->isstr=0; a->ndim=d->ndim; a->dim[0]=d->d1; a->dim[1]=d->d2;
        a->virt=1; a->chan=d->chan; a->base=d->base; a->mem=NULL;
    }
}

/* find DEF FN definitions so calls can jump into them */
static void scan_functions(void){
    int pi,li;
    for(pi=0;pi<nprogs;pi++)
        for(li=0;li<progs[pi].n;li++){
            const char *t=progs[pi].line[li].text;
            const char *p=t; while(*p==' ') p++;
            if(strncasecmp(p,"DEF",3)) continue;
            p+=3; while(*p==' ') p++;
            { Fn *f=&fns[nfns]; int i=0;
              if(nfns>=MAXFN) return;
              while(*p&&!strchr("( :",*p)&&i<9) f->name[i++]=(char)upc(*p++);
              f->name[i]=0;
              f->param[0]=0;
              while(*p==' ') p++;
              if(*p=='('){ p++; i=0;
                  while(*p&&*p!=')'&&i<9) f->param[i++]=(char)upc(*p++);
                  f->param[i]=0; if(*p==')') p++; }
              while(*p==' '||*p==':') p++;
              f->prog=pi; f->lineidx=li; f->bodyoff=(int)(p-t);
              nfns++; }
        }
}

/* ================================================================ main == */

static int trace;

int main(int argc,char **argv){
    int i;
    trace = getenv("BPTRACE")!=NULL;
    keepcase = getenv("BPKEEPCASE")!=NULL;
    setvbuf(stdout,NULL,_IONBF,0);
    srand((unsigned)time(NULL));

    if(argc>1){ for(i=1;i<argc && nprogs<MAXPROGS;i++) load_prog(argv[i]); }
    else { load_prog("UNDERG.BAS"); load_prog("LOOK.BAS"); }
    scan_functions();
    for(i=0;i<nprogs;i++) scan_vdims(&progs[i]);

    curprog=0; pc=0; lineoff=0;
    install_vdims(0);

    if(setjmp(err_jmp)){
        in_handler=1;
        { int k=find_line(curprog,errhandler);
          if(k<0) fatal("Error %d at line %d (no handler %d)",errcode,errline,errhandler);
          pc=k; lineoff=0; }
    }

    for(;;){
        if(pc<0||pc>=progs[curprog].n) break;
        { const char *txt=progs[curprog].line[pc].text;
          int len=(int)strlen(txt);
          if(lineoff>len) lineoff=len;
          g_line_base=txt;
          if(trace) fprintf(stderr,"[%s %d%s]\n",progs[curprog].name,
                            progs[curprog].line[pc].num,
                            lineoff?"+":"");
          flow=FLOW_NEXT;
          exec_range(txt+lineoff,txt+len);
          lineoff=0;
          if(flow==FLOW_END) break;
          if(flow==FLOW_JUMP){
              int k=find_line(curprog,jump_line);
              if(k<0){ errline=curlineno(); rt_error(19); }
              pc=k;
          } else if(flow==FLOW_RESUME){
              pc=resume_pc; lineoff=resume_off;
          } else pc++;
        }
    }
    return 0;
}
