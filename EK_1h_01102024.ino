// IL PROGRAMMA RICEVE IN UN PERIODO TEMPORALE STABILITO DALLA VARIABILE <Integrazione>  I SEGUENTI DATI NELL'ORDINE:
// DATA e ORARIO * TEMPERATURA * PRESSIONE * SEGNALI -
// I DATI ACCUMULATISI NEL PERIODO PRESTABILITO VENGONO SALVATI IN ORDINE TEMPORALE NELLA MEMORIA <SD> E SONO RILEGGIBILI
// SUL MONITOR SERIALE OPPURE DA TERMINALE.
// IL CICLO SI RIPETE ALL'INFINITO - Mod.121022-Mod.20/11/2022 aggiunto partenza misure con orario prestabilito - 24/11/2022 Aggiunto su CT conteggio misure

// **********  PER AGGIORNARE LA DATA E L'ORA RIFERIRSI AL PROGRAMMA ESTERNO ' RTCSync Terminal ' ***********
// **********  PER MODIFICARE L'HEADER DEL PROGRAMMA RIFERIRSI ALLA RIGA 370  ************
#include "Adafruit_BMP085.h"
#include "RTClib.h"
#include <Wire.h>
#include <SD.h>
//---------------     PIN SELECT   --------------------
#define chipSelect    10 // per scheda rtc/sd - controllare 4 e 9
#define interruptPin  2 // numero pin interrupt - controllare 3 e 2
#define interruptType FALLING
#define LED_Pin       7 // pin indicatore LED

//---------- Definizione dell'elenco dei comandi via seriale  ------------
#define LeggiFile     "readdata"  //!!!!!!!!!!! !!!!!!!!!!!   Comando che scrive il file TXT sulla seriale !!!!!!!!!! !!!!!!!!!!!!   !!!!!!!!!!!

//------------------   DEFINIZIONI VARIABILI  --------------------
Adafruit_BMP085 bmp;
RTC_DS3231 rtc;
#define STRBUFFER_LEN 20
char stringBuffer[STRBUFFER_LEN];
bool exitFlag = false;

//*******************************************************************************************************************************
//*******************************************************************************************************************************
#define TOTSecDay         86400// NON MODIFICARE

//!!!!!!! STABILISCE IL TEMPO DI INTEGRAZIONE  !!!!!!!!
#define Ore             1  // ****** Definisce le ORE di Integrazione ******
#define Minuti          0  // ****** Definisce i MINUTI di Integrazione ******
// ***** Esempio: Ore = 1, Minuti = 25, Tempo di integrazione = 1h e 25m
#define luogo       F(" Ekar")
#define modalita    F("Device EKAR; Array 22+22 SBM19 - DISTANZA mm.120 + Array 10+10 SBM19 - DISTANZA mm.100")
#define programma   F("EK-_1h_01102024")
#define FileName    F("log1.txt") // Nome del file dati- era "log0001.txt"

File myFile;
#define MeasureStartTime  0    // per ora NON UTILIZZATO, definisce l'offset di partenza delle misure in secondi
// ***** Esempio: MeasureStartTime = 1200s (20min) e tempo di integrazione 1h
// ***** la cadenza oraria delle misure sarà: 00:20 - 01:20 - 02:20 ecc. quindi 20min di offset
//!!!!!!!

const unsigned int Integrazione = (Ore * 3600) + (Minuti * 60);
const unsigned char MeasureNum = TOTSecDay / Integrazione;
//*******************************************************************************************************************************
//*******************************************************************************************************************************

DateTime Time;

bool primaMisura = true;
bool SegnaleArrivato = false;
bool SD_Ok = true;
unsigned long nMisura = 0;// aggiunto numero misure
unsigned int Segnale = 0;
unsigned int n = 0;
float Cpm = 0;



// ---------------  DEFINIZIONE FUNZIONI --------------
void ControlloSegnale(bool LedBlink = true);
void StampaInfo();
void StampaFileTerminale();
void SalvaMisura(bool stampa = true);
void LEDBlink();
bool TempoTrascorso();
void SerialListener();
void RTCSync();

// ------- Setup Arduino ----------
void setup()
{
  Serial.begin(9600);
  bmp.begin();               // attiva sensore pressione-temperature BMP180

  pinMode(LED_Pin, OUTPUT);   // LED segnala ricezione segnale e fine lavoro

  Wire.begin();
  rtc.begin();               // inizializza RTC3231

  Time = rtc.now();
  StampaInfo();

  if (!SD.begin(chipSelect)) //  inizializza scheda Catalex SD
  {
    Serial.println(F("\n*** ERRORE Memoria SD, impossibile salvare le misure ***"));
    Serial.println(F("*** Le misure verranno stampate sul terminale ma NON salvate ***\n"));
    //exitFlag = true;
    SD_Ok = false;
  }
  else
  { //-------------- PREPARA FILE PER SALVATAGGIO -------------
    myFile = SD.open(FileName, FILE_WRITE);
    Serial.println(myFile.available());
  }
} // end setup

// ------------ Loop di Arduino  -------------

