#include<iostream>
#include <thread>
#include <mutex>
#include <ncurses.h>
#include <chrono>
#include <deque>
#include <atomic>
#include <unistd.h>
#include <vector>
#include <condition_variable>
#include <algorithm>
using namespace std;

#define NUMBER_OF_CLIENTS 6
#define NUMBER_OF_BARMANS 2
#define NUMBER_OF_MUGS 20
#define NUMBER_OF_TOILETS 1
#define NUMBER_OF_WASHSTANDS 2


#define CLIENT_SIT 1
#define CLIENT_IN_QUEUE 2
#define CLIENT_DRINK 3
#define CLIENT_IS_WAITNG 6
#define CLIENT_IN_QUEUE_TO_TOILET 4
#define CLIENT_IN_TOILET 5

#define BARMAN_FREE 4
#define BARMAN_POUR 5
#define BARMAN_CLEANING 3
#define BARMAN_WAITING_FOR_MUGS 2

#define MUG_CLEAN 1
#define MUG_IN_HAND 2
#define MUG_DIRTY 3
#define MUG_IN_CLEANING 4

bool isRunning = true;
bool displayActive = true;
thread displayThread;

atomic<int> clientState[NUMBER_OF_CLIENTS];
atomic<int> barmanState[NUMBER_OF_BARMANS];
atomic<int> mugState[NUMBER_OF_MUGS];
int clientHand[NUMBER_OF_CLIENTS]; //Tablica przechowująca dane o tym jaki klient trzyma jaki kufel

thread clientLife[NUMBER_OF_CLIENTS];
thread barmanLife[NUMBER_OF_BARMANS];

mutex clientMutex[NUMBER_OF_CLIENTS];
bool clientBool[NUMBER_OF_CLIENTS];
condition_variable client_cv[NUMBER_OF_CLIENTS];
atomic<int> WC[NUMBER_OF_CLIENTS];
// unique_lock<mutex> lk[NUMBER_OF_CLIENTS];
// mutex barmanMutex[NUMBER_OF_BARMANS];
mutex mugsMutex;
mutex dirtyMugsMutex;
mutex queueMutex;
mutex queueToToiletMutex;
mutex cleaningMutex[NUMBER_OF_WASHSTANDS]; //Zmywak

mutex toilet[NUMBER_OF_TOILETS];

deque<int> queueClients;
deque<int> queueToToilet;
vector<int> dirtyMugs;
vector<int> mugsInUse;
vector<int> mugsInCleaning[NUMBER_OF_WASHSTANDS];
vector<int> cleanMugs;



int client_timeofact[NUMBER_OF_CLIENTS];
float client_timeofactfl[NUMBER_OF_CLIENTS];
float client_timeofactfl10[NUMBER_OF_CLIENTS];

float client_curtime[NUMBER_OF_CLIENTS];
float client_curtimefl[NUMBER_OF_CLIENTS];

int barman_timeofact[NUMBER_OF_BARMANS];
float barman_timeofactfl[NUMBER_OF_BARMANS];
float barman_timeofactfl10[NUMBER_OF_BARMANS];

float barman_curtime[NUMBER_OF_BARMANS];
float barman_curtimefl[NUMBER_OF_BARMANS];


