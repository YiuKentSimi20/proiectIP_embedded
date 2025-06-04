//https://web.archive.org/web/20151214164138/http://www.robofun.ro/shield-motoare-l298-v2?search=l298
  #include <Wire.h>
  #include <string.h>
  #include <stdlib.h>
  #include <SoftwareSerial.h>

class Salon {
private:
  String directii;

public:
  Salon(String directii) {
    this->directii = directii;
  }

  String getDirectii() {
    return directii;
  }

};

// Proiectarea hartii spitalului cu rutele saloanelor
Salon saloane[] = {Salon("ss"), Salon("sd"), Salon("fs"), Salon("fd"), Salon("ds"), Salon("dd")};



//scanner card RFID
SoftwareSerial rfidSerial(10, 11); // RX = 2, TX not used

// motoare
int MOTOR2_PIN1 = 3;
int MOTOR2_PIN2 = 5;
int MOTOR1_PIN1 = 6;
int MOTOR1_PIN2 = 9;

//Ultrasonic
const int trigPin = 29;
const int echoPin = 28;


// Senzori IR
int S1 = A11;
int S2 = A10;
int S3 = A9;
int S4 = A8;


const int IR_THRESH = 300;
const unsigned long TIMEOUT = 5000;

bool emergencyStop = false;


// Statusuri comanda
String statusRobot = "";
bool comandaFinalizata = false;
bool ajunsLaDestinatie = false;
bool cardRfidValidat = false;
bool comandaPreluata = false; // false -> comanda in asteptare
int intersectiiTrecute  = 0;
bool modRcActivat = false;
bool liniePierduta = false;


// Detalii comanda
String orderRfid = "";
String orderSalon = "";
String orderPat = "";



void setup() {
  pinMode(MOTOR1_PIN1, OUTPUT);
  pinMode(MOTOR1_PIN2, OUTPUT);
  pinMode(MOTOR2_PIN1, OUTPUT);
  pinMode(MOTOR2_PIN2, OUTPUT);

  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);

  Serial.begin(9600);
  Serial1.begin(9600);
  rfidSerial.begin(9600);

  
}



//––– Helper pentru detecția liniei pe senzorii interiori –––––––––––––
bool checkCenter() {
  int v2 = analogRead(S2);
  int v3 = analogRead(S3);
  return (v2 > IR_THRESH && v3 > IR_THRESH);
}


bool ocolire_obstacol() {
  Serial.println("Ocolire obstacol...");

  float distanta;
  
  go(0, 0);
  delay(200);

  go(-200, -100);
  delay(700);
 
  go(200, -150);
  delay(700);

  go(200, 150);
  delay(1000);
  distanta = getDistanta();
  if(getDistanta() < 5) {
    emergencyStop = true;

    Serial.println("Alarma: Emergency Stop");
    Serial1.println("Alarma: Emergency Stop");
    return false;
  }
  delay(700);
 
  go(-200, 150);
  delay(800);

  // go(150, 150);
  // delay(1000);
  while(!checkCenter()) {
    go(200, 150);
    delay(50);
  }
  go(0,0);
  delay(100);

  while(!checkCenter()) {
    go(200, -150);
    delay(50);
  }
  go(0,0);
  
  Serial.println("Ocolire finalizata.");
  

  return true;

}

void virare_intersectie() {

  char directieVirare;

  if(!comandaFinalizata) {
    // cod pentru comanda in curs de livrare
    switch(intersectiiTrecute) {

    case 0:
      directieVirare = saloane[orderSalon.toInt()-1].getDirectii().charAt(intersectiiTrecute);
      break;

    case 1: 
      directieVirare = saloane[orderSalon.toInt()-1].getDirectii().charAt(intersectiiTrecute);
      break;

    case 2:
      if (orderPat == "1") {
        directieVirare = 's';
      } else if(orderPat == "2") directieVirare = 'd';
      break;
    default:
      break;

    }
    intersectiiTrecute++;
  

  } else {
    // cod pentru comanda finalizata (intoarcere la farmacie)
    intersectiiTrecute--;
    switch(intersectiiTrecute) {
    case 0:
      directieVirare = saloane[orderSalon.toInt()-1].getDirectii().charAt(intersectiiTrecute);
      if (directieVirare == 's') directieVirare = 'd';
      else if(directieVirare == 'd') directieVirare = 's';

      break;
    case 1: 
    
      directieVirare = saloane[orderSalon.toInt()-1].getDirectii().charAt(intersectiiTrecute);
      if (directieVirare == 's') directieVirare = 'd';
      else if(directieVirare == 'd') directieVirare = 's';
      break;
    case 2:
       if (orderPat == "1") {
        directieVirare = 'd';
      } else if(orderPat == "2") directieVirare = 's';
      break;
    default:
      break;

    }

  }


  if(directieVirare == 's') {

    go(200, 100);
    delay(300);

    go(-200, 150);
    delay(500);

    while(!checkCenter()) {
      go(-200, 150);
      //delay(50);
    }
    go(0,0);

  } else if(directieVirare == 'd') {

    go(200, 100);
    delay(300);

    go(200, -150);
    delay(500);

    while(!checkCenter()) {
      go(200, -150);
      //delay(50);
    }
    go(0,0);

  } else {
    go(200, 100);
    delay(300);
    go(0, 0);
  }
  
}


