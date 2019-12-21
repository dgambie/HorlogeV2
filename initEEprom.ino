//Programme d'initialisation de l'EEPROM pour le projet HorlogeV2

#include <EEPROM.h>


#define ADDSSID 10
#define ADDMDP 50
#define ADDTEMPO_MESSAGE 100
#define ADDACTUALISATION 110
#define FLASH_SIZE 128

//Variable a mettre dans l'eeprom
bool ete = 1; //Pour l'heure d'ete ou l'heure d'hiver
int tempoMessage = 60000;
int actualisation = 5;
bool lectEte; // pour la lecture
int lectTempoMessage;
int lectActualisation;
String lectText; // pour la lecture d'un string
bool eeprom = LOW;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  EEPROM.begin(FLASH_SIZE);
  Serial.print("Taille EEPROM : ");
  Serial.println(EEPROM.length());
  //Ecrtiure de lete
  EEPROM.put(0, ete);
  //pour copier le cache mémoire en eeprom ( l'écriture se fait à ce nomment la )
  EEPROM.commit();
  delay(100);

  //ecriture tempo message
  EEPROM.put(ADDTEMPO_MESSAGE, tempoMessage);
  EEPROM.commit();
  delay(100);

   //ecriture actualisation
  EEPROM.put(ADDACTUALISATION, actualisation);
  EEPROM.commit();
  delay(100);
  
  EEPROM.end(); // pour liberer la memoire

  ecriture_EEPROM(ADDSSID,"********\0"); // reservation de 50
  ecriture_EEPROM(ADDMDP,"********************\0");//reservation de 100
  
}

void loop() {
  // put your main code here, to run repeatedly:

  if (!eeprom)
  {
    EEPROM.begin(FLASH_SIZE);
    EEPROM.get(0, lectEte);
    delay(100);
    EEPROM.get(ADDTEMPO_MESSAGE,lectTempoMessage);
    delay(100);
    EEPROM.get(ADDACTUALISATION,lectActualisation);
    delay(100);
    EEPROM.end(); // pour liberer la memoire
    delay(100);
    Serial.print("La variable ete est positionne sur  : ");
    Serial.println(lectEte);
    Serial.print("La variable tempo message est positionne sur  : ");
    Serial.println(lectTempoMessage);
    Serial.print("La variable actualisation est positionne sur  : ");
    Serial.println(lectActualisation);
    lectText = lecture_EEPROM(ADDSSID);
    Serial.print ("La variable SSID est positionne sur : ");
    Serial.println(lectText);
    lectText = lecture_EEPROM(ADDMDP);
    Serial.print ("La variable MDP est positionne sur : ");
    Serial.println(lectText);
    eeprom = HIGH;
    
  }
  

}

void ecriture_EEPROM (int adresseDebut, String text){
  int i = 0;
  char c = text [0];
  Serial.println("Ecriture dans EEPROM");
  EEPROM.begin(FLASH_SIZE);
  
  while (c !='\0'){
    Serial.print("i : ");
    Serial.print(i);
    Serial.print("\tAddr : ");
    Serial.print(adresseDebut + i);
    Serial.print("\tdata : ");
    Serial.println(c);

    EEPROM.put(adresseDebut + i, c);
    EEPROM.commit();
    delay(100);
    i++;
    c = text[i];
  }
  EEPROM.put(adresseDebut + i, '\0');
  EEPROM.end();
}

String lecture_EEPROM (int adresseDebut){
  int i = 0;
  String lecture;
  char c;

  Serial.println("Lecture dans EEPROM");
  EEPROM.begin(FLASH_SIZE); 
  EEPROM.get(adresseDebut, c);
 
  while (c != '\0') {
    Serial.print("i : ");
    Serial.print(i);
    Serial.print("\tAddr : ");
    Serial.print(adresseDebut + i);
    Serial.print("\tdata : ");
    Serial.println(c);
    lecture += c;
    i++;
    EEPROM.get(adresseDebut + i, c);
  }
  EEPROM.end();
  return lecture;
}