void clientCycle(int id){
while(isRunning){
clientBool[id]=false;
clientState[id]=CLIENT_SIT;

//Ustawienie jak długo klient będzie siedział przy stole nie wstając po piwo
client_timeofact[id]=(rand() % 25 )+35;
client_timeofactfl[id]=(float)client_timeofact[id];
client_timeofactfl10[id]=client_timeofactfl[id]/10;

//Uruchomienie pętli kontrolującej czas siedzenia klienta
for(int i=0;i<client_timeofact[id] && isRunning;i++){
    client_curtime[id]=(float)(i);
    client_curtimefl[id]=client_curtime[id]/10;
    usleep(100000);
}

//Klient ustawia się do kolejki
queueMutex.lock();
bool inqueue=true;
clientState[id]=CLIENT_IN_QUEUE;
//Klient ustawia swoje id do kolejki
//clientMutex[id].lock();
 usleep(100000);
queueClients.push_back(id);
queueMutex.unlock();

//Oczekiwanie na wyjście z kolejki : metoda idle waits
unique_lock<mutex> lk(clientMutex[id]);
while(!clientBool[id])
{
client_cv[id].wait(lk);
}
 usleep(1000);
//Oczekiwanie na wyjscie z kolejjki : metoda busy wait
// while(clientState[id]==CLIENT_IN_QUEUE){
// usleep(1);
// }


//clientMutex[id].unlock();

//Klient czeka na nalanie piwa - wykorzystanie cv
clientBool[id]=false;
clientState[id]=CLIENT_IS_WAITNG;
while(!clientBool[id])
{
client_cv[id].wait(lk);
}
clientState[id]=CLIENT_DRINK;

 usleep(100000);
//Klient pije piwo


//Ustawienie czasu ile klient będzie pił piwo
client_timeofact[id]=(rand() % 9 )+25;
client_timeofactfl[id]=(float)client_timeofact[id];
client_timeofactfl10[id]=client_timeofactfl[id]/10;


for(int i=0;i<client_timeofact[id] && isRunning;i++){
    client_curtime[id]=(float)(i);
    client_curtimefl[id]=client_curtime[id]/10;
    usleep(100000);
}
// Klient odklada kufel
mugsMutex.lock();
dirtyMugsMutex.lock();
mugsInUse.erase(remove(mugsInUse.begin(),mugsInUse.end(),clientHand[id]),mugsInUse.end());
mugState[clientHand[id]]=MUG_DIRTY;
dirtyMugs.push_back(clientHand[id]);
clientHand[id]=-1;
dirtyMugsMutex.unlock();
mugsMutex.unlock();

// Zwiększenie poziomu zapotrzebowania do toalety
int temp = rand()%40+1;
WC[id]+=temp;

// Sprawdzenie czy klient musi juz isc do toalety
if(WC[id].load() >= 100) {
    
    WC[id]=100;
    //Klient ustawia sie w kolejce do toalety
    clientState[id]=CLIENT_IN_QUEUE_TO_TOILET;
    usleep(100000);
    queueToToiletMutex.lock();
    queueToToilet.push_back(id);
    queueToToiletMutex.unlock();
    usleep(100000);
    //Pętla lokująca i odblokuwająca mutex kolejki do toalety
    // W niej sprawdzane jest czy klient jest juz pierwszy w kolejce
    //Jeśli jest - wyjscie z petli
   queueToToiletMutex.lock();
    while(queueToToilet[0]!=id){
        queueToToiletMutex.unlock();
        usleep(100000);
        queueToToiletMutex.lock();
    }
    queueToToiletMutex.unlock();
    //Wyjscie z pętli oznacza że klient jest pierwszy w kolejce do toalety
    //próbuje dostać się do toalet:
    bool finish=false;
    int i=0;
    while(!finish){
        if(toilet[i].try_lock()){
                queueToToiletMutex.lock();
                queueToToilet.erase(remove(queueToToilet.begin(),queueToToilet.end(),id),queueToToilet.end());
                queueToToiletMutex.unlock();
            clientState[id]=CLIENT_IN_TOILET;
           usleep(100000);
            //Klient w toalecie
            //Ustalenie czasu korzystania z toalety
            //Ustawienie jak długo klient będzie korzystał z toalety
        client_timeofact[id]=(rand() % 25 )+35;
        client_timeofactfl[id]=(float)client_timeofact[id];
        client_timeofactfl10[id]=client_timeofactfl[id]/10;

//Uruchomienie pętli kontrolującej czas korzystania z toalety przez klienta
        for(int i=0;i<client_timeofact[id] && isRunning;i++){
        client_curtime[id]=(float)(i);
        client_curtimefl[id]=client_curtime[id]/10;
        usleep(100000);
        }
            //Klient skończył korzystac z toalety
        toilet[i].unlock();
        finish=true;
        }

        if(!finish){
        if(i==NUMBER_OF_TOILETS-1){
            i=0;
        }else{
            i++;
        }
         }

    }
    WC[id]=0;

}
}




}