void repozitionareRobot() {
  while(!checkCenter()) {
    go(-200, 150);
  }

  while(!checkCenter()) {
    go(-200, -100);
  }
  go(0, 0);

  if(modRcActivat) {

    if(comandaFinalizata) { // repozitionare in caz de interventie dupa finalizarea comenzii
      comandaFinalizata = false;
      ajunsLaDestinatie = false;
      cardRfidValidat = false;
      comandaPreluata = false;
      liniePierduta = false;
      intersectiiTrecute = 0;

      Serial1.println("Ajuns la farmacie");
      Serial.println("Ajuns la farmacie");
      


    } else { // repozitionare in caz de interventie in timpul livrarii

      intersectiiTrecute = 0;
      liniePierduta = false;
      Serial1.println("Reluare comanda");
      Serial.println("Reluare comanda");

    }

  } else { //robotul a ajuns inapoi la farmacie fara interventii => comanda finalizata cu success
    comandaFinalizata = false;
    ajunsLaDestinatie = false;
    cardRfidValidat = false;
    comandaPreluata = false;
    intersectiiTrecute = 0;

    Serial1.println("Ajuns la farmacie");
    Serial.println("Ajuns la farmacie");

  }


}

float getDistanta() {
  // Trimit un semnal ultrasonic
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Măsor durata semnalului primit
  long durata = pulseIn(echoPin, HIGH);

  // Convertesc durata în distanță (în cm)
  float distanta = durata * 0.034 / 2;

  return distanta;
}


void urmarire_linie() {
  int s1 = analogRead(S1); // stânga extremă
  int s2 = analogRead(S2); // stânga mijloc
  int s3 = analogRead(S3); // dreapta mijloc
  int s4 = analogRead(S4); // dreapta extremă


  // Dacă durata > THRESH -> vede negru
  bool b1 = (s1 > IR_THRESH);
  bool b2 = (s2 > IR_THRESH);
  bool b3 = (s3 > IR_THRESH);
  bool b4 = (s4 > IR_THRESH);


    // Logică urmărire
  if (!b1 && !b2 && !b3 && !b4) {
    
    go(0, 0);
    if (!comandaFinalizata && intersectiiTrecute == 3) { // robotul a ajuns la punctul de livrare
      ajunsLaDestinatie = true;
      statusRobot = "Ajuns la destinatie";
    } else if(comandaFinalizata && intersectiiTrecute == 0) { // robotul a ajuns inapoi la farmacie

      // Resetare pentru urmatoare comanda;
      orderSalon = "";
      orderPat = "";
      orderRfid = "";
      repozitionareRobot();

    } else { // robotul a pierdut linia pe parcursul navigarii
      if(!liniePierduta) {
        Serial1.println("Alarma: Linie pierduta");
        Serial.println("Alarma: Linie pierduta");
        liniePierduta = true;
      }

    }


  }
  else if(b1 && b2 && b3 && b4 || b1 && b2 && b3 && !b4 || !b1 && b2 && b3 && b4 ) {
    virare_intersectie();
    Serial.println("Virare intersectie");
  }
  else if (b2 && b3) {
    // linie centrată
    go(200, 100);
  }
  else if (b1) {
    // linie spre stânga
    go(0, 100);
  }
  else if (b4) {
    // linie spre dreapta
    go(200, 0);
  }
  else if (b2) {
    // ușor stânga
    go(120, 100);
  }
  else if (b3) {
    // ușor dreapta
    go(200, 80);

  }
  
}

void asteptareRfid() {
  Serial.println("Se asteapta Rfid: " + orderRfid);
  Serial1.println("Se asteapta Rfid: " + orderRfid);
  
  
  String rfidBuffer = "";
  String rfid = "";

  while(!cardRfidValidat) {

    while (rfidSerial.available()) {
      char c = rfidSerial.read();

      // Dacă am primit sfârșit linie (CR sau LF), afișăm bufferul și îl resetăm
      if (c == '\r' || c == '\n') {
        if (rfidBuffer.length() > 0) {
          Serial.print("Cod card: ");
          Serial.println(rfidBuffer);
          rfid = rfidBuffer;
          rfidBuffer = "";  // reset buffer
        }
      }
      else if (isPrintable(c)) {
        rfidBuffer += c;  // adaugă caracter la buffer
      }
    }
      
      if(rfid.equals(orderRfid)) {
        cardRfidValidat = true;
        Serial.println("Rfid validat: " + orderRfid);
        Serial1.println("Rfid validat: " + orderRfid);
      } 

  }


}


