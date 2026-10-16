#define main edax_original_main
#include "../src/all.cpp"
#undef main
static unsigned rng=0x651197;
static unsigned rr(void){rng^=rng<<13;rng^=rng>>17;return rng^=rng<<5;}
static unsigned long long ref_flip(Board b,int x){
 if((b.player|b.opponent)&(1ULL<<x))return 0;
 unsigned long long ans=0;int r=x/8,c=x%8;
 for(int dr=-1;dr<=1;++dr)for(int dc=-1;dc<=1;++dc)if(dr||dc){int y=r+dr,z=c+dc;unsigned long long run=0;
 while(y>=0&&y<8&&z>=0&&z<8){unsigned long long q=1ULL<<(y*8+z);if(b.opponent&q)run|=q;else{if(b.player&q)ans|=run;break;}y+=dr;z+=dc;}}
 return ans;
}
static unsigned long long ref_moves(Board b){unsigned long long m=0;for(int x=0;x<64;++x)if(ref_flip(b,x))m|=1ULL<<x;return m;}
static Board ref_next(Board b,int x){unsigned long long f=ref_flip(b,x);Board q={b.opponent^f,b.player^f^(1ULL<<x)};return q;}
static int ref_solve(Board b,int alpha,int beta){unsigned long long m=ref_moves(b);if(!m){Board q={b.opponent,b.player};if(ref_moves(q))return -ref_solve(q,-beta,-alpha);int d=bit_count(b.player)-bit_count(b.opponent),e=64-bit_count(b.player|b.opponent);return d>0?d+e:d<0?d-e:0;}int best=-65;while(m){int x=first_bit(m);m&=m-1;int v=-ref_solve(ref_next(b,x),-beta,-alpha);if(v>best)best=v;if(v>alpha)alpha=v;if(alpha>=beta)break;}return best;}
static const char *eval_path(void) { const char *p = getenv("EDAX_EVAL_FILE"); return p ? p : "data/eval.dat"; }
static void init_search(Search *s,int tasks){options.n_task=tasks;options.hash_table_size=10;options.verbosity=0;options.cpu_affinity=false;eval_open(eval_path());search_global_init();search_init(s);}
static Board late_board(int empties){Board b;board_init(&b);while(board_count_empties(&b)>empties){unsigned long long m=ref_moves(b);if(!m){board_pass(&b);m=ref_moves(b);if(!m)break;}int k=rr()%bit_count(m);while(k--)m&=m-1;b=ref_next(b,first_bit(m));}return b;}
static void core(int tasks){Search s;init_search(&s,tasks);unsigned long long checks=0;int exact=0,passes=0;for(int g=0;g<300;++g){Board b;board_init(&b);for(int ply=0;ply<120;++ply){unsigned long long m=ref_moves(b);if(m!=board_get_moves(&b)){puts("FAIL moves");exit(2);}for(int x=0;x<64;++x)if(!((b.player|b.opponent)&(1ULL<<x))){Move mv;unsigned long long f=board_get_move_flip(&b,x,&mv);++checks;if(f!=ref_flip(b,x)){puts("FAIL flips");exit(2);}}
 Board unique;board_unique(&b,&unique);for(int sym=0;sym<8;++sym){Board transformed,u;board_symetry(&b,sym,&transformed);board_unique(&transformed,&u);if(!board_equal(&unique,&u)){puts("FAIL symmetry");exit(2);}}
 if(!m){board_pass(&b);++passes;if(!ref_moves(b))break;continue;}int k=rr()%bit_count(m);while(k--)m&=m-1;int x=first_bit(m);Board next=ref_next(b,x),actual;board_next(&b,x,&actual);if(!board_equal(&next,&actual)){puts("FAIL next");exit(2);}Eval e={0},fresh={0};e.n_empties=board_count_empties(&b);eval_set(&e,&b);eval_update(x,ref_flip(b,x),&e);fresh.n_empties=board_count_empties(&next);eval_set(&fresh,&next);if(memcmp(&e.feature,&fresh.feature,sizeof e.feature)){puts("FAIL incremental eval");exit(2);}b=next;}}
 for(int i=0;i<96;++i){Board b=late_board(1+i%12);int want=ref_solve(b,-65,65);search_cleanup(&s);search_set_board(&s,&b,BLACK);search_set_level(&s,60,board_count_empties(&b));search_run(&s);if(s.result->score!=want){printf("FAIL exact tasks=%d empty=%d expected=%d actual=%d\n",tasks,board_count_empties(&b),want,s.result->score);exit(2);}++exact;}
 printf("PASS core tasks=%d games=300 flip_checks=%llu symmetries=all8 exact_positions=%d empties=1..12 pass_events=%d\n",tasks,checks,exact,passes);search_free(&s);}
