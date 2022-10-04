#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*-------------------DEFINING--------------*/
typedef struct{
  char *string;     //salvo la stringa che va salvata
}item;

typedef struct{
  char *newstring;  //stringa che viene inserita ora (utile nelle redo) //nella delete uso solo questa
  char *oldstring;  //stringa che è sostituita quando c'è un cambio (utile nelle undo)
}savedstring;

typedef struct{
  char command;
  int addr1;
  int addr2;
  int prevmaxhash;    //maxhash prima che questa è stata salvata, per ritornare alle condizioni iniziali (penso non serva, lo ricalcolo)
  savedstring *ptr;
}savedinstr;

item *hash;
int sethash=99; //indice massimo a cui posso arrivare per ora (si modifica poi in esecuzione)
#define ADDED 100
int maxundo=0;    //numero di operazioni effettuate fino ad adesso
int lastpointed=0;   //indice a cui siamo arrivati (maxundo è sempre il max a meno di undo-change/undo-delete), questo indica facendo gli undo/redo a dove si arriva
int maxchanges=99;   //indice massimo a cui posso arrivare salvando i comandi
savedinstr *history;
/*------------------FUNCTIONS-----------------*/
int convertstring(char* instruction,int maxhash);
int change(int ind1,int ind2,int maxhash);
void additem(int currind,int maxhash,int ind1);
void print(int ind1,int ind2,int maxhash);
int delete(int ind1,int ind2,int maxhash);
int undo(int num,int maxhash);
int redo(int num,int maxhash);
/*--------------------MAIN------------------*/
int main(){
  int maxhash=0; //massimo indice a cui sono arrivato
  char* instruction;
  instruction=malloc(sizeof(char)*51);

  hash=malloc(sizeof(item)*(sethash+1));
  item zero;
  zero.string=malloc(sizeof(char)*3);
  zero.string=".\n";
  hash[0]=zero;

  history=malloc(sizeof(savedinstr)*(maxchanges+1));


  do{
    fgets(instruction,50,stdin);
    if(*(instruction)=='.') continue;
    else if(*(instruction)!='q') maxhash=convertstring(instruction,maxhash);
    else  return 0;                                 //stop case
    if(maxhash==-1) return 0;   //utile per la lettura di undo/redo concatenate
  }while(strcmp(instruction,"q\n")!=0);


  return 0;
}
/*-----------------FUNCTIONS--------------*/
int convertstring(char* instruction,int maxhash){
  char *instr2;
  instr2=malloc(sizeof(char)*51);
  int ind1=0,ind2=0,i,len,instrlen,undo_redo=0,check=0;   //undo_redo: sottraggo per gli undo, aggiungo per i redo. In base al numero (pos o neg), si sceglie cosa fare
  len=strlen(instruction);            //check fa da booleano: 1 se ho undo/redo (quindi mi serve di eseguire sia instruction che instr2)

  //trovo ind1 e ind2
  ind1=atoi(instruction);

  if((*(instruction+len-2)=='r')||(*(instruction+len-2)=='u')){     //questo if è per unire undo/redo consecutivi
    check=1;
    if(*(instruction+len-2)=='r'){
      undo_redo=ind1;
      if(undo_redo>(maxundo-lastpointed))   undo_redo=maxundo-lastpointed;
    }
    else{
      undo_redo=-ind1;
      if(-undo_redo>lastpointed)   undo_redo=-lastpointed;
    }
    do{
      fgets(instr2,50,stdin);
      instrlen=strlen(instr2);
      ind1=atoi(instr2);
      if(*(instr2+instrlen-2)=='r'){
        undo_redo+=ind1;
        if(undo_redo>(maxundo-lastpointed))   undo_redo=maxundo-lastpointed;
      }
      else if(*(instr2+instrlen-2)=='u'){
        undo_redo-=ind1;
        if(-undo_redo>lastpointed)   undo_redo=-lastpointed;
      }
    }while((*(instr2+instrlen-2)=='r')||(*(instr2+instrlen-2)=='u'));
    for(i=0;i<instrlen;i++){          //uscito dal while ho un'istruzione normale, quindi trovo ind2
      if(*(instr2+i)==','){
        ind2=atoi(instr2+i+1);
        break;
      }
    }
    strncpy(instruction,instr2,instrlen+1);
    len=instrlen;
  }
  else{
    for(i=0;i<len;i++){
      if(*(instruction+i)==','){
        ind2=atoi(instruction+i+1);
        break;
      }
    }
  }

  //controllo se serva più spazio per salvare i comandi
  if(maxundo>=maxchanges-1){
    history=realloc(history,sizeof(savedinstr)*(maxchanges+ADDED+1));
    maxchanges+=ADDED;
  }

  //cases
  if(check==1){
    if(undo_redo<0){
      undo_redo= -undo_redo;
      if(lastpointed<=0)   lastpointed=0;      //Non si possono fare undo
      else if(lastpointed-undo_redo<0) maxhash=undo(lastpointed,maxhash);      //caso di troppi undo, ne faccio il più possibile
      else   maxhash=undo(undo_redo,maxhash);       //Si possono fare tutti e li faccio
    }
    else if(undo_redo>0){
      if(lastpointed>=maxundo)    lastpointed=maxundo;    //non si possono fare redo
      else if(lastpointed+undo_redo>maxundo)   maxhash=redo(maxundo-lastpointed,maxhash);  //troppi redo, ne faccio il più possibile
      else   maxhash=redo(undo_redo,maxhash);    //si possono fare tutti e li faccio
    }
  }
  if(*(instruction+len-2)=='c'){
    if(lastpointed==maxundo){    //non ci sono undo-redo (o si annullano)
      maxundo++;
      lastpointed++;
    }
    else{                     //caso con undo-redo e che si annullano le modifiche successive
      lastpointed++;
      maxundo=lastpointed;
    }
    (history+lastpointed)->command= *(instruction+len-2);
    (history+lastpointed)->addr1=ind1;
    (history+lastpointed)->addr2=ind2;
    (history+lastpointed)->prevmaxhash=maxhash;
    maxhash=change(ind1,ind2,maxhash);
  }
  else if(*(instruction+len-2)=='p') print(ind1,ind2,maxhash);
  else if(*(instruction+len-2)=='d'){
    if(lastpointed==maxundo){    //non ci sono undo-redo (o si annullano)
      maxundo++;
      lastpointed++;
    }
    else{                     //caso con undo-redo e che si annullano le modifiche successive
      lastpointed++;
      maxundo=lastpointed;
    }
    (history+lastpointed)->command= *(instruction+len-2);
    (history+lastpointed)->addr1=ind1;
    (history+lastpointed)->addr2=ind2;   //qui prevmaxhash non serve dato che basta sommare o sottrarre per arrivare al maxhash
    (history+lastpointed)->prevmaxhash=maxhash;
    maxhash=delete(ind1,ind2,maxhash);
  }
  else if(*(instruction)=='q')  return -1;

  return maxhash;
}
/*-----------------------------------------*/
int change(int ind1,int ind2,int maxhash){
  int i;

  if(ind2>sethash){
    hash=realloc(hash,sizeof(item)*(sethash+ADDED+1));
    sethash+=ADDED;
  }

  (history+lastpointed)->ptr=malloc(sizeof(savedstring)*(ind2-ind1+1));
  for(i=ind1;i<=ind2;i++){
    additem(i,maxhash,ind1);
  }

  if(ind2>maxhash)  return ind2;
  else  return maxhash;
}
/*--------------------------------------*/
void additem(int currind,int maxhash,int ind1){
  int tmpsize;
  char *tmp;

  //per il tempo: eventualmente evita di riallocare e guarda la dimensione: se piccola allora new, altrimenti lascia così (hash[currind].string=tmp).

  if(currind>maxhash){    //caso in cui non ci sono stringhe precedenti di cui tenere conto
    tmp=malloc(sizeof(char)*1026);

    fgets(tmp,1026,stdin);      //leggo la stringa
    tmpsize=strlen(tmp);

    ((history+lastpointed)->ptr+(currind-ind1))->oldstring=NULL;          //salvo così per poterlo controllare
    ((history+lastpointed)->ptr+(currind-ind1))->newstring=malloc(sizeof(char)*(tmpsize+1));
    strncpy(((history+lastpointed)->ptr+(currind-ind1))->newstring,tmp,tmpsize+1);
    (hash+currind)->string=((history+lastpointed)->ptr+(currind-ind1))->newstring;    //in hash non salvo, copio solo i puntatori (hash serve per stampare)


    free(tmp);
  }
  else{           //caso con stringhe precedenti di cui tenere conto
    //salvo la stringa precedente
    tmpsize=strlen((hash+currind)->string);
    ((history+lastpointed)->ptr+(currind-ind1))->oldstring=malloc(sizeof(char)*(tmpsize+1));
    strncpy(((history+lastpointed)->ptr+(currind-ind1))->oldstring,(hash+currind)->string,tmpsize+1);

    //salvo la stringa nuova
    tmp=malloc(sizeof(char)*1026);

    fgets(tmp,1026,stdin);      //leggo la stringa
    tmpsize=strlen(tmp);

    ((history+lastpointed)->ptr+(currind-ind1))->newstring=malloc(sizeof(char)*(tmpsize+1));
    strncpy(((history+lastpointed)->ptr+(currind-ind1))->newstring,tmp,tmpsize+1);
    (hash+currind)->string=((history+lastpointed)->ptr+(currind-ind1))->newstring;

    free(tmp);
  }

  return;
}
/*--------------------------------------*/
void print(int ind1,int ind2,int maxhash){
  int i;

  for(i=ind1;i<=ind2;i++){
    if(i>maxhash) printf(".\n");
    else  printf("%s",(hash+i)->string);
  }
  return;
}
/*--------------------------------------*/
int delete(int ind1,int ind2,int maxhash){
  int i,diff,len;

  diff=ind2-ind1+1;

  (history+lastpointed)->ptr=malloc(sizeof(savedstring)*(diff));
  for(i=ind1;i<=ind2 && i<=maxhash;i++){        //NUOVA la seconda condizione
    ((history+lastpointed)->ptr+(i-ind1))->oldstring=NULL;   //lo setto a NULL per poterlo poi controllare
    len=strlen((hash+i)->string);
    ((history+lastpointed)->ptr+(i-ind1))->newstring=malloc(sizeof(char)*(len+1));
    strncpy(((history+lastpointed)->ptr+(i-ind1))->newstring,(hash+i)->string,len+1);
  }

  if((ind2<maxhash)&&(maxhash>0)){
    for(i=ind2+1;i<=maxhash;i++){
      (hash+i-diff)->string=(hash+i)->string;
      (hash+i)->string=NULL;
    }
  }
  else if((ind2>maxhash)&&(ind1<=maxhash)&&(maxhash>0)){
    ind2=maxhash;
  }

  for(i=ind1;i<=ind2;i++){
    if(maxhash>0) maxhash--;
  }

  return maxhash;
}
/*---------------------------------------*/
int undo(int num,int maxhash){
  int i,j,ind1,ind2,len;
  char comm;

  for(i=0;i<num;i++){
    comm=(history+lastpointed)->command;

    if(comm=='c'){    //undo di una CHANGE
      ind1=(history+lastpointed)->addr1;
      ind2=(history+lastpointed)->addr2;
      maxhash=(history+lastpointed)->prevmaxhash;   //lo cambio a prescindere

      for(j=0;j<=ind2-ind1;j++){    //ogni volta salvo (ind2-ind1+1) stringhe, quindi gli indici vanno da 0 a ind2-ind1
        if(((history+lastpointed)->ptr+j)->oldstring==NULL){      //non c'è una vecchia stringa da salvare
          (hash+ind1+j)->string=NULL;
        }
        else{               //c'è una vecchia stringa da salvare
          (hash+ind1+j)->string=((history+lastpointed)->ptr+j)->oldstring;
        }
      }
    }


    else {          //undo di una DELETE
      ind1=(history+lastpointed)->addr1;
      ind2=(history+lastpointed)->addr2;
      maxhash=(history+lastpointed)->prevmaxhash;   //lo cambio a prescindere
      for(j=maxhash;j>ind2;j--){
        (hash+j)->string=(hash+j-ind2+ind1-1)->string;
      }
      for(j=0;j<=ind2-ind1;j++){
        (hash+ind1+j)->string=((history+lastpointed)->ptr+j)->newstring;
      }
    }

    lastpointed--;    //decremento cosicché si aggiorni nel programma
  }

  return maxhash;
}
/*---------------------------------------*/
int redo(int num,int maxhash){
  int i,j,ind1,ind2,len,diff;
  char comm;

  for(i=0;i<num;i++){
    lastpointed++;            //incremento cosicché si aggiorni nel programma.          GUARDA se è possibile che vada oltre i limiti; anche nell'undo
    comm=(history+lastpointed)->command;

    if(comm=='c'){    //redo di una CHANGE
      ind1=(history+lastpointed)->addr1;
      ind2=(history+lastpointed)->addr2;
      if(ind2>maxhash) maxhash=ind2;

      for(j=0;j<=ind2-ind1;j++){    //ogni volta salvo (ind2-ind1+1) stringhe, quindi gli indici vanno da 0 a ind2-ind1
          (hash+ind1+j)->string=((history+lastpointed)->ptr+j)->newstring;
      }
    }

    else {          //redo di una DELETE
      ind1=(history+lastpointed)->addr1;
      ind2=(history+lastpointed)->addr2;
      diff=ind2-ind1+1;


      if((ind2<maxhash)&&(maxhash>0)){
        for(j=ind2+1;j<=maxhash;j++){
          (hash+j-diff)->string=(hash+j)->string;
          (hash+j)->string=NULL;
        }
      }
      else if((ind2>maxhash)&&(ind1<=maxhash)&&(maxhash>0)){
        ind2=maxhash;
      }
      for(j=ind1;j<=ind2;j++){      //stesso meccanismo della delete
        if(maxhash>0) maxhash--;
      }
    }

  }

  return maxhash;
}
