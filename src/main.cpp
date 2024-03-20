//
//Occupation mémoire au début du développement 20 déc 2023
//Sketch uses 230437 bytes (17%) of program storage space. Maximum is 1310720 bytes.
//Global variables use 20968 bytes (6%) of dynamic memory, leaving 306712 bytes for local variables. Maximum is 327680 bytes.
//================================================================================================================================
// V30.00 Refonte de la structure.
// V30.10 Version divisée en fichiers Funct & NbTimer
// V30_30 Ré installé la classe et les fonctions dans un seul fichier 
//        puisque incapable d'activer les methodes depuis une fonction
// V30.31 Application completement fonctionelle et sommairement testée.
// V30.32
// GrelotV1++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// V30.33 Version développée pour seeed_xiao_esp32s3
// V30.34 Stopper la tune si on désactive le bouton pendant l'exécution.
// V30.35 Version pour Esp32 XIAO
// V30.36 Test des fonctions de SaveBatt et ajout de timer TmrBuzzer
// Clonée en GrelotV3
// V30.37 Gestion du mode TmrBuzzer Ok sans output 440hz....
// V30.38 Enleve Dropbox /Dropbox/Serge/Programmation/GrelotV3/Grelot
// migrer a /Dropbox/Serge/Programmation/ProgVsCode/Grelotv3/

// TODO
// Implanter 440 Hz sur mode TmrBuzzer
// Rendre intermittent 20% On 80% Off  l'alerte de save battery
// Stopper la melodie avant sa fin.
//================================================================================================================================

 #define ledBPin   7  // Blue   
 #define ledGPin   8  // Green
 #define ledRPin   9  // Red  
 #define BpLPin    4  // Bb Led on
 #define BpSPin    5  // Bp Sound on
 #define SoundPin  6  // Sortie de son Tone
//================================================================================================================================

//====================================================================================
#include <Arduino.h>
//#include "WiFi.h" 

char buffer[80];  // Pour l'impression de sprintfchar buffer[40];  // Pour l'impression de sprintf
const int channel= 0;   // # de timer ESP32 Sound
//================================================================================================================================
#pragma region 
//================================================================================================================================
// Classe NbTimer
//================================================================================================================================

class NbTimer
{
   public:
      NbTimer(unsigned long interval , int timerType);

      void StartTimer(int duree);        // Démarre le timer.
      void TimerUpdate(void);           // Rafraichir doit etre appelé
      int timeToGo;                     // TTG Donne le temps qui reste.
      int isTimeDone;                   // =1 Si temps écoulé
      int TimeJustDone;                 // TJD 0: Not done 1: Just done
                                        // 2: Done depuis plus de 1 scan
                                        // a chaque scan.
      int timerType =0;                 // TTY 1 Free run  0 One shot 
    
   private:    
      void resetTimer();                // RAZ du timer
      int razTimer ;                    // Demande de RAZ du timer
      unsigned long Interval=0;         // Durée du cycle..
      unsigned long startCount=0;       // Démarrage du timer
      int duree=1;                      // duree : facteur multiplicateur de temps 
      unsigned long CurMillis ;         // Valeur actuelle du timer
};

// Constructeur de la classe NbTimer
// Type-> 0 OneShot 1 Recurrent timer
NbTimer::NbTimer(unsigned long interval,int TimerType){
  Interval=interval;
  timerType = TimerType;
}

// Méthode Update , met a jour les timers.
// Doit etre appelé a chaque scan, pour chaque instance.
void NbTimer::TimerUpdate(void){
  if(this->razTimer==1)resetTimer();
  if(this->TimeJustDone==1)this->TimeJustDone=2;
  this->CurMillis = millis();
  this->timeToGo = this->startCount+(this->Interval*this->duree)-CurMillis;
  if (this->timeToGo<=0)this->timeToGo=0;
  if (this->timeToGo>0)return;
  this->isTimeDone=1;
  if(this->TimeJustDone==0)this->TimeJustDone=1;
  if (this->timerType==0)return;
  this->razTimer=1;
  return;
}

// StartTimer Démarrage du timer dur= n Itervalle.
void NbTimer::StartTimer(int dur){
  if (this->isTimeDone==0)return;
  this->duree = dur;
  this->razTimer=0;
  resetTimer();
  return;
}

