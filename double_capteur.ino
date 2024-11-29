#include <TFT_eSPI.h>          // Bibliothèque pour l'écran
#include <Wire.h>              // Communication I2C
#include <Adafruit_VL53L0X.h>  // Bibliothèque pour le capteur ToF
#include <ESP32Servo.h>

//APPLI
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

Servo myservo;  // create servo object to control a servo

// Initialisation de l'écran et des capteurs
TFT_eSPI tft = TFT_eSPI();
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();

int voiture = 0;  // Compteur des voitures détectées
int pos = 180;    // variable to store the servo position

//APPLI
bool deviceConnected = false;
bool oldDeviceConnected = false;
BLECharacteristic *pCharacteristic;
BLEServer *pServer;
String valor;
String alea;

// Définition des broches XSHUT pour les capteurs
#define XSHUT_LOX1 25
#define XSHUT_LOX2 26

// LEDs pour les feux
#define V_ENTREE 17  // LED Vert Entrée
#define J_ENTREE 2 // LED Jaune Entrée
#define R_ENTREE 27 // LED Rouge Entrée
#define V_SORTIE 15 // LED Vert Sortie
#define J_SORTIE 13 // LED Jaune Sortie
#define R_SORTIE 12 // LED Rouge Sortie

//APPLI
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


//APPLI

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue();
     
	  if (value.length() > 0) {
        valor = "";
        for (int i = 0; i < value.length(); i++){
          // Serial.print(value[i]); // Presenta value.
          valor = valor + value[i];
        }

        Serial.println("*********");
        Serial.print("valor = ");
        Serial.println(valor); // Presenta valor.
		
        if(valor == "1") {
          myservo.write(90);
          Serial.println("Servo à 90 °");
        
          }
        else if(valor == "0") {
          myservo.write(180);
          Serial.println("Servo à 0 °");
          }
      }
    }
};


void setup() {
  Serial.begin(115200);
  myservo.attach(32);
  // Initialisation de l'écran
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);

  // Initialisation des broches XSHUT pour capteurs
  pinMode(XSHUT_LOX1, OUTPUT);
  pinMode(XSHUT_LOX2, OUTPUT);

  // Initialisation des LEDs pour les feux
  pinMode(V_ENTREE, OUTPUT);
  pinMode(J_ENTREE, OUTPUT);
  pinMode(R_ENTREE, OUTPUT);
  pinMode(V_SORTIE, OUTPUT);
  pinMode(J_SORTIE, OUTPUT);
  pinMode(R_SORTIE, OUTPUT);

  // Configuration des capteurs VL53L0X
  Wire.begin();

  // Capteur 1
  digitalWrite(XSHUT_LOX1, LOW);
  digitalWrite(XSHUT_LOX2, LOW);
  delay(10);
  digitalWrite(XSHUT_LOX1, HIGH);
  delay(10);
  if (!lox1.begin(0x30)) {
    tft.println("Erreur capteur 1");
    while (1);
  }
  // Capteur 2
  digitalWrite(XSHUT_LOX2, HIGH);
  delay(10);
  if (!lox2.begin(0x31)) {
    tft.println("Erreur capteur 2");
    while (1);
  }

  init_feux();
  tft.println("System Ready!");
  delay(2000);
  tft.fillScreen(TFT_BLACK);
  
  // APPLI   
  Serial.begin(115200);
  BLEDevice::init("ChahanESP32");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Iniciado.");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void loop() {
  VL53L0X_RangingMeasurementData_t measure1;
  VL53L0X_RangingMeasurementData_t measure2;

  // Lecture des données du capteur 1
  lox1.rangingTest(&measure1, false);
  if (measure1.RangeStatus != 4 && measure1.RangeMilliMeter < 155) {
    voiture++; // Incrémenter le compteur
    afficherCompteur();
    Serial.println("Capteur 1 : Voiture entrée");

    feuVertEntree(); // Passer au vert
    openservo();
    while (true) {
      lox1.rangingTest(&measure1, false);
      if (measure1.RangeStatus == 4 || measure1.RangeMilliMeter > 155) {
        break; // Sortir de la boucle lorsque la voiture n'est plus détectée
      }
    }
    transitionEntree(); // Transition progressive au feu rouge
    closeservo();
  }

  // Lecture des données du capteur 2
  lox2.rangingTest(&measure2, false);
  if (measure2.RangeStatus != 4 && measure2.RangeMilliMeter < 140) {
    voiture--; // Décrémenter le compteur
    afficherCompteur();
    Serial.println("Capteur 2 : Voiture sortie");

    feuVertSortie(); // Passer au vert
    openservo();
    while (true) {
      lox2.rangingTest(&measure2, false);
      if (measure2.RangeStatus == 4 || measure2.RangeMilliMeter > 140) {
        break; // Sortir de la boucle lorsque la voiture n'est plus détectée
      }
    }
    transitionSortie(); // Transition progressive au feu rouge
    closeservo();
  }

  if (deviceConnected){
    String value = pCharacteristic->getValue();
    delay(5);
  }
  if (!deviceConnected && oldDeviceConnected){
    delay(500);
    pServer->startAdvertising();
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected){
    oldDeviceConnected = deviceConnected;
  }
}

