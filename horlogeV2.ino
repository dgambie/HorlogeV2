

// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       horlogeV2.ino
    Created:	15/10/2019 18:02:40
    Author:     PC_FIXE\David-Fixe
	Remarque : Ajouter une fonction pour permettre l'ajout d'un ntp par la console
				Ajouter capteur de temperature => ok
				Ajouter un message meteo recupere sur le serveur meteofrance
				Ameliorer le watchdog pour plus d'info


	Attention : utiliser l'Initialisation de l'eeprom avant !!
	EEPROM : @dress : 	0 ete
						10 SSID
						100 MDP
						150 tempoMessage
						160 actualisation
*/

// Define User Types below here or use a .h file
//

#include <DHT.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

//************Couleurs******************
#define RESET_SCR       "\033[0m"
#define BLACK       "\033[30m" /* Black */
#define RED           "\033[31m" /* Red */
#define GREEN      "\033[32m" /* Green */
#define YELLOW    "\033[33m" /* Yellow */
#define BLUE         "\033[34m" /* Blue */
#define MAGENTA "\033[35m" /* Magenta */
#define CYAN         "\033[36m" /* Cyan */
#define WHITE       "\033[37m" /* White */ 
/*Un exemple :
UARTprintf(RED"Failed test\n"RESET);
ou plus simplement
UARTprintf("\033[31mFailed test\n\033[0m");*/

//**************************MATRICE-LED*********************************************
// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW // FC16_HW pour l'avoir dans le bon sens
#define MAX_DEVICES 4
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D6
#define PAUSE_TIME    2000
#define SCROLL_SPEED  50
#define TEMPO_MSSG 60000
#define BUF_SIZE 240 // taille du buffer pour le tableau de caractere
#define ADDETE 0
#define ADDSSID 10
#define ADDMDP 50
#define ADDTEMPO_MESSAGE 100
#define ADDACTUALISATION 110
#define FLASH_SIZE 128
#define TEMPSESSAI 60000
#define ACTUALISATION 1
#define DHTPIN D3
#define DHTTYPE DHT11


// Hardware SPI connection
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);


// Variable et Constante du projet :
char* pchaineSSID;
char chaineSSID[BUF_SIZE];
char* pchaineMDP;
char chaineMDP[BUF_SIZE];
char* ssid = NULL;
char* password = NULL;
WiFiServer server(80); // on instancie un serveur ecoutant sur le port 80
char chaine[BUF_SIZE];
char* pchaine;
bool maintient_message = false;
String message = "";
bool ete = false;
int actualisation; // variable pour gerer l'actualisation des messages temperature et humidite
int tempoMessage; //variable pour gerer la tempo des messages
int heureActuelle; // pour le contr�le de l'heure - pour pallier au bug ntp
int heurePrecedente;
int minutePrecedente;
int secondePrecedente;
bool messageErreurNtp = true;
int temperature = 0; // variable pour stocker la temperature
int humidite = 0; // " " " humidite


//Pour recuperer l'heure sur un serveur de temps
WiFiClient espClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.unice.fr", 3600, 60000);
NTPClient timeClient2(ntpUDP, "ntp.midway.ovh", 3600, 60000); // ntp de secours

//Pour le capteur temperature/humidite :
//dht11 DHT11;
DHT dht(DHTPIN, DHTTYPE);