void NbTimer::resetTimer(){
  this->startCount =CurMillis;
  this->razTimer=0;
  this->TimeJustDone=0;
  this->isTimeDone=0; 
  return;
}
#pragma endregion
//================================================================================================================================
// Instanciation des timer
//================================================================================================================================
NbTimer TmrBpLed(2000,0);           // Pb bouncing led
NbTimer TmrBpTune(2000,0);          // Pb bouncing musique
NbTimer TmrNote(150,0);             // Tempo de le note
NbTimer TmrPower(15000,1);          // Tempo Power save
NbTimer TmrLed(250,1);              // Tempo Cycle des leds
NbTimer TmrBuzzer(1000,1);          // Tempo Cyclage du buzzer de power on
NbTimer TmrPrint(1000,1);           // Tempo de DEBUG pour imprimer.
//================================================================================================================================
// Fonctions diverses.
//================================================================================================================================
bool ModeL=1;     // Mode L Led Actif
bool ModeT=0;     // Mode T Tune Actif
bool LastModeT=1; // Mode T Last
int NoteInd=0;    // Note Index 

//================================================================================================================================
void PlayNote(void) {
  
// Joue la mélodie
// Playnote ...... Routines de jeu de notes
// La tune se termine par une note == 0
// Si le retour de PlayNote < 0 alors la mélodie est terminée sinon
// NoteId retourne l'indice de la prochaine note a jouer.
// Si la toune n'est pas terminée et que une note ou son internote
// est terminée alors l'appel a PlayNote amorce la diffusion de la
// prochaine note.
//====================================================================================
//                   il, sap  pe   lait  nez  rou  ge   ah  com   il
const int melody[] = {392,392, 440, 392, 330, 262, 440, 392, 393, 440, 392, 440, 392, 262, 494, 349, 392, 349, 294, 494, 440, 392, 392, 440, 392, 440, 392, 440, 330, 392, 440, 392, 330, 262, 440, 392, 392, 440, 392, 440, 392, 262, 494, 349, 392, 349, 294, 494, 440, 392, 392, 440, 392, 440, 392, 294, 262, 440, 440, 262, 440, 392, 330, 392, 349, 440, 392, 349, 330, 294, 330, 392, 440, 494, 494, 494, 262, 262, 494, 440, 392, 349, 294, 392, 440, 392, 330, 262, 440, 392, 392, 440, 392, 440, 392, 262, 494, 349, 392, 349, 294, 494, 440, 392, 392, 440, 392, 440, 392, 294, 262, 0 };
const int beat[]   = {  1,  2,   2,   1,   2,   2,   2,   4,   2,   2,   2,   2,   2,   2,   2,   1,   2,   1,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4,   2,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4,   1,   2,   1,   2,   2,   2,   4, 0 };
//const int beat[] = {50, 100, 100, 50,  100, 100, 100, 200, 100, 100, 100, 100, 100, 100, 100, 50,  100,  50, 100, 100, 100, 200, 50,  100, 50,  100, 100, 100, 200,  50, 100, 50,  100, 100, 100, 200, 50,  100, 50,  100, 100, 100, 200, 50,  100, 50,  100, 100, 100, 200, 50, 100,  50,  100, 100, 100, 200, 50,  100, 50,  100, 100, 100, 200, 100, 100, 100, 100, 200, 50,  100, 50,  100, 100, 100, 200, 50,  100, 50,  100, 100, 100, 200, 50,  100, 50,  100, 100, 100, 200, 50,  100, 50,  100, 100, 100, 200, 50,  100, 50,  100, 100, 100, 200, 50,  100, 50,  100, 100, 100, 200, 0 };
//====================================================================================
// Si on désactive la tune pendant que ca joue. V30.34 
if (NoteInd!=0 && ModeT==0) {
  NoteInd=0;
  ledcWriteTone(channel,0);     // Stopper la note
return;
}
// Est-ce la 1e note 0 et ModeT est il actif.
// Jouer cette note
if (NoteInd==0 && ModeT==1) {
 sprintf(buffer,"                                       PLAYNOTE NoteInd %d ",NoteInd);
 Serial.println(buffer);
 //tone(SoundPin, melody[NoteInd], beat[NoteInd]*50);  
 ledcWriteTone(channel,melody[NoteInd]);   // Jouer la note  
 TmrNote.StartTimer(beat[NoteInd]);                
++ NoteInd;  // Pointer la prochaine note
return;
}
//====================================================================================
// Si le temps est écoulé, que ce n'est pas la note 0
// et pas la derniere note, Jouer la prochaine note.

if(TmrNote.TimeJustDone && NoteInd!=0 && beat[NoteInd]!=0){

 ledcWriteTone(channel,melody[NoteInd]);              // Jouer la note  
 TmrNote.StartTimer(beat[NoteInd]);                  // A and wait for same ms
 ++ NoteInd;  // Et Pointer la prochaine note

  sprintf(buffer,"PLAYNOTE ++++++++ After PLAYING NoteInd: %d Freq: %d Beat: %d",
   NoteInd,melody[NoteInd],beat[NoteInd]);
   Serial.println(buffer);

return;
}
//====================================================================================

// Si c'est la dernière note désactiver ModeT et pointer 
// la note 0 .
if(ModeT==1 && beat[NoteInd]==0){
  ModeT=0;
  NoteInd=0;
  ledcWriteTone(channel,0);     // Stopper la note
return;
}
}
//================================================================================================================================
bool bSaL = 0; 
bool bSaT = 0;
//================================================================================================================================