int main(int argc,char **argv){if(argc<2)return 1;setbuf(stdout,NULL);bit_init();edge_stability_init();const char *t=argv[1];
#if !defined(hasSSE2) && (defined(USE_GAS_X86) || defined(USE_MSVC_X86))
 if(getenv("EDAX_NO_SSE2")){hasSSE2=false;puts("(EDAX_NO_SSE2: the code for processors without SSE2)");} /* x86 builds that choose SSE2 at run time */
#endif
 if(!strcmp(t,"bool")){bool b=false;options_read("book-usage","abcdef");printf("value=%d\n",b);}
 else if(!strcmp(t,"short")){Base b;base_init(&b);char *p=(char*)malloc(2);p[0]='x';p[1]=0;base_load(&b,p);free(p);base_free(&b);}
 else if(!strcmp(t,"ggf")){Game g;FILE *f=tmpfile();fputs("(;GM[Othello]PB[",f);for(int i=0;i<1000;++i)fputc('a',f);fputs("];)",f);rewind(f);game_import_ggf(&g,f);fclose(f);}
 else if(!strcmp(t,"edx")){Game g;Base b,c;remove("audit-roundtrip.edx");game_init(&g);g.move[0]=26;g.date.month=10;base_init(&b);base_init(&c);base_append(&b,&g);base_save(&b,"audit-roundtrip.edx");base_load(&c,"audit-roundtrip.edx");printf("EDX roundtrip expected_games=1 actual=%d expected_firstmove=26 actual=%d\n",c.n_games,c.n_games?c.game[0].move[0]:-1);if(c.n_games!=1||c.game[0].move[0]!=26)return 2;base_free(&b);base_free(&c);}
 else if(!strcmp(t,"savefail")){Book b={0};book_init(&b);int ok=book_save(&b,"audit-directory-does-not-exist/book.dat");book_free(&b);printf("BOOK save to missing directory accepted=%d\n",ok);if(ok)return 2;}
 else if(!strcmp(t,"deviatesave")){
  const char *path="audit-deviate-save.dat";Book b={0};book_init(&b);int rounds=0;FILE *f;
  remove(path);options_read("book-deviate-save-rounds","0");
  book_deviate_save_progress(&b,path,1,&rounds);book_deviate_save_progress(&b,path,1,&rounds);
  if((f=fopen(path,"rb"))!=NULL){fclose(f);return 2;}
  book_deviate_save_progress(&b,path,0,&rounds);
  if((f=fopen(path,"rb"))==NULL)return 2;fclose(f);remove(path);
  options_read("book-deviate-save-rounds","10");rounds=0;
  for(int i=0;i<9;++i)book_deviate_save_progress(&b,path,1,&rounds);
  if((f=fopen(path,"rb"))!=NULL){fclose(f);return 2;}
  book_deviate_save_progress(&b,path,1,&rounds);
  if((f=fopen(path,"rb"))==NULL)return 2;fclose(f);remove(path);
  book_deviate_save_progress(&b,path,1,&rounds);book_deviate_save_progress(&b,path,0,&rounds);
  if((f=fopen(path,"rb"))==NULL)return 2;fclose(f);remove(path);
  options_read("book-deviate-save-rounds","1");rounds=0;
  book_deviate_save_progress(&b,path,1,&rounds);
  if((f=fopen(path,"rb"))==NULL)return 2;fclose(f);remove(path);
  book_free(&b);puts("PASS deviate save cadence: final-only, every 10 rounds, final remainder, and every round");
 }
 else if(!strcmp(t,"reproducible")){
  /* book-seed: the random numbers of a book start from the seed; auto and a wrong value leave it to the clock */
  Book r={0};Random want;options_read("book-seed","12345");book_init(&r);random_seed(&want,12345);int ok=options.book_seed_set&&r.random.x==want.x;book_free(&r);
  options_read("book-seed","auto");ok=ok&&!options.book_seed_set;options_read("book-seed","-1");ok=ok&&!options.book_seed_set;options_read("book-seed","1x");ok=ok&&!options.book_seed_set;
  printf("REPRODUCIBLE seed option ok=%d\n",ok);if(!ok)return 2;
  /* the links of a new position are checked against the book before its expansion is applied */
  Search s;init_search(&s,4);Book b={0};b.search=&s;book_init(&b);Board start;board_init(&start);
  Position c,d;position_init(&c);c.board=start;position_link(&c,&b);int fresh=position_links_current(&c,&b);
  position_init(&d);board_next(&start,F5,&d.board);d.score.value=3;position_unique(&d);if(book_add(&b,&d)<=0)return 2;
  int stale_new=!position_links_current(&c,&b);position_link(&c,&b);int relinked=position_links_current(&c,&b)&&c.n_link==4; /* (the 4 first moves give the same position) */
  for(int i=0;i<c.n_link;++i)relinked=relinked&&position_links(&c)[i].score==-3;
  Board after;board_next(&start,F5,&after);book_probe(&b,&after)->score.value=5;int stale_score=!position_links_current(&c,&b);position_free(&c);book_free(&b);
  printf("REPRODUCIBLE links check: fresh=%d new_position=%d relinked=%d new_score=%d\n",fresh,stale_new,relinked,stale_score);if(!fresh||!stale_new||!relinked||!stale_score)return 2;
  /* the same book with 4 threads (expansions applied in the order of the list) and with 1 (one after the other) */
  options_read("book-reproducible","on");options.book_file=(char*)"audit-repro.dat";
  const char *out[3]={"audit-repro-4a.dat","audit-repro-4b.dat","audit-repro-1.dat"};const int tasks[3]={4,4,1};unsigned char *bytes[3];long size[3];
  /* (a shallow book: positions down to 52 empties, 8 moves; with 36 empties the book grew to hundreds of MB) */
  for(int k=0;k<3;++k){options.n_task=tasks[k];book_new(&b,2,52);book_deviate(&b,&start,2,4);if(!book_save(&b,out[k]))return 2;printf("REPRODUCIBLE n_task=%d positions=%d\n",tasks[k],b.n_nodes);book_free(&b);
   FILE *f=fopen(out[k],"rb");if(!f)return 2;fseek(f,0,SEEK_END);size[k]=ftell(f);rewind(f);bytes[k]=(unsigned char*)malloc(size[k]);if(fread(bytes[k],1,size[k],f)!=(size_t)size[k])return 2;fclose(f);remove(out[k]);}
  remove("audit-repro.dat.dev");
  for(int k=1;k<3;++k){int same=size[k]==size[0]&&!memcmp(bytes[k],bytes[0],10)&&!memcmp(bytes[k]+18,bytes[0]+18,size[0]-18);
   printf("REPRODUCIBLE %s against %s: same except the date=%d (%ld, %ld bytes)\n",out[k],out[0],same,size[k],size[0]);if(!same)return 2;}
  printf("REPRODUCIBLE expansions searched again (their links changed): %lld\n",book_test_expand_redo);
  for(int k=0;k<3;++k)free(bytes[k]);options_read("book-reproducible","off");search_free(&s);
  puts("PASS reproducible: book-seed, links check, the same book with 4 threads (twice) and 1 thread");
 }
 else if(!strcmp(t,"expandredo")){
  /* book-reproducible: the expansion 1 is searched before the expansion 0, whose new position is a move of the new position of 1:
     when 0 is applied, 1 has old links, is searched again, then applied; the book is the one of position_expand (0, then 1) */
  Search s;init_search(&s,1);options_read("book-reproducible","on");
  Board x0=late_board(40),x,c0;unsigned long long m0=board_get_moves(&x0);if(!m0)return 2;int a=first_bit(m0);board_next(&x0,a,&x);
  unsigned long long m1=board_get_moves(&x);if(!m1)return 2;int m=first_bit(m1);board_next(&x,m,&c0);
  Book b[2]={{0}};
  for(int k=0;k<2;++k){b[k].search=&s;book_init(b+k);b[k].options.level=2;b[k].options.n_empties=20;
   Position p;position_init(&p);p.board=x;p.level=2;p.leaf.move=(unsigned char)m;p.leaf.score=0;p.score.value=0;position_unique(&p);if(book_add(b+k,&p)<=0)return 2;
   position_init(&p);p.board=x0;p.level=2;p.leaf.move=(unsigned char)a;p.leaf.score=0;p.score.value=0;position_unique(&p);if(book_add(b+k,&p)<=0)return 2;}
  /* the reference: one after the other */
  position_expand(book_probe(b+1,&x),b+1);position_expand(book_probe(b+1,&x0),b+1);
  /* the same expansions, applied in their order after being searched in the other order */
  ExpandShared sh;memset(&sh,0,sizeof sh);sh.book=b;sh.action="Expand test";sh.tmp_file="audit-expandredo.tmp";sh.t=real_clock();sh.n_slot=4;sh.slot=(ExpandSlot*)calloc(4,sizeof *sh.slot);for(int i=0;i<4;++i)sh.slot[i].index=-1;
  lock_init(&sh);condition_init(&sh);
  b[0].todo_list.item=(decltype(b[0].todo_list.item))malloc(2*sizeof *b[0].todo_list.item);b[0].todo_list.n=b[0].todo_list.size=2;b[0].todo_list.valid=true;
  b[0].todo_list.item[0]=position_key(b,book_probe(b,&x));b[0].todo_list.item[1]=position_key(b,book_probe(b,&x0));
  Search *search=book_search(b);ExpandSlot *e0=sh.slot,*e1=sh.slot+1;
  for(int i=0;i<2;++i){ExpandSlot *e=sh.slot+i;e->index=i;e->skip=false;e->ready=false;if(!book_expand_take(&sh,book_position(b,b[0].todo_list.item[i]),&e->parent,&e->child))return 2;}sh.next=2;
  int links_before=e1->child.n_link;
  e1->r=book_expand_search(search,&e1->parent,&e1->child,&e1->leaf);e1->ready=true;Link stale_leaf=e1->leaf,stale_parent=e1->parent.leaf;book_expand_apply_ready(&sh);int waited=sh.applied==0&&sh.redo==NULL;
  e0->r=book_expand_search(search,&e0->parent,&e0->child,&e0->leaf);e0->ready=true;book_expand_apply_ready(&sh);int redo=sh.applied==1&&sh.redo==e1&&e1->child.n_link==links_before+1;
  sh.redo=NULL;e1->r=book_expand_search(search,&e1->parent,&e1->child,&e1->leaf);e1->ready=true;
  printf("EXPANDREDO expansion 1 with the old links: link %d:%d, next leaf %d:%d; searched again: link %d:%d, next leaf %d:%d\n",stale_leaf.move,stale_leaf.score,stale_parent.move,stale_parent.score,e1->leaf.move,e1->leaf.score,e1->parent.leaf.move,e1->parent.leaf.score);
  book_expand_apply_ready(&sh);int applied=sh.applied==2;
  printf("EXPANDREDO waited_for_0=%d searched_again=%d (links %d -> %d) applied=%d positions=%d/%d\n",waited,redo,links_before,links_before+1,applied,b[0].n_nodes,b[1].n_nodes);
  int same=b[0].n_nodes==b[1].n_nodes;const Board *boards[3]={&x0,&x,&c0};
  for(int i=0;i<3;++i){Position *p=book_probe(b,boards[i]),*q=book_probe(b+1,boards[i]);if(!p||!q){same=0;continue;}
   int eq=p->leaf.move==q->leaf.move&&p->leaf.score==q->leaf.score&&p->score.value==q->score.value&&p->n_link==q->n_link;
   for(int j=0;eq&&j<p->n_link;++j){int f=0;for(int k=0;k<q->n_link;++k)f|=position_links(p)[j].move==position_links(q)[k].move&&position_links(p)[j].score==position_links(q)[k].score;eq=f;}
   printf("EXPANDREDO position %d: leaf %d:%d / %d:%d, score %d / %d, links %d / %d, same=%d\n",i,p->leaf.move,p->leaf.score,q->leaf.move,q->leaf.score,p->score.value,q->score.value,p->n_link,q->n_link,eq);same=same&&eq;}
  free(sh.slot);condition_free(&sh);lock_free(&sh);book_free(b);book_free(b+1);options_read("book-reproducible","off");search_free(&s);remove("audit-expandredo.tmp");
  if(!waited||!redo||!applied||!same)return 2;
  puts("PASS expandredo: an expansion whose links changed is searched again, and the book is the one of position_expand");
 }
 else if(!strcmp(t,"analyze")){Search s;init_search(&s,1);Game g;game_init(&g);g.initial_board=late_board(4);printf("ANALYZE custom initial empty=%d moves=0\n",board_count_empties(&g.initial_board));fflush(stdout);game_analyze(&g,&s,4,false);search_free(&s);}
 else if(!strcmp(t,"enhance")){Search s;init_search(&s,1);Book b={0};b.search=&s;book_init(&b);b.options.level=0;b.options.n_empties=58;options.book_file=(char*)"audit-enhance.dat";Board start;board_init(&start);book_add_board(&b,&start);book_enhance(&b,&start,64,64);printf("ENHANCE nodes_after=%d\n",b.n_nodes);book_clean(&b);position_negamax(book_probe(&b,&start),&b);book_clean(&b);position_enhance(book_probe(&b,&start),&b);int pending=0;Position *p;PositionArray *a;Book *bp=&b;foreach_position(p,a,bp)pending+=position_is_todo(p,bp);printf("ENHANCE pending_after_finished=%d\n",pending);book_free(&b);search_free(&s);}
 else if(!strcmp(t,"core")){core(argc>2?atoi(argv[2]):1);}
 else if(!strcmp(t,"nboard")){Result result={0};result.n_nodes=1234;result.time=12345;nboard_observer(&result);}
 else if(!strcmp(t,"gtp")||!strcmp(t,"gtptime")||!strcmp(t,"gtpreg")||!strcmp(t,"xboard")){
 options.n_task=1;options.hash_table_size=10;options.verbosity=0;options.cpu_affinity=false;options.book_file=(char*)"tests/merge-complete-fixture.dat";
 eval_open(eval_path());search_global_init();UI *ui=(UI*)mm_malloc(sizeof *ui);memset(ui,0,sizeof *ui);event_init(&ui->event);
 if(!strcmp(t,"xboard")){ui->type=UI_XBOARD;ui_init_xboard(ui);event_add_message(&ui->event,string_duplicate("level 0 5 1"));event_add_message(&ui->event,string_duplicate("quit"));ui_loop_xboard(ui);ui_free_xboard(ui);}
 else {ui->type=UI_GTP;ui_init_gtp(ui);if(!strcmp(t,"gtp")){event_add_message(&ui->event,string_duplicate("42 play white d3"));event_add_message(&ui->event,string_duplicate("7 name"));event_add_message(&ui->event,string_duplicate("name"));}
 else if(!strcmp(t,"gtpreg")){options.level=0;event_add_message(&ui->event,string_duplicate("reg_genmove black"));} else{ui->play->force.real[0].opponent=0x12345678;printf("GTP before forced_board=%llx\n",ui->play->force.real[0].opponent);event_add_message(&ui->event,string_duplicate("time_left banana 99 0"));}
 event_add_message(&ui->event,string_duplicate("quit"));ui_loop_gtp(ui);if(!strcmp(t,"gtptime"))printf("GTP after forced_board=%llx\n",ui->play->force.real[0].opponent);ui_free_gtp(ui);}
 event_free(&ui->event);mm_free(ui);
 }
 else if(!strcmp(t,"formats")){Game src,dst;Random random;random_seed(&random,1234);game_rand(&src,20,&random);src.date.year=2026;src.date.month=9;src.date.day=27;src.date.hour=1;src.date.minute=2;src.date.second=3;
 void (*exports[4])(const Game*,FILE*)={game_export_text,game_export_ggf,game_export_sgf,game_export_pgn};void (*imports[4])(Game*,FILE*)={game_import_text,game_import_ggf,game_import_sgf,game_import_pgn};const char *names[]={"txt","ggf","sgf","pgn"};
 for(int j=0;j<4;++j){FILE *f=tmpfile();exports[j](&src,f);rewind(f);imports[j](&dst,f);printf("FORMAT %s board_equal=%d moves_equal=%d date=%d/%d/%d %d:%d:%d\n",names[j],board_equal(&src.initial_board,&dst.initial_board),memcmp(src.move,dst.move,60)==0,dst.date.year,dst.date.month,dst.date.day,dst.date.hour,dst.date.minute,dst.date.second);fclose(f);}
 game_init(&src);src.initial_board=late_board(4);FILE *f=tmpfile();game_export_pgn(&src,f);rewind(f);game_import_pgn(&dst,f);printf("PGN_CUSTOM expected_empty=%d actual_empty=%d board_equal=%d\n",board_count_empties(&src.initial_board),board_count_empties(&dst.initial_board),board_equal(&src.initial_board,&dst.initial_board));fclose(f);
 }
 else if(!strcmp(t,"pgnlong")){Game g;FILE *f=tmpfile();fputs("[Event \"audit\"]\n",f);for(int i=0;i<300;++i)fprintf(f,"%d. d3 c3 ",i+1);rewind(f);game_import_pgn(&g,f);fclose(f);}
 else if(!strcmp(t,"pgnfen")){Game g;Board want=late_board(12);char fen[128];FILE *f=tmpfile();fprintf(f,"[FEN \"%s\"]\n*\n",board_to_FEN(&want,BLACK,fen));rewind(f);game_import_pgn(&g,f);printf("PGN FEN expected_empty=%d actual_empty=%d\n",board_count_empties(&want),board_count_empties(&g.initial_board));fclose(f);}
 else if(!strcmp(t,"unsupported")){FILE *f=fopen("audit-sentinel.xyz","wb");fputs("KEEP THIS FILE",f);fclose(f);Play *p=(Play*)mm_malloc(sizeof *p);memset(p,0,sizeof *p);board_init(&p->initial_board);p->initial_player=BLACK;play_save(p,"audit-sentinel.xyz");f=fopen("audit-sentinel.xyz","rb");fseek(f,0,SEEK_END);printf("UNSUPPORTED SAVE bytes_before=14 bytes_after=%ld\n",ftell(f));fclose(f);mm_free(p);}
 else if(!strcmp(t,"bookio")){Book b={0},c={0};FILE *f=fopen("audit-book-saved.dat","wb");fputs("KEEP",f);fclose(f);if(!book_load(&b,"tests/merge-complete-fixture.dat")||b.n_nodes!=2)return 2;if(!book_save(&b,"audit-book-saved.dat"))return 2;if(!book_load(&c,"audit-book-saved.dat")||c.n_nodes!=2)return 2;printf("BOOK save and reload nodes=%d\n",c.n_nodes);book_free(&b);book_free(&c);}
 else if(!strcmp(t,"evalslots")){options.verbosity=0;eval_open(eval_path());long checks=0,xchecks=0;
#if EVAL_SLOTS > 48
Eval_weight (*w0)[EVAL_N_2PLY-1]=EVAL_WEIGHT,(*w1)[EVAL_N_2PLY-1]=(Eval_weight(*)[EVAL_N_2PLY-1])malloc(sizeof *EVAL_WEIGHT);memcpy(w1,w0,sizeof *EVAL_WEIGHT);for(int p=0;p<EVAL_N_2PLY-1;++p)for(int i=0;i<4;++i)for(int k=0;k<59049;++k){(*w1)[p].X[i][k][0]=(short)(rr()%201)-100;(*w1)[p].X[i][k][1]=(short)(rr()%201)-100;}
#endif
for(int g=0;g<300;++g){Board b;board_init(&b);Eval e;e.n_empties=60;eval_set(&e,&b);for(int ply=0;ply<80;++ply){unsigned long long m=board_get_moves(&b);if(!m){Board q={b.opponent,b.player};if(!board_get_moves(&q))break;b=q;eval_pass(&e);}else{int k=rr()%bit_count(m);while(k--)m&=m-1;int x=first_bit(m);Move mv;board_get_move_flip(&b,x,&mv);eval_update(x,mv.flipped,&e);board_update(&b,&mv);--e.n_empties;}Eval r;r.n_empties=e.n_empties;eval_set(&r,&b);bool same=true;for(int i=0;i<EVAL_SLOTS;++i)if((i<46||i>=48)&&r.feature.us[i]!=e.feature.us[i])same=false;if(!same){printf("FAIL update game %d ply %d\n",g,ply);return 2;}++checks;
#if EVAL_SLOTS > 48
Board pb=b;if(e.n_empties&1){pb.player=b.opponent;pb.opponent=b.player;}for(int i=0;i<16;++i){int v=0;for(int j=0;j<10;++j)v=v*3+board_get_square_color(&pb,EVAL_X_SQUARE[i][j]);if(e.feature.us[48+i]!=v){printf("FAIL extra feature %d game %d ply %d\n",i,g,ply);return 2;}}{int pl=60-e.n_empties,b0=pl&1,p=pl-2;if(p<0)p=0;p>>=1;if(p>=EVAL_N_2PLY-1)p=EVAL_N_2PLY-2;int ex=0;for(int i=0;i<16;++i)ex+=(*w1)[p].X[i/4][e.feature.us[48+i]][b0];EVAL_WEIGHT=w0;int s0=accumlate_eval(pl,&e);EVAL_WEIGHT=w1;int s1=accumlate_eval(pl,&e);EVAL_WEIGHT=w0;if(s1-s0!=ex){printf("FAIL extra weights game %d ply %d: %d - %d != %d\n",g,ply,s1,s0,ex);return 2;}++xchecks;}
#endif
}}printf("EVAL slots=%d update checks=%ld extra checks=%ld\n",EVAL_SLOTS,checks,xchecks);
#if EVAL_SLOTS > 48
{if(!eval_convert(eval_path(),"audit-eval64-13.dat"))return 2;FILE *f=fopen("audit-eval64-13.dat","rb");unsigned char h13[EVAL2_HEADER_SIZE(EVAL2_N_GROUP)];if(fread(h13,1,sizeof h13,f)!=sizeof h13)return 2;unsigned short np;memcpy(&np,h13+36,2);short *wp=(short*)malloc((size_t)np*EVAL_N_PACKED*2);if(fread(wp,2,(size_t)np*EVAL_N_PACKED,f)!=(size_t)np*EVAL_N_PACKED)return 2;fclose(f);const int nx=EVAL_N_PACKED+4*EVAL_N_X;short *w=(short*)malloc((size_t)np*nx*2);for(int p=0;p<np;++p){memcpy(w+(size_t)p*nx,wp+(size_t)p*EVAL_N_PACKED,EVAL_N_PACKED*2);for(int k=EVAL_N_PACKED;k<nx;++k)w[(size_t)p*nx+k]=(short)((int)(rr()%2001)-1000);}unsigned char h[EVAL2_HEADER_SIZE(EVAL2_N_GROUP_ALL)]={};memcpy(h,h13,EVAL2_GROUPS_AT);unsigned short size=sizeof h,ng=EVAL2_N_GROUP_ALL;unsigned int nw=nx;memcpy(h+10,&size,2);memcpy(h+40,&nw,4);memcpy(h+44,&ng,2);eval_groups_write(h+EVAL2_GROUPS_AT,EVAL2_N_GROUP_ALL);unsigned int crc=crc32c_bytes(0,h,size-4);memcpy(h+size-4,&crc,4);crc=crc32c_bytes(0,w,(size_t)np*nx*2);f=fopen("audit-eval64.dat","wb");fwrite(h,1,size,f);fwrite(w,2,(size_t)np*nx,f);fwrite(&crc,4,1,f);fwrite(EVAL2_END,1,4,f);fclose(f);eval_close();eval_open("audit-eval64.dat");long bad=0;for(int ply=2;ply<EVAL_N_2PLY*2;++ply){const Eval_weight *pe=&(*EVAL_WEIGHT)[(ply-2)>>1];const short *wx=w+(size_t)ply*nx+EVAL_N_PACKED;for(int i=0;i<4;++i)for(int k=0;k<EVAL_N_X;++k)if(pe->X[i][k][ply&1]!=wx[i*EVAL_N_X+((ply&1)?OPPONENT_FEATURE[k]:k)])++bad;}int refused=!eval_convert("audit-eval64.dat","audit-eval64-1.dat");printf("EVAL file with the extra groups: wrong weights=%ld, form 1 refused=%d\n",bad,refused);if(bad||!refused)return 2;eval_close();eval_open(eval_path());free(w);free(wp);free(w1);}
#endif
}
 else if(!strcmp(t,"bookform2")){Book b={0},c={0},d={0};if(!book_load(&b,"tests/merge-complete-fixture.dat")||b.n_nodes!=2||b.file_format!=1)return 2;options.book_format=2;if(!book_save(&b,"audit-form2.dat"))return 2;if(!book_load(&c,"audit-form2.dat")||c.n_nodes!=2||c.file_format!=2)return 2;options.book_format=0;if(!book_save(&c,"audit-form2b.dat"))return 2;options.book_format=1;if(!book_save(&c,"audit-form1.dat"))return 2;options.book_format=0;if(!book_load(&d,"audit-form2b.dat")||d.file_format!=2)return 2;book_free(&d);FILE *f=fopen("tests/merge-complete-fixture.dat","rb"),*g=fopen("audit-form1.dat","rb");int same=1;long i=0;for(int x,y;;++i){x=fgetc(f);y=fgetc(g);if(x!=y&&(i<10||i>17))same=0;if(x==EOF||y==EOF){if(x!=y)same=0;break;}}fclose(f);fclose(g);printf("FORM2 reload nodes=%d form1 again same=%d\n",c.n_nodes,same);if(!same)return 2;f=fopen("audit-form2.dat","rb");fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);unsigned char *s=(unsigned char*)malloc(n);if(fread(s,1,n,f)!=(size_t)n)return 2;fclose(f);const long at[4]={30,70,n-10,n-1};int refused=0;for(int k=0;k<5;++k){g=fopen("audit-form2-bad.dat","wb");if(k<4){s[at[k]]^=1;fwrite(s,1,n,g);s[at[k]]^=1;}else fwrite(s,1,n-1,g);fclose(g);Book e={0};if(!book_load(&e,"audit-form2-bad.dat"))++refused;book_free(&e);}free(s);printf("FORM2 damaged files refused=%d/5\n",refused);if(refused!=5)return 2;if(!book_convert("audit-form2.dat","audit-conv1.dat")||!book_load(&d,"audit-conv1.dat")||d.file_format!=1||d.n_nodes!=2)return 2;book_free(&d);puts("FORM2 convert ok");book_free(&b);book_free(&c);}
 else if(!strcmp(t,"evalform2")){if(!eval_convert(eval_path(),"audit-eval2.dat")||!eval_convert("audit-eval2.dat","audit-eval1.dat"))return 2;FILE *f=fopen(eval_path(),"rb"),*g=fopen("audit-eval1.dat","rb");int same=1;for(int x,y;;){x=fgetc(f);y=fgetc(g);if(x!=y){same=0;break;}if(x==EOF)break;}fclose(f);fclose(g);eval_open(eval_path());size_t z=sizeof *EVAL_WEIGHT;void *w=malloc(z);memcpy(w,EVAL_WEIGHT,z);eval_close();eval_open("audit-eval2.dat");int equal=memcmp(w,EVAL_WEIGHT,z)==0;eval_close();free(w);printf("EVAL form2 round trip same=%d weights equal=%d\n",same,equal);if(!same||!equal)return 2;}
 else if(!strcmp(t,"truncated")){FILE *in=fopen("tests/merge-complete-fixture.dat","rb"),*out=fopen("audit-truncated.dat","wb");fseek(in,0,SEEK_END);long n=ftell(in);rewind(in);for(long i=0;i<n-1;++i)fputc(fgetc(in),out);fclose(in);fclose(out);Book b={0};int ok=book_load(&b,"audit-truncated.dat");printf("TRUNCATED accepted=%d nodes=%d original_expected=2\n",ok,b.n_nodes);book_free(&b);if(ok)return 2;}
 else if(!strcmp(t,"corruptstartup")){FILE *in=fopen("tests/merge-complete-fixture.dat","rb"),*out=fopen("audit-truncated.dat","wb");fseek(in,0,SEEK_END);long n=ftell(in);rewind(in);for(long i=0;i<n-1;++i)fputc(fgetc(in),out);fclose(in);fclose(out);options.n_task=1;options.hash_table_size=10;options.verbosity=0;options.cpu_affinity=false;options.book_file=(char*)"audit-truncated.dat";eval_open(eval_path());search_global_init();UI *ui=(UI*)mm_malloc(sizeof *ui);memset(ui,0,sizeof *ui);event_init(&ui->event);ui->type=UI_GTP;ui_init_gtp(ui);int safe=!ui->book.need_saving;ui_free_gtp(ui);event_free(&ui->event);mm_free(ui);in=fopen("audit-truncated.dat","rb");fseek(in,0,SEEK_END);long after=ftell(in);fclose(in);printf("CORRUPT STARTUP autosave_disabled=%d original_bytes=%ld after_bytes=%ld\n",safe,n-1,after);if(!safe||after!=n-1)return 2;}
 else if(!strcmp(t,"ggsclock")){GGSClock c;char input[WORD_SIZE+1];memset(input,'1',WORD_SIZE);input[WORD_SIZE]=0;ggs_parse_clock(&c,input);}
 else if(!strcmp(t,"cassio")){options.n_task=1;options.hash_table_size=10;options.verbosity=0;options.cpu_affinity=false;eval_open(eval_path());search_global_init();Engine *e=(Engine*)calloc(1,sizeof *e);e->search=engine_create_search();for(int i=0;i<24;++i){Board b=late_board(3+i%7);char text[80];int want=ref_solve(b,-65,65);for(int repeat=0;repeat<2;++repeat){int got=engine_endgame_search(e,board_to_string(&b,BLACK,text),-64,64,100);if(got!=want){printf("FAIL Cassio expected=%d actual=%d\n",want,got);return 2;}}}engine_free(e->search);free(e);puts("PASS Cassio endgame API 24 positions x 2 repeated searches");}
 else if(!strcmp(t,"events")){Event ev;event_init(&ev);int expected=0;for(int i=0;i<10000;++i){char b[32];sprintf(b,"%d",i);event_add_message(&ev,string_duplicate(b));if(i%3==0){char *s=event_peek_message(&ev);if(!s||atoi(s)!=expected++)return 2;free(s);}}char *s;while((s=event_peek_message(&ev))){if(atoi(s)!=expected++)return 2;free(s);}printf("PASS event FIFO %d messages with wrap and growth\n",expected);event_free(&ev);}
 else if(!strcmp(t,"missinglink")){Search s;init_search(&s,1);Book b={0};if(!book_load(&b,"tests/merge-complete-fixture.dat")||b.n_nodes!=2)return 2;b.search=&s;Position *root=book_root(&b),*child=NULL,*p;PositionArray *a;Book *bp=&b;foreach_position(p,a,bp)if(p!=root)child=p;if(!root||!child||root->n_link==0)return 2;{PositionArray *ca=b.array+(board_get_hash_code(&child->board)&(b.n-1));book_remove_at(&b,ca,(int)(child-ca->positions));}book_clean(&b);position_negamax(book_root(&b),&b);book_fix(&b);root=book_root(&b);printf("MISSING LINK nodes=%d root_links_after_fix=%d\n",b.n_nodes,root->n_link);if(b.n_nodes!=1||root->n_link!=0)return 2;book_free(&b);search_free(&s);}
 else if(!strcmp(t,"mergefile")){FILE *in=fopen("tests/merge-complete-fixture.dat","rb"),*out=fopen("audit-merge-truncated.dat","wb");fseek(in,0,SEEK_END);long n=ftell(in);rewind(in);for(long i=0;i<n-1;++i)fputc(fgetc(in),out);fclose(in);fclose(out);Book d={0};book_init(&d);int ok=book_merge_file(&d,"audit-merge-truncated.dat");printf("MERGE truncated_source accepted=%d destination_nodes=%d\n",ok,d.n_nodes);if(ok||d.n_nodes!=0)return 2;ok=book_merge_file(&d,"tests/merge-complete-fixture.dat");printf("MERGE complete_source accepted=%d destination_nodes=%d\n",ok,d.n_nodes);if(!ok||d.n_nodes!=2)return 2;book_free(&d);}
 else if(!strcmp(t,"linkstore")){Position p;Link l;int i,ok;position_init(&p);for(i=0;i<7;++i){l.move=(unsigned char)i;l.score=(signed char)i;position_add_link(&p,&l);}ok=p.n_link==7&&position_links(&p)[6].move==6;{Link keep[3]={{1,1},{3,3},{5,5}};position_set_links(&p,keep,3);}ok=ok&&p.n_link==3&&position_links(&p)[2].move==5;for(i=10;i<13;++i){l.move=(unsigned char)i;l.score=0;position_add_link(&p,&l);}ok=ok&&p.n_link==6&&position_links(&p)[0].move==1&&position_links(&p)[5].move==12;position_free(&p);printf("LINK storage inline/heap ok=%d\n",ok);if(!ok)return 2;}
 else if(!strcmp(t,"mergetwice")){ /* a second merge must not take the positions of the first one for duplicated positions; a merge that fails must only remove what it added */
  Book s={0},d={0};Position p;Board b;FILE *f;long n;unsigned char *data;unsigned int count=2;int ok;
  book_init(&s);board_init(&b);position_init(&p);board_unique(&b,&p.board);p.level=1;if(book_add(&s,&p)<=0)return 2;
  b=late_board(40);position_init(&p);board_unique(&b,&p.board);p.level=1;if(book_add(&s,&p)<=0)return 2; /* a position out of reach from the root */
  if(!book_save(&s,"audit-merge-a.dat"))return 2;book_free(&s);
  book_init(&s);b=late_board(38);position_init(&p);board_unique(&b,&p.board);p.level=1;if(book_add(&s,&p)<=0)return 2;if(!book_save(&s,"audit-merge-one.dat"))return 2;book_free(&s);
  f=fopen("audit-merge-one.dat","rb");if(f==NULL)return 2;fseek(f,0,SEEK_END);n=ftell(f);rewind(f);data=(unsigned char*)malloc(n);if(fread(data,1,n,f)!=(size_t)n)return 2;fclose(f);
  memcpy(data+38,&count,4);f=fopen("audit-merge-dup.dat","wb");fwrite(data,1,n,f);fwrite(data+42,1,n-42,f);fclose(f);free(data); /* the same position twice */
  book_init(&d);
  ok=book_merge_file(&d,"audit-merge-a.dat");printf("MERGE first accepted=%d nodes=%u\n",ok,d.n_nodes);if(!ok||d.n_nodes!=2)return 2;
  ok=book_merge_file(&d,"audit-merge-a.dat");printf("MERGE same book again accepted=%d nodes=%u\n",ok,d.n_nodes);if(!ok||d.n_nodes!=2)return 2;
  ok=book_merge_file(&d,"audit-merge-dup.dat");printf("MERGE book with a duplicated position accepted=%d nodes=%u\n",ok,d.n_nodes);if(ok||d.n_nodes!=2)return 2;
  book_free(&d);remove("audit-merge-a.dat");remove("audit-merge-one.dat");remove("audit-merge-dup.dat");}
 else if(!strcmp(t,"negamaxloop")){ /* damaged book: a link that leads back to its own position must not make the negamax with threads endless */
  Book b={0};Position p;Board board;Link l={0,PASS};
  book_init(&b);board_init(&board);position_init(&p);board_unique(&board,&p.board);p.level=1;position_add_link(&p,&l);if(book_add(&b,&p)<=0)return 2;
  options.n_task=4;book_negamax(&b);printf("NEGAMAX with a link that leads back: done, nodes=%u\n",b.n_nodes);book_free(&b);}
 else if(!strcmp(t,"learnline")){ /* the first moves of a game to learn are copied without their spaces: a long line is not cut between two moves */
  char line[400],copy[LEARN_MOVES_SIZE];int i,ok;
  strcpy(line,"F5d6");for(i=4;i<253;++i)line[i]=' ';strcpy(line+253,"c3g5");
  learn_moves_copy(copy,line);ok=!strcmp(copy,"f5d6c3g5");printf("LEARN line of %d bytes copied as \"%s\" ok=%d\n",(int)strlen(line),copy,ok);if(!ok)return 2;}
 else if(!strcmp(t,"hashlock")){ /* v4.5.5-nikque.16: the hash codes of the same entries take the same lock (the lock was hash_code & lock_mask) */
  HashTable h={0};int bad=0;hash_init(&h,1024);
  for(unsigned long long k=0;k<100000;++k){unsigned long long c=k*0x9E3779B97F4A7C15ULL;for(int d=1;d<HASH_N_WAY;++d)if(hash_lock(&h,c)!=hash_lock(&h,c^(unsigned long long)d))++bad;}
  printf("HASH lock of the same entries: n_lock=%d hash_mask=%llu different=%d\n",h.n_lock,h.hash_mask,bad);hash_cleanup(&h);free(h.memory);if(bad)return 2;}
 else if(!strcmp(t,"lastgame")){ /* v4.5.5-nikque.16: the last game of a file without an end of line, and of a pgn file, is loaded */
  const char *ext[]={".txt",".pgn",".ggf",".sgf",".edx"};const char *line[]={"f5d6c3d3c4f4f6f3e6e7d7g6g5f7e8","f5f6e6f4e3c5c4","f5d6c5f4e3"};int ok=1;
  for(int e=0;e<5;++e){char name[32];Base b;Game g;FILE *f;long n;char *buf;sprintf(name,"audit-lastgame%s",ext[e]);remove(name);base_init(&b);
   for(int i=0;i<3;++i){game_init(&g);text_to_game(line[i],&g);base_append(&b,&g);}base_save(&b,name);base_free(&b);
   f=fopen(name,"rb");fseek(f,0,SEEK_END);n=ftell(f);rewind(f);buf=(char*)malloc(n);if(fread(buf,1,n,f)!=(size_t)n)n=0;fclose(f);if(e<4)while(n>0&&(buf[n-1]=='\n'||buf[n-1]=='\r'))--n;
   f=fopen(name,"wb");fwrite(buf,1,n,f);fclose(f);free(buf);base_init(&b);int loaded=base_load(&b,name);printf("BASE %s without the final end of line: loaded=%d games=%d (expected 3)\n",ext[e],loaded,b.n_games);
   if(!loaded||b.n_games!=3||b.game[2].move[4]!=g.move[4]){ok=0;}base_free(&b);remove(name);}
  if(!ok)return 2;}
 else if(!strcmp(t,"basereplace")){ /* v4.5.5-nikque.16: base correct/complete save to another file first, then replace the file */
  Base b;Game g;FILE *f;int ok;remove("audit-replace.txt");remove("audit-replace.txt.saving.txt");f=fopen("audit-replace.txt","w");fputs("f5d6\nf5f6\nf5d6c3",f);fclose(f);
  base_init(&b);ok=base_load(&b,"audit-replace.txt")&&b.n_games==3;game_init(&g);text_to_game("f5f4",&g);b.game[0]=g;ok=ok&&base_replace(&b,"audit-replace.txt");base_free(&b);
  base_init(&b);ok=ok&&base_load(&b,"audit-replace.txt")&&b.n_games==3&&b.game[0].move[1]==g.move[1];base_free(&b);f=fopen("audit-replace.txt.saving.txt","r");if(f){fclose(f);ok=0;}
  printf("BASE replace: 3 games, first one changed, no file left aside ok=%d\n",ok);remove("audit-replace.txt");if(!ok)return 2;}
 else if(!strcmp(t,"keepunread")){ /* v4.5.5-nikque.16: a book file that could not be loaded is kept as .damaged before a book saved to another file replaces it */
  Search s;init_search(&s,1);Book b={0};FILE *f;int ok,kept;remove("audit-unread.dat");remove("audit-unread.dat.damaged");remove("audit-unread.dat.saving");
  f=fopen("audit-unread.dat","wb");for(int i=0;i<300;++i)fputc(i*7,f);fclose(f);b.search=&s;ok=!book_load(&b,"audit-unread.dat");book_free(&b);
  book_init(&b);b.search=&s;book_new(&b,2,36);ok=ok&&book_save(&b,"audit-unread.dat.saving");ok=ok&&book_keep_unread_file("audit-unread.dat");
  f=fopen("audit-unread.dat.damaged","rb");kept=f!=NULL;if(f){fseek(f,0,SEEK_END);kept=ftell(f)==300;fclose(f);}ok=ok&&kept&&rename("audit-unread.dat.saving","audit-unread.dat")==0;
  ok=ok&&book_keep_unread_file("audit-unread.dat");f=fopen("audit-unread.dat.damaged.1","rb");if(f){fclose(f);ok=0;} /* (once kept, the file is no longer the one that could not be loaded) */
  printf("BOOK unread file kept as .damaged (300 bytes)=%d ok=%d\n",kept,ok);book_free(&b);remove("audit-unread.dat");remove("audit-unread.dat.damaged");search_free(&s);if(!ok)return 2;}
 else if(!strcmp(t,"statsrange")){ /* v4.5.5-nikque.16: book stats counts apart the scores out of -64..64 (a book file holds up to +/-127) */
  Search s;init_search(&s,1);Book b={0};Board init;Position *p;b.search=&s;book_new(&b,2,36);board_init(&init);p=book_probe(&b,&init);if(!p)return 2;p->score.value=127;
  book_stats(&b);p->score.value=-127;book_stats(&b);printf("BOOK stats with scores +/-127 done\n");book_free(&b);search_free(&s);}
 else if(!strcmp(t,"gamescore")){ /* v4.5.5-nikque.16: the score of a game of an odd number of moves had the wrong sign */
  int bad[2]={0,0},total[2]={0,0};
  for(int k=0;k<4000;++k){Game g;Board b;int n=0,black=1;game_init(&g);board_init(&b);
   for(;;){unsigned long long m=board_get_moves(&b);if(!m){board_pass(&b);black^=1;m=board_get_moves(&b);if(!m)break;}int r=rr()%bit_count(m);while(r--)m&=m-1;g.move[n++]=first_bit(m);board_next(&b,first_bit(m),&b);black^=1;}
   int p=bit_count(b.player),o=bit_count(b.opponent),e=64-p-o,d=p-o;if(d>0)d+=e;else if(d<0)d-=e;++total[n&1];if(game_score(&g)!=(black?d:-d))++bad[n&1];}
  printf("GAME score: even moves %d games %d wrong, odd moves %d games %d wrong\n",total[0],bad[0],total[1],bad[1]);if(bad[0]||bad[1]||!total[1])return 2;}
 else if(!strcmp(t,"badinput")){ /* v4.5.5-nikque.16: FEN rows, edx games, boards of fewer than 4 discs, book depth */
  Board b;Game g;FILE *f;int ok=1;
  if(board_from_FEN(&b,"9p b")!=EMPTY||board_from_FEN(&b,"8/8/8/8/8/8/8/8/8/p b")!=EMPTY||board_from_FEN(&b,"8/8/8/3Pp3/3pP3/8/8 b")!=EMPTY)ok=0;
  if(board_from_FEN(&b,"8/8/8/3Pp3/3pP3/8/8/8 b")!=BLACK||board_count_empties(&b)!=60)ok=0;
  b.player=1;b.opponent=2;if(board_is_usable(&b))ok=0;board_init(&b);if(!board_is_usable(&b))ok=0;
  game_init(&g);g.move[0]=100;f=fopen("audit-bad.edx","wb");fwrite(&g,sizeof g,1,f);fclose(f);f=fopen("audit-bad.edx","rb");if(game_read_checked(&g,f))ok=0;fclose(f);
  game_init(&g);g.player=7;f=fopen("audit-bad.edx","wb");fwrite(&g,sizeof g,1,f);fclose(f);f=fopen("audit-bad.edx","rb");if(game_read_checked(&g,f))ok=0;fclose(f);
  {Base base;base_init(&base);if(base_load(&base,"audit-bad.edx"))ok=0;base_free(&base);}remove("audit-bad.edx");
  {Line l;line_init(&l,BLACK);for(int i=0;i<GAME_SIZE;++i)line_push(&l,PASS);if(line_push(&l,PASS)||l.n_moves!=GAME_SIZE)ok=0;}
  printf("BAD input refused ok=%d\n",ok);if(!ok)return 2;}
 else if(!strcmp(t,"wthortrunc")){ /* v4.5.5-nikque.16: a wthor file that cannot be read completely is not loaded, and no game is added to it */
  Base b;Game g;FILE *f;long n;char buf[1024];int ok;remove("audit-trunc.wtb");base_init(&b);for(int i=0;i<3;++i){game_init(&g);text_to_game(i?"f5f6e6f4":"f5d6c3",&g);base_append(&b,&g);}
  ok=base_save(&b,"audit-trunc.wtb");base_free(&b);{Base c;base_init(&c);ok=ok&&base_load(&c,"audit-trunc.wtb")&&c.n_games==3;base_free(&c);}
  f=fopen("audit-trunc.wtb","rb");n=fread(buf,1,sizeof buf,f);fclose(f);f=fopen("audit-trunc.wtb","wb");fwrite(buf,1,n-30,f);fclose(f);{Base c;base_init(&c);ok=ok&&!base_load(&c,"audit-trunc.wtb");base_free(&c);} /* (a truncated wthor file is not loaded) */
  base_init(&b);game_init(&g);text_to_game("f5f4",&g);base_append(&b,&g);ok=ok&&!base_save(&b,"audit-trunc.wtb");base_free(&b);f=fopen("audit-trunc.wtb","rb");fseek(f,0,SEEK_END);ok=ok&&ftell(f)==n-30;fclose(f);
  printf("WTHOR truncated file kept ok=%d\n",ok);remove("audit-trunc.wtb");remove("WTHOR.JOU");if(!ok)return 2;}
 else if(!strcmp(t,"cassiolist")){ /* v4.5.5-nikque.16: the list of the positions of Cassio keeps every position */
  options.n_task=1;options.hash_table_size=10;options.verbosity=0;options.cpu_affinity=false;eval_open(eval_path());search_global_init();
  Engine *e=(Engine*)calloc(1,sizeof *e);Board b[3];int ok;e->search=engine_create_search();e->last_position.n=0;
  for(int i=0;i<3;++i){b[i]=late_board(20+i);is_position_new(e,b+i);}
  ok=e->last_position.n==3&&board_equal(e->last_position.board,b+2)&&board_equal(e->last_position.board+1,b+1)&&board_equal(e->last_position.board+2,b);
  ok=ok&&!is_position_new(e,b);printf("CASSIO position list of 3 ok=%d\n",ok);engine_free(e->search);free(e);if(!ok)return 2;}
 else if(!strcmp(t,"parsers")){ /* v4.5.5-nikque.16: D:HH:MM:SS times, "P@" alone, option files that read themselves, mode */
  int ok=string_to_time("0:1:0:0")==3600000&&string_to_time("1:0:0:0")==86400000&&string_to_time("1:00:00")==3600000&&string_to_time("1:30")==90000&&string_to_time("2:03.5")==123500;
  char *p=(char*)malloc(3);strcpy(p,"P@");ok=ok&&string_to_coordinate(p)==NOMOVE;free(p);p=(char*)malloc(2);strcpy(p,"@");ok=ok&&string_to_coordinate(p)==NOMOVE;free(p);
  ok=ok&&string_to_coordinate("P@d3")==D3&&string_to_coordinate("@@@@")==PASS;
  FILE *f=fopen("audit-loop.ini","w");fputs("option-file audit-loop.ini\nmode 9\n",f);fclose(f);options_parse("audit-loop.ini");options_bound();ok=ok&&options.mode==3;remove("audit-loop.ini");
  printf("PARSERS times, coordinates, option-file loop, mode ok=%d\n",ok);if(!ok)return 2;}
 else if(!strcmp(t,"tools")){ /* v4.5.5-nikque.16: opening names, shapes after a pass, the moves of an OBF line */
  int ok=1;const char *l;ShapeHash sh;BoardCache bc;Board b;unsigned seed=3;int found=0;OBF obf;FILE *f;
  l=opening_get_line("tigre centrale");ok=ok&&l&&!strcmp(l,"f5d6c3d3c4f4c5");l=opening_get_line("tigre");ok=ok&&l&&!strcmp(l,"f5d6c3d3c4");
  while(!found){board_init(&b);for(;;){unsigned long long m=board_get_moves(&b);if(!m){Board p=b;board_pass(&p);if(board_get_moves(&p)&&board_count_empties(&b)>3)found=1;break;}int k=(int)((seed=seed*1103515245+12345)>>16);k%=bit_count(m);while(k--)m&=m-1;board_next(&b,first_bit(m),&b);}}
  {Board p=b;board_pass(&p);shapehash_init(&sh,12);boardcache_init(&bc,12);ok=ok&&count_shape(&sh,&bc,&b,1)==(unsigned long long)bit_count(board_get_moves(&p));shapehash_delete(&sh);boardcache_delete(&bc);}
  f=fopen("audit-moves.obf","w");fputs("---------------------------OX------XO--------------------------- X",f);for(int i=0;i<40;++i)fputs(";f5:+1",f);fputs(";\n",f);fclose(f);
  f=fopen("audit-moves.obf","r");memset(&obf,0,sizeof obf);obf_read(&obf,f);fclose(f);ok=ok&&obf.n_moves==MAX_MOVE;obf_free(&obf);remove("audit-moves.obf");
  printf("TOOLS opening names, shapes after a pass, OBF moves ok=%d\n",ok);if(!ok)return 2;}
 else if(!strcmp(t,"unique")){ /* v4.5.5-nikque.16: base unique sorts the games (the first ones kept, in order); games from other boards differ */
  Base b;Game g,c;int ok;base_init(&b);
  for(int i=0;i<300;++i){game_init(&g);text_to_game(i%3?"f5d6c3":"f5f6e6f4",&g);sprintf(g.name[0],"p%d",i%50);base_append(&b,&g);}
  c=g;c.initial_board.player^=1ULL<<A1;base_append(&b,&c);base_unique(&b);
  ok=b.n_games==101&&!strcmp(b.game[0].name[0],"p0")&&b.game[0].move[1]==F6&&!strcmp(b.game[1].name[0],"p1")&&b.game[1].move[1]==D6&&!board_equal(&b.game[100].initial_board,&b.game[99].initial_board);
  printf("BASE unique: %d games kept (101 expected) ok=%d\n",b.n_games,ok);base_free(&b);if(!ok)return 2;}
 else if(!strcmp(t,"mergehint")){ /* book merge keeps the leaves of the source as hints for the positions of the destination (book_link_parallel looks for them): sorted by their places, every one found */
  Book s={0},d={0},*bp=&s;Position *p;PositionArray *a;book_init(&s);book_init(&d);options.verbosity=0;
  for(int k=0;k<2;++k){Book *x=k?&d:&s;Position q;Board ib;position_init(&q);board_init(&ib);board_unique(&ib,&q.board);if(book_add(x,&q)<=0)return 2;}
  int n_cur=1,n_next;Board *cur=(Board*)malloc(sizeof(Board)),*next;board_init(cur);
  for(int ply=1;ply<=7;++ply){next=(Board*)malloc((size_t)n_cur*MAX_MOVE*sizeof(Board));n_next=0;
   for(int i=0;i<n_cur;++i){unsigned long long m=board_get_moves(cur+i);int x;foreach_bit(x,m){Board nb,u;board_next(cur+i,x,&nb);board_unique(&nb,&u);if(book_probe(&s,&u))continue;Position q;position_init(&q);q.board=u;
    unsigned long long lm=board_get_moves(&u);q.leaf.move=(unsigned char)first_bit(lm);q.leaf.score=(signed char)(rr()%129-64);q.level=10;if(book_add(&s,&q)<=0)return 2;if(rr()%3){position_init(&q);q.board=u;if(book_add(&d,&q)<=0)return 2;}next[n_next++]=u;}}
   free(cur);cur=next;n_cur=n_next;}
  free(cur);if(!book_save(&s,"audit-hint-src.dat")||!book_merge_file(&d,"audit-hint-src.dat"))return 2;
  long long unsorted=0,missing=0;for(long long i=0;i<merge_hint_n;++i){if(i&&merge_hint[i-1].key>=merge_hint[i].key)++unsorted;if(merge_hint_find(merge_hint[i].key)!=merge_hint+i)++missing;}
  printf("MERGE hints=%lld (source %u positions, destination %u after the merge) unsorted=%d missing=%lld\n",merge_hint_n,s.n_nodes,d.n_nodes,(int)unsorted,missing);
  bool ok=merge_hint_n>1000&&!unsorted&&!missing;merge_hint_free();book_free(&s);book_free(&d);(void)bp;(void)p;(void)a;if(!ok)return 2;}
 else if(!strcmp(t,"crc32c")){ /* CRC-32C of the book and evaluation files of form 2 (instruction or table: the same values), against a reference computed bit by bit */
  auto reference=[](const unsigned char *p,size_t n){unsigned int c=0xFFFFFFFFu;for(size_t i=0;i<n;++i){c^=p[i];for(int k=0;k<8;++k)c=(c>>1)^(0x82F63B78u&(0u-(c&1)));}return ~c;};
  unsigned char buffer[300];int bad=0;unsigned int check=crc32c_bytes(0,"123456789",9);if(check!=0xE3069283u)++bad;
  for(int i=0;i<2000;++i){size_t n=rr()%257,o=rr()%8,m=n?rr()%(n+1):0;for(size_t k=0;k<n;++k)buffer[o+k]=(unsigned char)rr();unsigned int want=reference(buffer+o,n);if(crc32c_bytes(0,buffer+o,n)!=want||crc32c_bytes(crc32c_bytes(0,buffer+o,m),buffer+o+m,n-m)!=want)++bad;}
  printf("CRC32C check value %08x (e3069283 expected), 2000 buffers against the bit by bit reference, differences=%d\n",check,bad);if(bad)return 2;}
 else if(!strcmp(t,"bookread")){ /* a book of 80,068 positions with random values (counts of 1 to 5 bytes in form 2, some in the table), saved in form 1 and in form 2, is read back the same (the records read in the buffer, and the last ones across its end) */
  Book b={0},*bp=&b;Position *p;PositionArray *a;book_init(&b);options.verbosity=0;
  {Position q;Board ib;position_init(&q);board_init(&ib);board_unique(&ib,&q.board);if(book_add(&b,&q)<=0)return 2;}
  int n_cur=1,n_next;Board *cur=(Board*)malloc(sizeof(Board)),*next;board_init(cur);
  for(int ply=1;ply<=8;++ply){next=(Board*)malloc((size_t)n_cur*MAX_MOVE*sizeof(Board));n_next=0;
   for(int i=0;i<n_cur;++i){unsigned long long m=board_get_moves(cur+i);int x;foreach_bit(x,m){Board nb,u;board_next(cur+i,x,&nb);board_unique(&nb,&u);if(book_probe(&b,&u))continue;Position q;position_init(&q);q.board=u;if(book_add(&b,&q)<=0)return 2;next[n_next++]=u;}}
   free(cur);cur=next;n_cur=n_next;}
  free(cur);
  auto rnd_count=[](){unsigned r=rr()%100;return r<70?rr()%128:r<85?rr()%16384:r<95?rr()%65534:r<98?65534+rr()%100000:rr();};
  foreach_position(p,a,bp){p->level=(unsigned char)(rr()%61);p->score.value=(signed char)(rr()%129-64);p->score.lower=(signed char)(p->score.value-rr()%3);p->score.upper=(signed char)(p->score.value+rr()%3);
   unsigned long long m=board_get_moves(&p->board);if(m&&rr()%2){p->leaf.move=(unsigned char)first_bit(m);p->leaf.score=(signed char)(rr()%129-64);}unsigned int c[4]={rnd_count(),rnd_count(),rnd_count(),rnd_count()};position_set_counts_n(p,&b,c);}
  foreach_position(p,a,bp)position_link(p,&b);
  int bad=0;unsigned int table=b.counts?(unsigned int)b.counts->map.size():0;
  for(int form=1;form<=4;++form){Book c={0};options.book_format=form>2?form-2:form;const char *f=form==1?"audit-read-1.dat":form==2?"audit-read-2.dat":form==3?"audit-read-3.dat":"audit-read-4.dat";
   if(form==3){int n0=b.n;book_grow_buckets(&b,(long long)b.n*64);if(b.n<=n0)return 2;} /* 3, 4: saved in the order of more buckets than a load makes */
   if(!book_save(&b,f)||!book_load(&c,f)||c.n_nodes!=b.n_nodes||c.file_format!=options.book_format||(form>2&&c.n>=b.n))return 2;
   foreach_position(p,a,bp){Position *q=book_probe(&c,&p->board);unsigned int n1[4],n2[4];if(!q){++bad;continue;}position_get_counts_n(p,&b,n1);position_get_counts_n(q,&c,n2);
    if(memcmp(n1,n2,16)||q->n_link!=p->n_link||memcmp(position_links(q),position_links(p),2*p->n_link)||q->leaf.move!=p->leaf.move||q->leaf.score!=p->leaf.score||q->level!=p->level||q->score.value!=p->score.value||q->score.lower!=p->score.lower||q->score.upper!=p->score.upper)++bad;}
   if(form>2){ /* the positions of a bucket keep the order of the file */
    boost::unordered_flat_map<BookCountsKey,unsigned int,BookCountsHash> order;unsigned int k=0;foreach_position(p,a,bp)order[BookCountsKey{p->board.player,p->board.opponent}]=k++;
    Book *cp=&c;for(a=cp->array;a<cp->array+cp->n;++a)for(int i=1;i<a->n;++i)if(order[BookCountsKey{a->positions[i-1].board.player,a->positions[i-1].board.opponent}]>order[BookCountsKey{a->positions[i].board.player,a->positions[i].board.opponent}])++bad;}
   book_free(&c);}
  options.book_format=0;
  for(int k=0;k<3;++k){ /* in another bucket order too, a damaged file is not loaded: one byte less (form 1), one bit changed (form 2), the first position twice (form 1) */
   FILE *f=fopen(k==1?"audit-read-4.dat":"audit-read-3.dat","rb");fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);unsigned char *d=(unsigned char*)malloc(n);if(fread(d,1,n,f)!=(size_t)n)return 2;fclose(f);
   if(k==1)d[n/2]^=4;f=fopen("audit-read-bad.dat","wb");
   if(k==2){unsigned int count;memcpy(&count,d+38,4);++count;memcpy(d+38,&count,4);fwrite(d,1,n,f);fwrite(d+42,1,40+2*(d[42+38]+1),f);}else fwrite(d,1,k?n:n-1,f);
   fclose(f);free(d);Book c={0};if(book_load(&c,"audit-read-bad.dat"))++bad;book_free(&c);}
  {FILE *f=fopen("audit-read-1.dat","rb");fseek(f,0,SEEK_END);long size=ftell(f);fclose(f);
   printf("BOOKREAD positions=%u counts_in_table=%u form1_bytes=%ld differences=%d (forms 1 and 2, saved in the bucket order of the load and of 4 times more buckets)\n",b.n_nodes,table,size,bad);}
  book_free(&b);if(bad||table==0)return 2;}
 else if(!strcmp(t,"bigcounts")){ /* game counts above 65534 (kept in the table of the book): the negamax with threads gives the counts of one thread, positions going in and out of the table; the table keeps an entry for each position whose counts are in it, and for no other board (book fix, book subtree) */
  Search s;init_search(&s,1);Book b={0},*bp=&b;Position *p,*root;PositionArray *a;book_init(&b);b.search=&s;b.options.level=1;b.options.n_empties=40;
  {Position q;Board ib;position_init(&q);board_init(&ib);board_unique(&ib,&q.board);q.level=1;if(book_add(&b,&q)<=0)return 2;} /* every position up to ply 8 (unique boards) */
  int n_cur=1,n_next;Board *cur=(Board*)malloc(sizeof(Board)),*next;board_init(cur);
  for(int ply=1;ply<=8;++ply){next=(Board*)malloc((size_t)n_cur*MAX_MOVE*sizeof(Board));n_next=0;
   for(int i=0;i<n_cur;++i){unsigned long long m=board_get_moves(cur+i);int x;foreach_bit(x,m){Board nb,u;board_next(cur+i,x,&nb);board_unique(&nb,&u);if(book_probe(&b,&u))continue;Position q;position_init(&q);q.board=u;q.level=1;if(book_add(&b,&q)<=0)return 2;next[n_next++]=u;}}
   free(cur);cur=next;n_cur=n_next;}
  free(cur);foreach_position(p,a,bp)position_link(p,&b);
  const unsigned int n=b.n_nodes;unsigned int (*ref)[4]=(unsigned int(*)[4])malloc((size_t)n*16),(*got)[4]=(unsigned int(*)[4])malloc((size_t)n*16);bool *was=(bool*)malloc(n);
  auto counts=[&](unsigned int (*o)[4]){unsigned int k=0;foreach_position(p,a,bp)position_get_counts_n(p,&b,o[k++]);};
  auto table_ok=[&]()->bool{size_t in=0;foreach_position(p,a,bp)if(position_counts_in_table(p)){++in;if(!b.counts||b.counts->map.find(BookCountsKey{p->board.player,p->board.opponent})==b.counts->map.end())return false;}return in==(b.counts?b.counts->map.size():0);};
  auto leaves=[&](int r){foreach_position(p,a,bp)if(board_count_empties(&p->board)==52){bool keep=r%3==0||((board_get_hash_code(&p->board)>>r)&(r%3==1?1:3))==0;p->leaf.move=keep?(unsigned char)first_bit(board_get_moves(&p->board)):NOMOVE;p->leaf.score=keep?0:-SCORE_INF;}};
  int differences=0,transitions=0;unsigned int root_lines=0;
  for(int r=0;r<9;++r){leaves(r);unsigned int k=0;foreach_position(p,a,bp)was[k++]=position_counts_in_table(p);
   options.n_task=4;book_negamax(&b);counts(got);k=0;foreach_position(p,a,bp){transitions+=was[k]!=position_counts_in_table(p);++k;}if(!table_ok())++differences;
   options.n_task=1;book_negamax(&b);counts(ref);if(memcmp(ref,got,(size_t)n*16))++differences;if(r==0){unsigned int c[4];position_get_counts_n(book_root(&b),&b,c);root_lines=c[3];}}
  leaves(0);root=book_root(&b);bool in=position_counts_in_table(root);root->level=61;options.n_task=4;book_fix(&b);bool fix_ok=in&&table_ok(); /* the fixed position starts again from 0 */
  book_negamax(&b);Board b1,b2;board_init(&b2);board_next(&b2,F5,&b1);board_next(&b1,D6,&b2);
  unsigned int before=n;book_subtree(&b,&b2);bool subtree_ok=b.n_nodes<before&&table_ok();
  printf("BIGCOUNTS positions=%u root_lines=%u (390216 expected) transitions=%d differences=%d table_after_fix=%d table_after_subtree=%d nodes_after_subtree=%u\n",n,root_lines,transitions,differences,fix_ok,subtree_ok,b.n_nodes);
  free(ref);free(got);free(was);book_free(&b);search_free(&s);if(root_lines!=390216||transitions==0||differences||!fix_ok||!subtree_ok)return 2;}
 else return 1;
 return 0;
}