// Fonction pour éteindre tous les feux
void eteindreFeux_1() {
  digitalWrite(V_ENTREE, HIGH);
  digitalWrite(J_ENTREE, HIGH);
  digitalWrite(R_ENTREE, HIGH);
}

void eteindreFeux_2(){
  digitalWrite(V_SORTIE, HIGH);
  digitalWrite(J_SORTIE, HIGH);
  digitalWrite(R_SORTIE, HIGH);
}

// Fonction pour allumer les feux d'entrée
void feuRougeEntree() {
  eteindreFeux_1();
  digitalWrite(R_ENTREE, LOW);
}
void init_feux(){
  digitalWrite(V_ENTREE, HIGH);
  digitalWrite(J_ENTREE, HIGH);
  digitalWrite(R_ENTREE, LOW);
  digitalWrite(V_SORTIE, HIGH);
  digitalWrite(J_SORTIE, HIGH);
  digitalWrite(R_SORTIE, LOW);
}

void feuVertEntree() {
  eteindreFeux_1();
  digitalWrite(V_ENTREE, LOW);
}

void feuJauneEntree() {
  eteindreFeux_1();
  digitalWrite(J_ENTREE, LOW);
}

// Fonction pour allumer les feux de sortie
void feuRougeSortie() {
  eteindreFeux_2();
  digitalWrite(R_SORTIE, LOW);
}

void feuVertSortie() {
  eteindreFeux_2();
  digitalWrite(V_SORTIE, LOW);
}

void feuJauneSortie() {
  eteindreFeux_2();
  digitalWrite(J_SORTIE, LOW);
}

// Transition progressive pour le feu d'entrée
void transitionEntree() {
  digitalWrite(V_ENTREE, HIGH); // Éteindre la LED verte
  delay(200); // Pause entre chaque transition
  feuJauneEntree(); // Allumer le feu orange
  delay(1000); // Maintenir le feu orange
  digitalWrite(J_ENTREE, HIGH); // Éteindre la LED orange
  delay(1000); // Pause avant le rouge
  feuRougeEntree(); // Allumer le feu rouge
}

// Transition progressive pour le feu de sortie
void transitionSortie() {
  digitalWrite(V_SORTIE, HIGH); // Éteindre la LED verte
  delay(200); // Pause entre chaque transition
  feuJauneSortie(); // Allumer le feu orange
  delay(2000); // Maintenir le feu orange
  digitalWrite(J_SORTIE, HIGH); // Éteindre la LED orange
  delay(500); // Pause avant le rouge
  feuRougeSortie(); // Allumer le feu rouge
}

void openservo(){
  pos = 90;
  myservo.write(pos);                                  
}

void closeservo(){
  pos =180;
  myservo.write(pos);   
}


// Fonction pour afficher le compteur sur l'écran TFT
void afficherCompteur() {
  tft.fillScreen(TFT_BLACK); // Efface l'écran
  tft.setCursor(10, 40);
  tft.setTextSize(3);
  tft.println("Voiture:");
  tft.setCursor(10, 90);
  tft.setTextSize(5);
  tft.printf("%d", voiture);
}