int LedStat = 1 ; // Etat des led 0:off 1:R 2:B 3:G

void LedOn(void)    // Séquence des leds 1 Red 2 Green 3 Blue
{

if(ModeL==0){
    digitalWrite(ledRPin,0); 
    digitalWrite(ledGPin,0); 
    digitalWrite(ledBPin,0); 
  return;
}
if (TmrLed.TimeJustDone==1){
  LedStat=LedStat+1;
  if (LedStat==4)LedStat=1;  // Entre 1 et 3

  if (LedStat==1){
    digitalWrite(ledRPin,1); 
    digitalWrite(ledGPin,0); 
    digitalWrite(ledBPin,0); 
  }
   if (LedStat==2){
    digitalWrite(ledRPin,0); 
    digitalWrite(ledGPin,1); 
    digitalWrite(ledBPin,0); 
  }
   if (LedStat==3){
    digitalWrite(ledRPin,0); 
    digitalWrite(ledGPin,0); 
    digitalWrite(ledBPin,1); 
  }
  return;
}
}
//================================================================================================================================
int BpStat=0;
int mode =0;

int GetInputL(void) // Lecture des boutons Leds
{

// Gestion de la fonction Led
BpStat = digitalRead(BpLPin);
if(TmrBpLed.timeToGo==0 && bSaL==0){
if (BpStat==0){
  bSaL=1;
  TmrBpLed.StartTimer(1);
  ModeL = !ModeL;
  return 1;
}}
if(TmrBpLed.timeToGo==0 && bSaL==0){
if (BpStat==1){
  return 2;
}}
if(TmrBpLed.timeToGo!=0 && bSaL==1){
if (BpStat==1){
  return 3;
}}
if(TmrBpLed.timeToGo==0 && bSaL==1){
if (BpStat==0){
  return 4;
}}
if(TmrBpLed.timeToGo==0 && bSaL==1){
if (BpStat==1){
 bSaL = 0; 
  return 5;
}}
if(TmrBpLed.timeToGo!=0 && bSaL==1){
if (BpStat==0){
  bSaL = 1;
  return 6; 
}}
return 999;
}
//================================================================================================================================

int GetInputT(void) // Lecture des boutons Tune

// Gestion de la fonction TUNE
{
BpStat = digitalRead(BpSPin);


if(TmrBpTune.timeToGo==0 && bSaT==0){
if (BpStat==0){
  bSaT=1;
  TmrBpTune.StartTimer(1);
    ModeT = !ModeT;
  return 11;
}}
if(TmrBpTune.timeToGo==0 && bSaT==0){
if (BpStat==1){
  return 12;
}}
if(TmrBpTune.timeToGo!=0 && bSaT==1){
if (BpStat==1){
  return 13;
}}
if(TmrBpTune.timeToGo==0 && bSaT==1){
if (BpStat==0){
  return 14;
}}
if(TmrBpTune.timeToGo==0 && bSaT==1){
if (BpStat==1){
 bSaT = 0; 
  return 15;
}}
if(TmrBpTune.timeToGo!=0 && bSaT==1){
if (BpStat==0){
  bSaT = 1;
  return 16; 
}}
return 999;
}