//***************************************************SETUP*****************************************************************************************
//*                The setup() function runs once each time the micro-controller starts
//*************************************************************************************************************************************************
void setup()
{
	String Strssid;
	String Strpassword;
	bool testCo = false; // booleen pour le test de la connexion
	int essaiCo = 0;
	unsigned long oldTime = millis();
	unsigned long currentTime = millis();

	//Init de la com serie
	Serial.begin(115200);

	//Init de la matrice a led :
	P.begin();


	//Lecture de l'EEPROM //Attention, utiliser l'init EEprom avant pour charger l'eeprom
	EEPROM.begin(FLASH_SIZE);
	EEPROM.get(ADDETE, ete);
	delay(50);
	EEPROM.get(ADDTEMPO_MESSAGE,tempoMessage);
	EEPROM.get(ADDACTUALISATION,actualisation);
	EEPROM.end(); // pour liberer la memoire
	Strssid = lecture_EEPROM(ADDSSID); // recuperation du SSID dans l eeprom
	Strpassword = lecture_EEPROM(ADDMDP);
	Strssid.toCharArray(chaineSSID, BUF_SIZE); // conversion de Str vers tableau de char
	Strpassword.toCharArray(chaineMDP, BUF_SIZE);
	ssid = &chaineSSID[0]; // pointeur pour ssid
	password = &chaineMDP[0];

	//Affichage des info dans la console
	Serial.println("****************************************************************");
	Serial.print("SSID : ");
	Serial.print(ssid);
	Serial.print("\tMDP : " BLACK);
	Serial.print(password);
	Serial.print(RESET_SCR "\tETE : ");
	Serial.println(ete);
	Serial.println("****************************************************************");

	//Activation du watchdog mat�riel :
	ESP.wdtDisable();
	Serial.println("Demarrage - appuyer sur espace dans les 2 secondes pour lancer la configuration"); // Message de configuration

	unsigned long tempsPrecedent = millis();
	unsigned long tempsActuel = millis();

	while ((tempsActuel - tempsPrecedent) < 2000) {
		tempsActuel = millis();
		if (Serial.available()) {
			Serial.println("****************************");
			Serial.println("* Console de configuration *");
			Serial.println("****************************");
			configuration();
		}		
	}
	Serial.println("*****************************");

	// on demande la connexion au WiFi
	WiFi.begin(ssid, password);

	// on attend d'etre connecte au WiFi avant de continuer *-*--*-*-*-*Attention, pr�voir un message defilant en cas d'impossibilit� de connection
	while (WiFi.status() != WL_CONNECTED || testCo == true) {	
		delay(500);
		Serial.print(".");
		essaiCo = essaiCo + 1;
		if (essaiCo > 10) { // 10 essais de connexion, tempo de 60 secondes durant le message defilant puis nouvelle tentative de connexion
			ESP.wdtDisable();
			Serial.println(RED "Erreur de connexion WIFI" RESET_SCR);
			AffichageText("Erreur Wifi, verifier votre connexion !",60000,false);
			Serial.println("Nouvelle tentative...");
			essaiCo = 0;		
		}
	}

	// on affiche l'adresse IP qui nous a ete attribuee
	Serial.print("\nIP address: " GREEN);
	Serial.print(WiFi.localIP());
	Serial.println(RESET_SCR);

	// on commence a ecouter les requetes venant de l'exterieur
	server.begin();

	//Debut du sereur ntp
	timeClient.begin();
	timeClient2.begin(); // d�marrage du ntop de secours
	timeClient.update();//update de l'heure
	heurePrecedente = timeClient.getHours();
	minutePrecedente = timeClient.getMinutes();
	secondePrecedente = timeClient.getSeconds();

	//Initialisation du capteur
	dht.begin();

	//Acceuil pour la config
	//Serial.print("Configuration de l'horloge :\n");
	Serial.println("****************************");
	Serial.println("* Console de configuration *");
	Serial.println("****************************");
	Serial.println("> Presser espace pour commencer le reglage");//message de d�but de programme
}
//***********************************************************************LOOP****************************************************************
//*                                        Add the main program code into the continuous loop() function
//*******************************************************************************************************************************************
void loop()
{
	timeClient.update();//update de l'heure
	//int minuteActuelle = timeClient.getMinutes();
	if (Serial.available()) { // Si appuis sur une touche, on lance la config
		configuration();
	}

	//Recuperation de l'heure dans des variables 
	String strHeure = Heure_str(timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds(), ete);

	//Verification de l'heure affiche
	heureActuelle = timeClient.getHours();
	if (verifHeure(heureActuelle, heurePrecedente))
	{
		String strHeure = Heure_str(timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds(), ete);
	}
	else {
		String strHeure = Heure_str(timeClient2.getHours(), timeClient2.getMinutes(), timeClient2.getSeconds(), ete);
	}

	if (abs(timeClient.getMinutes() - minutePrecedente) > actualisation) {
		Serial.println("....");
		AffichageText(Get_TempHum(strHeure), 20000, false);
		//actualisation(strHeure);
		minutePrecedente = timeClient.getMinutes();
	}

	//actualisation(strHeure);
	//Serial.println("Boucle");
	Luminosite(timeClient.getHours());
	AffichageHeure(strHeure);
	heurePrecedente = timeClient.getHours();


	switch (decode(Communication_web())) {
	case 1:
		if (maintient_message == HIGH){
			AffichageText("Maintient OK", 5000, false);
		}
		else {
			AffichageText("Maintient NOK", 5000, false);
		}
		break;

	case 2:
		if (ete == HIGH){
			AffichageText("Ete OK", 5000, false);
		}
		else {
			AffichageText("Hiver OK", 5000, false);
		}
		break;

	case 3:
		AffichageText(Get_TempHum(strHeure), 9000, false);
		break;

	case 4:
		AffichageText("Temporisation modifiee", 5000, false);
		break;
	
	case 5:
		AffichageText("Actualisation modifiee", 5000, false);
		break;		

	case 99:
		AffichageText(message, tempoMessage, maintient_message);
		break;

	default:
		break;
	}

	

}
int decode(String reception) {
	char recept[2];
	char decode[4] = "GET";

	//Verification de la reception :
	for (int i = 0; i < 3; i++) {
		recept[i] = reception[i];
		if (recept[i] != decode[i]) {
			return 0;
		}
		if (i == 2) {//Si on arrive a 3 c'est qu'on a la presence d'un GET
			Serial.println("###################OK################");
			Serial.print(reception);

			//Analyse du message recu
			if (reception.indexOf("param") != -1) {
				//AffichageText("Parametrage", 1000);//Si c'est un message de parametrage
				int start = reception.indexOf(':');
				int stop = reception.lastIndexOf('*');
				String param = reception.substring(start + 1, stop);
				//AffichageText(param, 2000, false);
				if (param == "maintient") {
					/*
					AffichageText("Maintient OK", 5000, false);
					maintient_message = true;*/
					maintient_message = !maintient_message;
					return 1;
				}

				if (param == "ete") {
					ete = !ete;
					//Sav dans l'EEprom
					EEPROM.begin(128);
					EEPROM.put(ADDETE, ete);
					EEPROM.commit();
					EEPROM.end(); // pour liberer la memoire
					return 2;
				}

				if (param == "temperature") {
					return 3;
				}
				
				if (reception.indexOf("tempo") != -1){
					int start = reception.indexOf("-");
					int stop = reception.lastIndexOf("*");
					String stgTempo = reception.substring(start+1,stop);
					tempoMessage = stgTempo.toInt();
					Serial.print("La nouvelle temporisation de message est de : ");
					Serial.print(tempoMessage / 1000);
					Serial.println(" seconde(s)");
					EEPROM.begin(128);
					EEPROM.put(ADDTEMPO_MESSAGE, tempoMessage);
					EEPROM.commit();
					EEPROM.end(); // pour liberer la memoire
					return 4;
				}

				if (reception.indexOf("actualisation") != -1){
					int start = reception.indexOf("-");
					int stop = reception.lastIndexOf("*");
					String stgActualisation = reception.substring(start+1,stop);
					actualisation = stgActualisation.toInt();
					Serial.print("La nouvelle actualisation de la temperature de message est de : ");
					Serial.print(actualisation);
					Serial.println(" minute(s)");
					EEPROM.begin(128);
					EEPROM.put(ADDACTUALISATION, actualisation);
					EEPROM.commit();
					EEPROM.end(); // pour liberer la memoire
					return 5;
				}

			}
			if (reception.indexOf("message") != -1){
				int start = reception.indexOf(':');
				int stop = reception.lastIndexOf('*');
				message = reception.substring(start +1, stop);
				message.replace('_',' ');
				Serial.print("Le message recu est : ");
				Serial.println(message);
				return 99;				
			}

			else {
				Serial.println("Pas de message decode");
				return -1;
			}

		}
	}

}