void loop()
{
  SerialListener();

  ControlloSegnale();
}

//===================================================================================================================

// -------------------  FUNZIONE PROCESSAMENTO SEGNALE  -------------------
void ControlloSegnale(bool LedBlink = true)
{
  if (SegnaleArrivato)
  {
    Segnale++;

    if (LedBlink == true)
      LEDBlink();

    SegnaleArrivato = false;
  }

  if (exitFlag == false)
  {
    if (TempoTrascorso())
    {
      Time = rtc.now();
      SalvaMisura(LedBlink);
    }
  }
}

// -------------------  Funzione confronto tempo trascorso - tempo di integrazione  -------------------
bool TempoTrascorso()
{
  DateTime rtcnow = rtc.now();
  float r = TOTSecDay - (rtcnow.second() + (rtcnow.minute() * 60.0) + (rtcnow.hour() * 3600.0) - MeasureStartTime);
  r = r / Integrazione;

  if ( int(r) == r)
  {
    if (primaMisura == true)
    {
      Segnale = 0;
      Serial.print(" - Inizio Misure ");
      sprintf(stringBuffer, "%04d/%02d/%02d %02d:%02d:%02d",
              rtcnow.year(), rtcnow.month(), rtcnow.day(),
              rtcnow.hour(), rtcnow.minute(), rtcnow.second());
      Serial.print(stringBuffer);
      Serial.print(" UTC ");
      Serial.println(" *** WAIT,I'm working ***\n");
      //------------------INTERRUPT (SEGNALE)--------------------
      attachInterrupt(digitalPinToInterrupt(interruptPin), ricevi, interruptType);// Legge il segnale DIGITALE sui pin 2 oppure 3.

      primaMisura = false;
      delay(1100);
      return false;
    }
    else
      return true;
  }

  return false;
}

//-------------------  Funzione per salvare la misura  -------------------
void SalvaMisura(bool stampa = true)
{
  Cpm = Segnale / (Integrazione / 60.0); // ***  PER CALCOLO CPM
  
  double pressure = bmp.readPressure() / 100.0; // hPa
  double temperature = bmp.readTemperature();
  
  nMisura++;// conta le misure su CT
  // ----------------- Disabilito l'interrupt sul pin 2  ------------------
  detachInterrupt(digitalPinToInterrupt(interruptPin));

  //----------------   STAMPA I RISULTATI A VIDEO   --------------------
  if (stampa == true)
  {
    Serial.println();
    Serial.print(F("   "));
    sprintf(stringBuffer, "%04d/%02d/%02d ", Time.year(), Time.month(), Time.day());
    Serial.print(stringBuffer);
    sprintf(stringBuffer, "%02d:%02d:%02d", Time.hour(), Time.minute(), Time.second());
    Serial.print(stringBuffer);
    Serial.print(F(" * "));
    Serial.print(temperature);
    Serial.print(F(" * "));
    Serial.print(pressure);
    Serial.print(F(" * "));
    Serial.print(Segnale);

    delay(10);
  }
  if (SD_Ok)
  {
    unsigned long filesize = myFile.size();
    //---------------------- SALVATAGGIO SU SD   ----------------------------
    myFile.println();
    myFile.print(F("   "));
    sprintf(stringBuffer, "%04d/%02d/%02d ", Time.year(), Time.month(), Time.day());
    myFile.print(stringBuffer);
    sprintf(stringBuffer, "%02d:%02d:%02d", Time.hour(), Time.minute(), Time.second());
    myFile.print(stringBuffer);
    myFile.print(F(" * "));
    myFile.print(bmp.readTemperature());
    myFile.print(F(" * "));
    myFile.print(bmp.readPressure() / 100.00);
    myFile.print(F(" * "));
    myFile.print(Segnale);

    delay(10);

    //----------------  SVUOTA LA CACHE DI <MyFile>  ------------------
    if (filesize == myFile.size())
      Serial.println(F("/------ ERRORE nel salvataggio della misura ------/"));
    else
      Serial.println(F("- Salvataggio misura: OK"));

    myFile.flush();
  }
  else
  {
    Serial.println(F("- Salvataggio misura DISABILITATO: Errore memoria SD"));
  }


  delay(10);
  Segnale = 0;
  n = 0;

  // ----------------- Abilito l'interrupt sul pin 2  ----------------
  attachInterrupt(digitalPinToInterrupt(interruptPin), ricevi, interruptType);
  delay(1100);
}

// -------------------  FUNZIONE RICEZIONE SEGNALE  -------------------
void ricevi()
{
  SegnaleArrivato = true;
}

//------------------ LAMPEGGIO ARANCIO PER SEGNALE RICEVUTO  -----------
void LEDBlink()
{
  digitalWrite(LED_Pin, HIGH);  // Led lampeggia all'arrivo di ogni segnale
  delay(40); // delay(100); //40 è il valore più appropriato
  digitalWrite(LED_Pin, LOW);

  // ----- STAMPA UN ASTERISCO OGNI 500 SEGNALI RICEVUTI ------

  if (n == 500)
  {
    Serial.print(F("*"));
  }
  
  n++;
}