//================================================================================================================================
void Mode(void){ 
// Controle des fonctions. 
GetInputL();
GetInputT();
return ;
}

//================================================================================================================================
bool BuzzOn =0 ;
int Spo = 0 ;   // Status power off  
bool v2b0 =0;        // Value 2 buttons off
int AlmCyc =0 ;       // Cyclage de alarme.
int done =0 ;        // Pour solution d'un bug
int SavBatt(void)  // Alerte de sauvegarde batteries
{
v2b0 = !ModeL&&!ModeT; // 2 états a OFF 

bool front=0;

// Detection du front de frontière de cycle
if(TmrPower.timeToGo!=0) done=0; 
if (done==1)return 0 ;
if (TmrPower.TimeJustDone==1){
  done=1; front =1;
}

if(front==1) { // Front montant
  front=0;
// 2 boutons inactifs
 if(v2b0==1 && Spo==0){Spo=1; return 1;}
 if(v2b0==1 && Spo==1){Spo=1; BuzzOn=1; return 2;}
 if(v2b0==1 && Spo==2){Spo=0; return 3;}
}

// Au mons 1  bouton actif
if(front!=1) {
 if(v2b0==0 && Spo==1){Spo=2; return 4;}
 if(v2b0==0 && BuzzOn==1){Spo=0; BuzzOn=0; return 5;}
}

//  Cyclage du buzzer 
if(TmrBuzzer.TimeJustDone==1){
  AlmCyc = AlmCyc +1;
  if (AlmCyc>=4)AlmCyc=0;
}

if (BuzzOn==1 && AlmCyc ==1 && TmrBuzzer.TimeJustDone==1)ledcWriteTone(channel,440);   // Jouer la note
if (BuzzOn==1 && AlmCyc ==2 && TmrBuzzer.TimeJustDone==1)ledcWriteTone(channel,0);     // Stopper la note


return 99;
}

//================================================================================================================================
// Main
//================================================================================================================================



//====================================================================================
void setup() {
//====================================================================================

// Définition des I/O

ledcAttachPin(SoundPin,channel);  // Assigner la sortie au son.
//int ledRPin = 32; // Red led  
//int ledGPin = 33; // Green led
//int ledBPin = 25; // Blue led 

pinMode(ledRPin, OUTPUT);       // Led Red
pinMode(ledGPin, OUTPUT);       // Led Green
pinMode(ledBPin, OUTPUT);       // Led Blue
pinMode(BpLPin, INPUT_PULLUP);  // Pb Mode Led
pinMode(BpSPin, INPUT_PULLUP);  // Pb Tune Led
  
Serial.begin(115200);  // Activer la console pour Debug

// Disable le Wifi  et BT pour possiblement sauver de l'énergie.
// Consomme 557Kb 40% de mémoire FLASH
//WiFi.disconnect(true);
//WiFi.mode(WIFI_OFF);
//btStop();

// Démarrage des timers récurrents.

TmrPrint.StartTimer(1); TmrLed.StartTimer(1);
TmrBuzzer.StartTimer(1); TmrPower.StartTimer(1);

}

void loop() {

// Rafraichit les timers a chaque cycle
 TmrBpLed.TimerUpdate();TmrBpTune.TimerUpdate(); 
 TmrNote.TimerUpdate(); TmrPower.TimerUpdate(); TmrLed.TimerUpdate(); 
 TmrBuzzer.TimerUpdate(); TmrPrint.TimerUpdate();


// Appelle les fonctions.

SavBatt();  // Buzz si poweron trop long sans utiliser.
Mode();     // Lit les boutons poussoir et ajuste le mode
LedOn();    // Gère la séquence des Leds
PlayNote(); // Joue la mélodie.


// Impression de debugging a la seconde.
if(TmrPrint.TimeJustDone){
  sprintf(buffer," MAIN Mode Led  %d  Tune  %d  Buzz  %d ",ModeL,ModeT,BuzzOn);
  Serial.println(buffer);

  // sprintf(buffer," MAIN AlmCyc %d", AlmCyc);
  // Serial.println(buffer);
} 
}