void navigare() {
   if(!emergencyStop && comandaPreluata && !modRcActivat) {
    bool obstacol_ocolit = true;
    float distanta = getDistanta();
    if(distanta < 5) {
      Serial.println("Ocolire obstacol");
      Serial1.println("Ocolire obstacol");
      obstacol_ocolit = ocolire_obstacol();
    }
    if(obstacol_ocolit) {
      urmarire_linie();
    }

    if(ajunsLaDestinatie && !comandaFinalizata) {
      //asteapta dupa RFID
      Serial.println("Asteptare RFID");
      Serial1.println("Asteptare RFID");

      // -- Pentru testare fara RFID ---
      // delay(1000);
      // cardRfidValidat = true;

      asteptareRfid();
      delay(1000);

      //citire RFID valid
      comandaFinalizata = true;

      go(200, 100);
      delay(500);
      while(!checkCenter()) {
        go(-200, 150);
      }

      Serial.println("Finalizat: " + orderRfid);
      Serial1.println("Finalizat: " + orderRfid);    
    }
    if(comandaFinalizata) {
      statusRobot = "Intoarcere la farmacie";
    }
  }
}

String serial_input;
String serial_string_aux = "";
char serial_char;

void loop() {

  serial_input = "";


  if (Serial1.available()) { // Daca apare ceva pe Serial1
    serial_char = Serial1.read();
    serial_string_aux += serial_char;
    if(serial_char == '\n'){
    serial_input = serial_string_aux;
    serial_input.trim();
    serial_string_aux = "";
    }
  }

  //comanda primita prin Serial1 ar trebui sa arate de forma: "Comanda [salon] [pat] [RFID]"
  //ex -- "Comanda: 20 7 0A003B61D1"
  if(!serial_input.equals("")) {
    Serial.println(serial_input);

    if(serial_input.equals("RC-ON")) {
      modRcActivat = true;
      Serial.println("Mod RC activat");
    
    }else if(serial_input.equals("RC-OFF")) {
      Serial.println("Mod RC dezactivat");
      repozitionareRobot();
      modRcActivat = false;

    }else if(serial_input.startsWith("Comanda:") && !comandaPreluata) {

      // Găsim începutul datelor (după "Comanda: ")
      int startIndex = serial_input.indexOf("Comanda:");
      if (startIndex != -1) {
        String data = serial_input.substring(startIndex + 9);
        data.trim();

        int space1 = data.indexOf(' ');
        int space2 = data.indexOf(' ', space1 + 1);

        // Extragem valorile
        if (space1 != -1 && space2 != -1) {
          orderSalon = data.substring(0, space1);
          orderPat = data.substring(space1 + 1, space2);
          orderRfid = data.substring(space2 + 1);
          Serial.println("Comanda: " + orderRfid + " a fost inregistrata.");
          statusRobot = "Comanda in curs de livrare";
        }
      }

        comandaPreluata = true;
        
    } else if(modRcActivat) { 
      statusRobot = "Operare manuala";
      char directie = serial_input.charAt(0);
      
      switch (directie) {
        case 'F':
          // statements
          go(200, 100); 
          delay(500);
          break;
        case 'B':
          go(-200,-100);
          delay(500);
          break;
        case 'R':
          go(200,-100);
          delay(500);
          break;
        case 'L':
          go(-200, 100);
          delay(500);
        break;
        default:
          go(0,0);
        break;
      }
    }
  }

  navigare();

  if(!statusRobot.equals("")) {
    //Serial.println(statusRobot);
    Serial1.println(statusRobot);
  }

 
} 

void go(int speedLeft, int speedRight) {
  if (speedLeft > 0) {
    analogWrite(MOTOR1_PIN1, speedLeft);
    analogWrite(MOTOR1_PIN2, 0);
  } 
  else {
    analogWrite(MOTOR1_PIN1, 0);
    analogWrite(MOTOR1_PIN2, -speedLeft);
  }
 
  if (speedRight > 0) {
    analogWrite(MOTOR2_PIN1, speedRight);
    analogWrite(MOTOR2_PIN2, 0);
  }else {
    analogWrite(MOTOR2_PIN1, 0);
    analogWrite(MOTOR2_PIN2, -speedRight);
  }
}