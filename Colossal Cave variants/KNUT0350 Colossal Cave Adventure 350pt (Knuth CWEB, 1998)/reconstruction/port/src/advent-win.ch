Change file for the Windows console port of ADVENT (KNUT0350).

advent.w asks not to be changed, so it is used exactly as it came
(src_original/KNUT0350/advent.w: Knuth's text with the errata from his
web site to January 2020) and every port change lives here:

    ctangle advent.w advent-win.ch advent.c

Knuth's own corrections since then are in the errata to pages 235-394
of Selected Papers on Fun and Games (his fg.html); where one of them is
the change made here, it says so.

What the 1998 program never needed on a Unix terminal:

  1. End of input (Ctrl-Z Enter, a closed pipe) ends the run.  fgets
     returning NULL left the old buffer in place, so the program either
     repeated the last command or said "Tell me to do something." for ever.
  2. Command-line options: --seed N starts the random number generator
     from N instead of the clock, so a session can be replayed exactly;
     --no-fixes turns the fixes below off; --debug turns on the # commands
     the tests use; --help lists them.  The # commands refuse numbers
     that would index past the program's tables (a location outside 1 to
     max_loc, a prop outside the object's notes, deaths past max_deaths)
     and keep the liquids with the bottle, as TAKE and DROP do.
Two reads outside the program's arrays, mended whatever the options, since
what the program found there depended on how the compiler laid out the
globals:

  3. listen() never steps past the end of the typed text.  When a word
     ended exactly at the end of the buffer (71 characters and no newline)
     it read the byte after the buffer's terminating NUL.  This is Knuth's
     own erratum for page 317 (21 January 2022): "p++;" becomes ";" twice.
  4. SAY with a word the program does not know ("say blorp") looked up
     the word, got -1, and read hash_table[-1] to see whether it was a
     magic word; now an unknown word is no magic word, and the answer is
     'Okay, "blorp".', as it was here before whenever that byte happened
     to be harmless.  (Found by UndefinedBehaviorSanitizer; not in Knuth's
     errata, and still in his text of 5 June 2026.)

Fixes, all off with --no-fixes:

  Fix 1  The ranks as Woods had them: a score equal to a class's value
         belongs to that class, and "you need N more points" counts to one
         point past it.  Knuth's loop moved every exact score one class up,
         so 349 of 350 was already "Adventure Grandmaster ... would be a
         neat trick!".  Knuth's erratum for page 382 (6 October 2018 and
         29 April 2019), reported by Q P Liu; the same fix in O'Dwyer's
         ODWY0350 is Quuxplusone/Advent commit 531e061.
  Fix 2  When the cave closes, the liquid in a carried bottle goes to
         limbo before the destroy loop, as the death code does ("must not
         drop them"): destroying it through drop() took one off the count
         of things carried, so the endgame let you carry eight.  Woods's
         bug, carried over; Knuth's erratum for page 371 (9 February
         2020), on the recommendation of Arthur O'Dwyer, whose own fix in
         ODWY0350 is Quuxplusone/Advent commit ea27c76.
  Fix 3  The rest of a line too long for the 72-byte buffer is dropped.
         fgets left it in the input, where it became the next command: a
         long line ending in "north" moved you.  Whether the line filled
         the buffer is told by a newline put in its next-to-last byte
         before each read, which fgets overwrites only if it does: a
         search for the newline would stop at a NUL typed in the line.