void barmanCycle(int id){
   
while(isRunning){
    barmanState[id]=BARMAN_FREE;
bool isCleanMug=false;

//Barman szuka wolnego kufla
int idmug;
mugsMutex.lock();
for(int i=0; i<NUMBER_OF_MUGS;i++){
   
if(mugState[i]==MUG_CLEAN){
mugState[i]=MUG_IN_HAND;
idmug=i;
//clientHand[clientid]=i;
isCleanMug=true;

cleanMugs.erase(remove(cleanMugs.begin(),cleanMugs.end(),i),cleanMugs.end());
mugsInUse.push_back(i);
break;
}


}
mugsMutex.unlock();
while(!isCleanMug){
    //Barman nie znalazł wolnego kufla
    //Próbuje dostać się do zmywaka
    for(int i=0;i<NUMBER_OF_WASHSTANDS;i++){
    if(cleaningMutex[i].try_lock()){
    //Barman dostał się do zmywaka
    //Przenosi wszystkie kufle na zmywak
    dirtyMugsMutex.lock();
    mugsMutex.lock();
    for(int k=0; k<dirtyMugs.size();k++){
        mugsInCleaning[i].push_back(dirtyMugs[k]);
        mugState[dirtyMugs[k]]=MUG_IN_CLEANING;
       dirtyMugs.erase(dirtyMugs.begin()+k);
        
        
    }
   // dirtyMugs.clear();
   dirtyMugsMutex.unlock();
    mugsMutex.unlock();
  
   
    barmanState[id]=BARMAN_CLEANING;
     usleep(100000);
   // barmanMutex[id].lock();
   
    barman_timeofact[id]=((rand() % 11 )+25)*dirtyMugs.size();
    barman_timeofactfl[id]=(float)barman_timeofact[id];
    barman_timeofactfl10[id]=barman_timeofactfl[id]/10;
   // barmanMutex[id].unlock();

for(int i=0;i<barman_timeofact[id];i++){
    barman_curtime[id]=(float)(i);
    barman_curtimefl[id]=barman_curtime[id]/10;
    usleep(100000);
}
    mugsMutex.lock();
    // for(int k=0; k<mugsInCleaning[i].size();k++){
    //     cleanMugs.push_back(mugsInCleaning[i][k]);
    //     mugState[mugsInCleaning[i][k]]=MUG_CLEAN;
    //     mugsInCleaning[k].erase(mugsInCleaning[k].begin()+k);
        
        
    // }
    int itt=0;
  
    for( auto it = mugsInCleaning[i].begin(); it != mugsInCleaning[i].end(); it++){
            cleanMugs.push_back(*it);
            mugState[*it]=MUG_CLEAN;
           // mugsInCleaning[i].erase(mugsInCleaning[i].begin() + itt);
         usleep(10000);
             itt++;
    }
    

   mugsInCleaning[i].clear();
   usleep(1000);
    // Barman skończył myć kufle
    //Bierzy pierwszy z nich dla siebie i zwalnia zlewak
    idmug=1;
    
    isCleanMug=true;

    cleanMugs.erase(remove(cleanMugs.begin(),cleanMugs.end(),1),cleanMugs.end());
    mugsInUse.push_back(1);
    mugsMutex.unlock();

    cleaningMutex[i].unlock();
    break;
    }
}
if(isCleanMug){break;}
    //Barman nie dostał się żadnego do zmywaka, zmywaki są zajęte
    //Patrzy na zmywaki, czeka aż jakiś kolega skończy 
    //Wtedy weźmie sobie jakiś wolny kubek
    //Czeka na zwolnienie zmywaka, co bedzie oznaczało ze
    // <Rozwiązanie typu Busy-wait> 
    barmanState[id]=BARMAN_WAITING_FOR_MUGS;
int i=0;
   while(true){
       if(cleaningMutex[i].try_lock()){
           cleaningMutex[i].unlock();
           break;
       }
        usleep(1000);
       if(i==NUMBER_OF_WASHSTANDS-1){
           i=0;
       }
   }

  
for(int i=0; i<NUMBER_OF_MUGS;i++){
   mugsMutex.lock();
if(mugState[i]==MUG_CLEAN){
mugState[i]=MUG_IN_HAND;

idmug=i;
isCleanMug=true;
mugsMutex.unlock();
break;
}
 mugsMutex.unlock();
}

}

while(queueClients.empty()){
usleep(100000);
}

//barmanMutex[id].lock();
queueMutex.lock();
int clientid=queueClients[0];

queueClients.pop_front();
{
    lock_guard<mutex> lk(clientMutex[clientid]);
    clientBool[clientid] = true;
} // release lock
client_cv[clientid].notify_one();

//clientState[clientid]=CLIENT_IS_WAITNG;
clientHand[clientid]=idmug;
usleep(100000);
//barmanMutex[id].unlock();
queueMutex.unlock();
barmanState[id]=BARMAN_POUR;

barman_timeofact[id]=(rand() % 11 )+25;
barman_timeofactfl[id]=(float)barman_timeofact[id];
barman_timeofactfl10[id]=barman_timeofactfl[id]/10;


for(int i=0;i<barman_timeofact[id];i++){
    barman_curtime[id]=(float)(i);
    barman_curtimefl[id]=barman_curtime[id]/10;
    usleep(100000);
}

//Ustawienie cv klientowi - oznajmienie że zostało mu nalane piwo i może je wypić
{
    lock_guard<mutex> lk(clientMutex[clientid]);
    clientBool[clientid] = true;
} // release lock
client_cv[clientid].notify_one();

}

}