// -------------------  Funzione di scrittura txt su terminale  -------------------

void StampaFileTerminale()
{
  int fc = 10;

  if (SD_Ok == false)
  {
    Serial.println(F("/------ Impossibile leggere il file: Errore memoria SD ------/"));
    return;
  }

  File readFile = SD.open(FileName);
  delay(100);

  do
  {
    ControlloSegnale(false);

    if (readFile)
    {
      Serial.println();
      Serial.println(programma);
      Serial.println(F("/------ Inizio scrittura del file da 1h sul terminale ------/"));

      while (readFile.available())
      {
        ControlloSegnale(false);
        Serial.write(readFile.read());
      }

      readFile.close();
      delay(100);

      Serial.println();
      Serial.println();
      Serial.println(F("/------  Fine scrittura file da 1h sul terminale  ------/"));

      fc = 0;
    }
    else
      readFile = SD.open(FileName);

    delay(10);
    fc--;

  } while (fc > 1);

  if (fc == 1)
  {
    Serial.println();
    Serial.println(F("/------ ERRORE NELLA STAMPA DEL FILE SUL TERMINALE ------/"));
  }
}

// -------------------  Funzione per ricezione comandi da terminale  -------------------
void SerialListener()
{
  if (Serial.available() > 0)
  {
    String SerialMessage = Serial.readString();

    if (SerialMessage == LeggiFile)
      StampaFileTerminale();
    else if (SerialMessage == "remfile") // NON FUNZIONA CON FILE GRANDI
    {
      Serial.println();
      if (SD.remove(FileName))
      {
        Serial.println();
        Serial.println(F("File Dati rimosso, connettersi nuovamente"));
        exitFlag = true;
      }
      else
        Serial.println(F("Impossibile rimuovere il file dati"));

    }
    else if (SerialMessage == "RTCSYNC")
    {
      RTCSync();
    }
    else
    {
      Serial.println();
      Serial.println(SerialMessage);
      Serial.println(F("Comando non riconosciuto"));
    }

    Serial.flush();
  }
}

void RTCSync()
{
  unsigned int Y, M, D, h, m, s;

  Serial.print("RTCSYNC_ACK\n");
  delay(50);

  if (Serial.available() > 0)
  {
    //char cc=Serial.read();
    String SerialMessage = Serial.readString();

    int i = SerialMessage.indexOf('|');
    Y = SerialMessage.substring(0, i).toInt();

    SerialMessage.remove(0, i + 1);
    i = SerialMessage.indexOf('|');
    M = SerialMessage.substring(0, i).toInt();

    SerialMessage.remove(0, i + 1);
    i = SerialMessage.indexOf('|');
    D = SerialMessage.substring(0, i).toInt();

    SerialMessage.remove(0, i + 1);
    i = SerialMessage.indexOf('|');
    h = SerialMessage.substring(0, i).toInt();

    SerialMessage.remove(0, i + 1);
    i = SerialMessage.indexOf('|');
    m = SerialMessage.substring(0, i).toInt();

    SerialMessage.remove(0, i + 1);
    i = SerialMessage.indexOf('|');
    s = SerialMessage.substring(0, i).toInt();

    Serial.print("RTCSYNC_OK\n");

    char tempStr[30];
    sprintf(tempStr, "%04u/%02u/%02u %02u:%02u:%02u", Y, M, D, h, m, s);
    Serial.print(tempStr);
    Serial.print("\n");

    rtc.adjust(DateTime(Y, M, D, h, m, s));
  }
}

// -----------------  STAMPA LE INFO AD INIZIO PROGRAMMA -------------------
void StampaInfo()
{
  float r;
  unsigned int t = 0;

  //------------------ TESTI  MODIFICABILI -------------------

  Serial.print (F("\n\n - LUOGO DOVE SI TROVA: ")); Serial.println(luogo);
  Serial.print (F(" - PROGRAMMA: ")); Serial.println(programma);
  Serial.print (F(" - FILE DATI: ")); Serial.println(FileName);
  Serial.print(F(" - MODALITA': ")); Serial.println(modalita);
  Serial.print(F(" - Durata lettura min. "));  Serial.println(Integrazione/60);
  Serial.print(F(" - Data di connessione => "));
  sprintf(stringBuffer, "%04d/%02d/%02d %02d:%02d:%02d",
          Time.year(), Time.month(), Time.day(),
          Time.hour(), Time.minute(), Time.second());
  Serial.print(stringBuffer);
  Serial.println(" UTC");

  do
  {
    r = TOTSecDay - ((Time.second() + (Time.minute() * 60.0) + (Time.hour() * 3600.0)) + t - MeasureStartTime);
    r = r / Integrazione;
    t++;
  } while ( int(r) != r);

  DateTime invertT = (Time + TimeSpan(t - 1));
  Serial.print(" - Le misure inizieranno alle => ");
  Serial.print(invertT.toString("hh:mm:ss"));
  Serial.print(" UTC del ");
  Serial.println(invertT.toString("YYYY/MM/DD"));
}

// **********   END   ***********