@x l.108 main() gets its arguments (and is int main, as in Knuth's 2026 text)
main()
{
@y
int main(argc,argv)
  int argc; char *argv[];
{
@z

@x l.2424 the port's helpers, ahead of yes()
boolean yes @,@,@[ARGS((char*,char*,char*))@];
@y
int fixes=1; /* 0 with --no-fixes: the program as this advent.w has it */
int debug_mode; /* 1 with --debug: the commands of debug_command */
int score(void);

void drain_line(void) /* Fix 3: drop the rest of an over-long line */
{
  register int c;
  while ((c=getchar())!=EOF && c!='\n') ;
}

int debug_object(char *w) /* a number, or a word naming an object */
{
  char low[16];
  register int h,i;
  if (isdigit(*w) || *w=='-') return atoi(w);
  for (i=0;i<16;i++) low[i]='\0'; /* |lookup| looks at |low[5]| */
  for (i=0;w[i] && i<15;i++) low[i]=tolower(w[i]);
  h=lookup(low);
  if (h>=0 && hash_table[h].word_type==object_type) return hash_table[h].meaning;
  return -1000;
}

int debug_notes(int t) /* the notes from |offset[t]| up to the next object's:
                          the objects are not built in the order of their
                          numbers, and one built with no notes shares the
                          next one's */
{
  register int u,k=note_ptr;
  for (u=1;u<=max_obj;u++) if (offset[u]>offset[t] && offset[u]<k) k=offset[u];
  return k-offset[t];
}

void debug_liquid(void) /* the liquid goes where the bottle goes, as in
                           the program's own |TAKE| and |DROP| */
{
  place[WATER]=place[OIL]=limbo;
  if (toting(BOTTLE) && !bottle_empty) place[prop[BOTTLE]? OIL: WATER]=inhand;
}

void debug_command(char *b) /* --debug: a line typed with a leading hash sign */
{
  char cmd[16],w[16];
  long v=0;
  register int t,n,k;
  n=sscanf(b+1,"%15s %15s %ld",cmd,w,&v);
  if (n>=1 && strcmp(cmd,"show")==0) {
    printf("# loc %d, holding %d, tally %d, lost %d, dflag %d, clock1 %d, \
clock2 %d, deaths %d, bonus %d, score %d\n",loc,holding,tally,lost_treasures,
      dflag,clock1,clock2,death_count,bonus,score());
    return;
  }
  if (n>=2 && strcmp(cmd,"loc")==0) {
    t=atoi(w);
    if (t<1 || t>max_loc) printf("# ?  a location is 1 to %d\n",max_loc);
    else {
      loc=oldloc=oldoldloc=newloc=t;
      printf("# loc %d\n",loc);
    }
    return;
  }
  if ((n>=2 && strcmp(cmd,"where")==0) ||
      (n==3 && (strcmp(cmd,"put")==0 || strcmp(cmd,"prop")==0))) {
    t=debug_object(w);
    if (t<1 || t>max_obj) {
      printf("# ?  an object is 1 to %d, or its name\n",max_obj);
      return;
    }
    if (strcmp(cmd,"where")==0)
      printf("# object %d: place %d, prop %d\n",t,place[t],prop[t]);
    else if (strcmp(cmd,"put")==0) {
      if (t==WATER || t==OIL) printf("# ?  the liquids go where the bottle goes\n");
      else if (v<inhand || v>max_loc)
        printf("# ?  a place is -1 (carried), 0 (nowhere) or 1 to %d\n",max_loc);
      else {
        move(t,v);
        if (t==BOTTLE) debug_liquid();
        printf("# object %d: place %d, holding %d\n",t,place[t],holding);
      }
    } else {
      k=debug_notes(t)-1;
      if (k<0) k=0;
      if (t==PLANT && (v<0 || v>4 || (v&1))) /* watering prints |note[prop+1]|,
                                                and past prop 4 there is none */
        printf("# ?  the plant's prop is 0, 2 or 4\n");
      else if ((v<0 || v>k) && !(v==-1 && is_treasure(t)))
        printf("# ?  object %d's prop is %s0 to %d\n",t,is_treasure(t)? "-1 or ": "",k);
      else {
        prop[t]=v;
        if (t==BOTTLE) debug_liquid();
        printf("# object %d: prop %d\n",t,prop[t]);
      }
    }
    return;
  }
  if (n==3 && strcmp(cmd,"set")==0) {
    if (strcmp(w,"deaths")==0 && (v<0 || v>=max_deaths)) {
      printf("# ?  deaths is 0 to %d\n",max_deaths-1);
      return;
    }
    if (strcmp(w,"tally")==0) tally=v;
    else if (strcmp(w,"lost")==0) lost_treasures=v;
    else if (strcmp(w,"dflag")==0) dflag=v;
    else if (strcmp(w,"clock1")==0) clock1=v;
    else if (strcmp(w,"clock2")==0) clock2=v;
    else if (strcmp(w,"deaths")==0) death_count=v;
    else if (strcmp(w,"bonus")==0) bonus=v;
    else if (strcmp(w,"limit")==0) limit=v;
    else goto huh;
    printf("# %s %ld\n",w,v);
    return;
  }
huh: printf("# ?  show, loc N, where OBJ, put OBJ LOC, prop OBJ N, \
set tally/lost/dflag/clock1/clock2/deaths/bonus/limit N\n");
}

boolean yes @,@,@[ARGS((char*,char*,char*))@];
@z

@x l.2430 end of input while waiting for yes or no
    fgets(buffer,buf_size,stdin);
@y
    buffer[buf_size-2]='\n'; /* fgets writes over it only if the line fills
                                the buffer (a NUL typed in it would stop
                                |strchr|) */
    if (!fgets(buffer,buf_size,stdin)) {
      printf("\n");@+ exit(0);
    }
    if (fixes && buffer[buf_size-2]!='\n') drain_line();
@z

@x l.2451 end of input while waiting for a command; the debug commands
    fgets(buffer,buf_size,stdin);
@y
    buffer[buf_size-2]='\n';
    if (!fgets(buffer,buf_size,stdin)) {
      printf("\n");@+ exit(0);
    }
    if (fixes && buffer[buf_size-2]!='\n') drain_line();
    if (debug_mode && *buffer=='#') {
      debug_command(buffer);@+ continue;
    }
@z

@x l.2461 never step past the end of the typed text (Knuth's erratum, 2022)
    for (p++; isspace(*p); p++) ;
    if (*p==0) {
@y
    for (; isspace(*p); p++) ;
    if (*p==0) {
@z

@x l.2468 nor after the second word
    for (p++; isspace(*p); p++) ;
    if (*p==0) return;
@y
    for (; isspace(*p); p++) ;
    if (*p==0) return;
@z

@x l.2859 SAY with a word the program does not know: lookup gave -1
 switch (hash_table[k].meaning) {
@y
 switch (k<0? -1: hash_table[k].meaning) {
@z

@x l.3658 options: --seed N, --no-fixes, --debug, --help
rx=(((int)time(NULL))&0xfffff)|1;
@y
rx=(((int)time(NULL))&0xfffff)|1;
{
  register int a;
  for (a=1;a<argc;a++) {
    if (strcmp(argv[a],"--seed")==0 || strncmp(argv[a],"--seed=",7)==0) {
      char *s=(argv[a][6]=='=')? argv[a]+7: (a+1<argc)? argv[++a]: "";
      char *e=s;
      if (*e=='-' || *e=='+') e++;
      if (!isdigit(*e)) {
        fprintf(stderr,"advent: --seed takes a number (try --help)\n");
        exit(2);
      }
      while (isdigit(*e)) e++;
      if (*e) {
        fprintf(stderr,"advent: --seed takes a number (try --help)\n");
        exit(2);
      }
      rx=(atoi(s)&0xfffff)|1;
    } else if (strcmp(argv[a],"--no-fixes")==0) fixes=0;
    else if (strcmp(argv[a],"--debug")==0) debug_mode=1;
    else if (strcmp(argv[a],"-h")==0 || strcmp(argv[a],"--help")==0) {
      printf("usage: advent [--seed N] [--no-fixes] [--debug] [-h]\n\n");
      printf("  --seed N    start the random numbers from N instead of the clock,\n");
      printf("              so that the same commands replay the same game\n");
      printf("  --no-fixes  the program as this advent.w has it, without the\n");
      printf("              three fixes README.md lists\n");
      printf("  --debug     commands for testing, typed with a leading hash\n");
      printf("              sign: show, loc N, where OBJ, put OBJ LOC, prop OBJ N,\n");
      printf("              set VARIABLE N\n");
      printf("  -h, --help  show this help\n\n");
      printf("ADVENT by Don Woods and Don Knuth (CWEB, 1998).  Type QUIT to stop,\n");
      printf("or end the input (Ctrl-Z, Enter) to leave at once.\n");
      exit(0);
    } else {
      fprintf(stderr,"advent: unknown option '%s' (try --help)\n",argv[a]);
      exit(2);
    }
  }
}
@z

@x l.4076 Fix 2: a carried bottle's liquid when the cave closes
  for (j=1;j<=max_obj;j++) if (toting(j)) destroy(j);
@y
  if (fixes) place[WATER]=limbo,place[OIL]=limbo; /* as at a death */
  for (j=1;j<=max_obj;j++) if (toting(j)) destroy(j);
@z

@x l.4393 Fix 1: the ranks as Woods had them
for (j=0;class_score[j]<=k;j++);
printf("%s\nTo achieve the next higher rating",class_message[j]);
if (j<highest_class)
  printf(", you need %d more point%s.\n",class_score[j]-k,
                 class_score[j]==k+1? "": "s");
@y
if (fixes) for (j=0;class_score[j]<k;j++);
else for (j=0;class_score[j]<=k;j++);
printf("%s\nTo achieve the next higher rating",class_message[j]);
if (j<highest_class && fixes)
  printf(", you need %d more point%s.\n",class_score[j]+1-k,
                 class_score[j]==k? "": "s");
else if (j<highest_class)
  printf(", you need %d more point%s.\n",class_score[j]-k,
                 class_score[j]==k+1? "": "s");
@z