void GUI(){
initscr();
start_color();
noecho();

init_pair(1,COLOR_GREEN, COLOR_BLACK);
init_pair(2,COLOR_YELLOW, COLOR_BLACK);
init_pair(3,COLOR_BLUE, COLOR_BLACK);
init_pair(4,COLOR_RED, COLOR_BLACK);
init_pair(5,COLOR_MAGENTA, COLOR_BLACK);
init_pair(6,COLOR_WHITE,COLOR_BLACK);

mvprintw(1,3,"Press ESC to exit\n");
//Klienci
for(int i=0; i <NUMBER_OF_CLIENTS; i++){
    mvprintw(i+4,3,"Client %d", i+1);
}

//Barmani
for(int i=0;i<NUMBER_OF_BARMANS;i++){
    mvprintw(i+5+NUMBER_OF_CLIENTS,3,"Barman %d",i+1);
}

//Kufle
for(int i=0;i<NUMBER_OF_MUGS;i++){
    mvprintw(i+8+NUMBER_OF_CLIENTS+NUMBER_OF_BARMANS,3,"Mug: %d",i+1);
}

//Zmywaki
for(int i=0;i<NUMBER_OF_WASHSTANDS;i++){
mvprintw(i+8+NUMBER_OF_BARMANS+NUMBER_OF_CLIENTS, 50,"Washstands: %d",i+1);
}


while(displayActive){

    //Wypisanie klientów
    for(int i=0;i<NUMBER_OF_CLIENTS;i++){
        mvprintw(i+4,45,"Need for a toilet: %d%%      ",WC[i].load());
   
        attron(COLOR_PAIR(clientState[i]));

        if(clientState[i]==CLIENT_SIT){
            mvprintw(i+4,14,"is sitting      %.1fs/%1.fs    ",client_curtimefl[i],client_timeofactfl10[i]);
        }
        if(clientState[i]==CLIENT_DRINK){
            mvprintw(i+4,14,"is drinking     %.1fs/%1.fs    ",client_curtimefl[i],client_timeofactfl10[i]);
        }
        if(clientState[i]==CLIENT_IN_QUEUE){
          mvprintw(i+4,14,"is in queue                    ");
        }
        if(clientState[i]==CLIENT_IN_QUEUE_TO_TOILET){
          mvprintw(i+4,14,"is in queue to toilet                  ");
        }
        if(clientState[i]==CLIENT_IS_WAITNG){
            mvprintw(i+4,14,"is in waiting                  ");
        }
        if(clientState[i]==CLIENT_IN_TOILET){
            mvprintw(i+4,14,"is in toilet   %.1fs/%1.fs     ",client_curtimefl[i],client_timeofactfl10[i]);
        }

        attroff(COLOR_PAIR(clientState[i]));
    }

    //Wypisanie barmanów
    for(int i=0; i<NUMBER_OF_BARMANS;i++){
        attron(COLOR_PAIR(barmanState[i]));

        if(barmanState[i]==BARMAN_FREE){
            mvprintw(i+5+NUMBER_OF_CLIENTS,14,"is free              \n");
        }
        if(barmanState[i]==BARMAN_POUR){
            
           mvprintw(i+5+NUMBER_OF_CLIENTS,14,"is pouring            %.1fs/%1.fs     ",barman_curtimefl[i],barman_timeofactfl10[i]);
        }
        if(barmanState[i]==BARMAN_CLEANING){
            
           mvprintw(i+5+NUMBER_OF_CLIENTS,14,"is cleaning glass     %.1fs/%1.fs     ",barman_curtimefl[i],barman_timeofactfl10[i]);
        }
        if(barmanState[i]==BARMAN_WAITING_FOR_MUGS){
            
           mvprintw(i+5+NUMBER_OF_CLIENTS,14,"is waiting for glass  %.1fs/%1.fs     ",barman_curtimefl[i],barman_timeofactfl10[i]);
        }
        attroff(COLOR_PAIR(barmanState[i]));
    }

    

    //Wypisanie kufli
    for(int i=0; i<NUMBER_OF_MUGS;i++){
        attron(COLOR_PAIR(mugState[i]));

        if(mugState[i]==MUG_CLEAN){
           mvprintw(i+8+NUMBER_OF_CLIENTS+NUMBER_OF_BARMANS,14,"is clean                ");
        }
        if(mugState[i]==MUG_DIRTY){
             mvprintw(i+8+NUMBER_OF_CLIENTS+NUMBER_OF_BARMANS,14,"is dirty               ");
        }
        if(mugState[i]==MUG_IN_HAND){

           mvprintw(i+8+NUMBER_OF_CLIENTS+NUMBER_OF_BARMANS,14,"is in hand                 "); 
        }
        if(mugState[i]==MUG_IN_CLEANING){
             mvprintw(i+8+NUMBER_OF_CLIENTS+NUMBER_OF_BARMANS,14,"in cleaning              ");
        }
        attroff(COLOR_PAIR(mugState[i]));
    }
    // Wypisanie kranów
    for(int i=0; i<NUMBER_OF_WASHSTANDS;i++){
        if(cleaningMutex[i].try_lock()){
            cleaningMutex[i].unlock();
            mvprintw(i+8+NUMBER_OF_BARMANS+NUMBER_OF_CLIENTS, 65," free     ");
        }else{
            mvprintw(i+8+NUMBER_OF_BARMANS+NUMBER_OF_CLIENTS, 65,"busy      ");
        }
    }
   // for(int i=0; i< NUM)
    //Wypisanie kolejki:
    attron(COLOR_PAIR(2));
    while(!queueMutex.try_lock()){
        usleep(10000);
    }
    mvprintw(9+NUMBER_OF_CLIENTS+NUMBER_OF_BARMANS+NUMBER_OF_MUGS,14,"queue to bar : %d",queueClients.size());
    

    for(int i=0;i<NUMBER_OF_CLIENTS;i++){
        if(i<queueClients.size()){
            mvprintw(11+i+NUMBER_OF_MUGS+NUMBER_OF_CLIENTS+NUMBER_OF_BARMANS,14,"Client: %d",queueClients[i]+1);
        }else{
            mvprintw(11+i+NUMBER_OF_MUGS+NUMBER_OF_CLIENTS+NUMBER_OF_BARMANS,14,"              ");
        }
        
    }
    queueMutex.unlock();
    attroff(2);

    //Wypisanie kolejki do WC 
    attron(COLOR_PAIR(3));
    while(!queueToToiletMutex.try_lock()){
        usleep(10000);
    }
    mvprintw(9+NUMBER_OF_CLIENTS+NUMBER_OF_BARMANS+NUMBER_OF_MUGS,40,"queue to Toilet : %d",queueToToilet.size());
    for(int i=0;i<NUMBER_OF_CLIENTS;i++){
        if(i<queueToToilet.size()){
            mvprintw(11+i+NUMBER_OF_MUGS+NUMBER_OF_CLIENTS+NUMBER_OF_BARMANS,40,"Client: %d",queueToToilet[i]+1);
        }else{
             mvprintw(11+i+NUMBER_OF_MUGS+NUMBER_OF_CLIENTS+NUMBER_OF_BARMANS,40,"              ");
        }
    }
    queueToToiletMutex.unlock();
    attroff(2);




refresh();
if(displayActive){
    usleep(100);
}
}

}

int main(){

displayThread = thread(GUI);

for(int i=0;i<NUMBER_OF_MUGS;i++){
    mugState[i]=1;
    cleanMugs.push_back(i);

}

for(int i=0;i <NUMBER_OF_CLIENTS;i++){
    WC[i]=0;
}

//Uruchomienie wszystkich klientów

for(int i=0; i<NUMBER_OF_CLIENTS;i++){
    clientLife[i]=thread(clientCycle,i);
}

for(int i=0; i<NUMBER_OF_BARMANS;i++){
    barmanLife[i]=thread(barmanCycle,i);
}


while(isRunning){
int k=getch();
if(k==27){
    refresh();
    isRunning = false;
}else{
    refresh();
}
}

for(int i=0;i<NUMBER_OF_CLIENTS;i++){
    clientLife[i].join();

}

for(int i=0;i<NUMBER_OF_BARMANS;i++){
    barmanLife[i].join();

}

displayActive=false;
displayThread.join();

endwin();
    return 0;
}

//Todo list;
//Toaleta



// Czy wątek GUI nie miesza?