int Luminosite(int heure) {
	//Fonction pour gerer la luminosite de l'affichage en fonction de l'heure
	if (heure > 8 && heure < 19)
	{
		P.setIntensity(7);
		return 0;
	}

	else
	{
		P.setIntensity(1);
		return 1;
	}
}

String Heure_str(int heure, int minute, int seconde, bool ete) {
	String strHeure = "";
	String strMinute = "";
	String strSeconde = "";
	int modulo; // pour savoir si les secondes sont paire
	String separateur = " ";
	ESP.wdtFeed();//Reset du wdt

	if (minute < 10) {
		strMinute = "0" + String(minute);
	}

	else {
		strMinute = String(minute);
	}


	if (ete) {
		strHeure = String(heure + 1);
	}

	else {
		strHeure = String(heure);
	}

	//Gestion du minuit :
	if (strHeure == "24") {
		strHeure = "00";
	}

	if (seconde < 10) {
		strSeconde = "0" + String(seconde);
	}

	else {
		strSeconde = String(seconde);
	}

	//Pour faire clignoter le s�parateur
	modulo = seconde % 2;

	if (modulo == 0) {
		separateur = " ";
	}

	else {
		separateur = ":";
	}

	return strHeure + separateur + strMinute;
}

void AffichageHeure(String heure) {
	//Reset du wdt
	ESP.wdtFeed();
	heure.toCharArray(chaine, BUF_SIZE);//conversion en tableau de caractere
	P.displayText(chaine, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
	P.displayAnimate();

}

void AffichageText(String text, int tempo, bool maintient) {
	ESP.wdtFeed();//Reset du wdt
	text.toCharArray(chaine, BUF_SIZE);//conversion en tableau de caractere
	unsigned long oldTime = millis();
	unsigned long currentTime = millis();

	if (maintient == true) {
		while (maintient == true)
		{
			ESP.wdtFeed();
			if (P.displayAnimate()) {
				P.displayText(chaine, PA_LEFT, 50, 200, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
			}
			if (decode(Communication_web()) != 0)
				break;
		}
	}
	else
	{
		while (currentTime <= (oldTime + tempo)) {
			ESP.wdtFeed();
			if (P.displayAnimate()) {
				P.displayText(chaine, PA_LEFT, 50, 200, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
			}
			currentTime = millis();
			yield();
		}
	}
}

String Communication_web() {

	WiFiClient client = server.available();
	if (!client) {
		return "";
	}
	// Wait until the client sends some data
	Serial.println("new client");
	while (!client.available()) {
		delay(1);
	}
	// Read the first line of the request
	String req = client.readStringUntil('\r');
	client.flush();
	int val;

	if (req.indexOf("GET") != -1)
		val = 0;
	else {
		Serial.println("invalid request");
		client.stop();
		return "";
	}

	String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nCommande recu";

	s += "</html>\n";
	client.print(s);
	delay(1);
	Serial.println("Client disonnected");
	return req;

}

String serial() {
	if (Serial.available()) {// teste s'il reste des donn�es en attente
		String reception = "";
		bool fin = false;

		while (fin == false) {
			while (Serial.available()) { //tant que des donn�es sont en attente
				char c = Serial.read(); // lecture
				Serial.print(c);
				if (c == '\r' || c == '\n') {
					fin = true;
				}
				else {
					reception += String(c); //on ajoute � l'objet reception
					delay(10); //petite attente
				}
			}
		}
		reception.trim(); //on enl�ve le superflu en d�but et fin de cha�ne
		if (Serial.available()) {
			while (Serial.available()) {
				char c = Serial.read();
			}
		}
		return reception;
	}
}

void configuration() {
	String newSSID;
	String newMDP;
	bool configuration = true;
	String reception;
	Serial.println("Configuration");
	Serial.println("-----------------");
	Serial.println(" 1 : SSID");
	Serial.println(" 2 : MDP");

	while (configuration) {
		reception = serial();

		if (reception == "1") {
			Serial.println("Configuration du SSID :");
			newSSID = configSSID();
			vide_Eeprom(ADDSSID, ADDSSID + 49);
			ecriture_EEPROM(ADDSSID, newSSID);
			configuration = false;
		}
		if (reception == "2") {
			Serial.println("Configuration du MDP :");
			newMDP = configMDP();
			vide_Eeprom(ADDMDP, ADDMDP + 99);
			ecriture_EEPROM(ADDMDP, newMDP);
			configuration = false;
		}
	}
	Serial.println("Fin de la configuration, restart ...\n\n\n");
	delay(1000);
	restart();
}

String configSSID() {
	String reception;
	bool ssid = true;

	while (ssid) {
		if (Serial.available()) {
			reception = serial();
			ssid = false;
		}
	}

	reception.toCharArray(chaineSSID, BUF_SIZE);//conversion en tableau de caractere
	pchaineSSID = &chaineSSID[0];
	return reception;
}

String configMDP() {
	String reception;
	bool mdp = true;

	while (mdp) {
		if (Serial.available()) {
			reception = serial();
			mdp = false;
		}
	}
	reception.toCharArray(chaineMDP, BUF_SIZE);//conversion en tableau de caractere
	pchaineMDP = &chaineMDP[0];
	return reception;
}

void restart() {
	ESP.restart();
}

void ecriture_EEPROM(int adresseDebut, String text) {
	int i = 0;
	char c = text[0];
	Serial.println("Ecriture dans EEPROM");
	EEPROM.begin(FLASH_SIZE);

	while (c != '\0') {
		EEPROM.put(adresseDebut + i, c);
		EEPROM.commit();
		delay(100);
		i++;
		c = text[i];
	}
	EEPROM.put(adresseDebut + i, '\0');
	EEPROM.end();
}

String lecture_EEPROM(int adresseDebut) {
	int i = 0;
	String lecture;
	char c;

	Serial.println("Lecture dans EEPROM");
	EEPROM.begin(FLASH_SIZE);
	EEPROM.get(adresseDebut, c);

	while (c != '\0') {
		lecture += c;
		i++;
		EEPROM.get(adresseDebut + i, c);
	}
	EEPROM.end();
	return lecture;
}

void vide_Eeprom(int addrDebut, int addrFin) {
	Serial.print("Erasing EEPROM de addr : ");
	Serial.print(addrDebut);
	Serial.print(" a addr : ");
	Serial.println(addrFin);

	EEPROM.begin(FLASH_SIZE);
	for (int i = addrDebut; i < addrFin; i++) {
		EEPROM.put(i, '*');
		EEPROM.commit();
	}
	EEPROM.put(addrFin, '\0');
	EEPROM.commit();
	EEPROM.end();
}

char* convert_Str_to_Pchar(String text) {
	text.toCharArray(chaine, BUF_SIZE);//conversion en tableau de caractere
	pchaine = &chaine[0];
	return pchaine;
}

bool verifHeure(int heureActuelle, int heurePrecedente) {

	if (abs(heureActuelle - heurePrecedente) > 1) {
		if (messageErreurNtp) { // permet d'afficher le message une fois, reviens si il tombe a nouveau apres que le premier ait marche
			Serial.println("Heure incoherente, Recuperation sur un autre ntp");
			messageErreurNtp = false;
		}

		timeClient2.update(); // update du client de secours
		if (abs(timeClient2.getHours() - heurePrecedente) > 1) {
			Serial.println("Deuxieme serveur ntp down, reboot de la carte...");
			ESP.restart();
		}
		else {
			return false;
		}
	}
	else {
		messageErreurNtp = true;
		return true;
	}
}

String Get_TempHum( String heure) {
	float tabData[2] = { 0,0 };
	String temperature;
	String humidite;
	tempHum(tabData);

	Serial.println("Actualisation :");
	Serial.print("heure : ");
	Serial.print(heure);
	Serial.print("\t temperature : " RED);
	Serial.print(tabData[0]);
	Serial.print(RESET_SCR "\t Humidite : " BLUE);
	Serial.print(tabData[1]);
	Serial.println(RESET_SCR);

	temperature = String(tabData[0],1);
	humidite = String(tabData[1],0);

	return "Temperature : " + temperature + "\xB0" + "C Humidite : " + humidite + "%";

}

void tempHum(float *tabData) {//Fonction pour la lecture de la temperature et de l'humidite => remplissage d'un tableau a deux entrees
	delay(1000);
	*tabData = dht.readTemperature()-5.5;
	*(tabData + 1) = dht.readHumidity();

}

void Page_Main_HTML(){
	//Construction de la page main pour la configuration
	
}
