#include <ArduinoSTL.h>
#include<iostream>
#include<stdio.h>

//--------------------- Header Files for RFID and Arduino Modules -----------------------

#include <SPI.h>
#include <MFRC522.h>
 
#define SS_PIN 10
#define RST_PIN 9
#define buzPin 4
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

const int billButton = 6;
const int trigPin = 2;
const int echoPin = 3;

using namespace std;

//------------------------------ DATA USED IN THIS CODE -------------------------------

struct ITEM //Note 1
{
    String productName, uid;
    int price;
    int dd, mm, yyyy;
};

ITEM list[10]; //Note 2
vector<ITEM> inCart;

int inStock = 4;    //No. of items in the store
int total = 0;
bool expired;

long duration;
int dis;
int safetydis;

//-------------------------------INITIALIZING STOCKS ------------------------------------

void initializeStocks()
{
    list[0].productName = "Milk"; 
    list[0].uid = "3C DE 9A 51"; // P ID
    list[0].price = 250; 
    list[0].dd = 31; 
    list[0].mm = 12;
    list[0].yyyy = 2021;  //31st Dec 2021 //EXPIRED

    list[1].uid = "5C 32 B5 51"; 
    list[1].productName = "Butter"; 
    list[1].price = 349;
    list[1].dd = 31;
    list[1].mm = 12; 
    list[1].yyyy = 2023; //NOT EXPIRED

    list[2].uid = "C5 3E 68 05"; 
    list[2].productName = "Bread"; //Plain card
    list[2].price = 500;
    list[2].dd = 31; 
    list[2].mm = 12; 
    list[2].yyyy = 2023; //NOT EXPIRED

    list[3].uid = "20 95 ED 1C"; //Blue tag
    list[3].productName = "Egg"; 
    list[3].price = 150;
    list[3].dd = 31; 
    list[3].mm = 4;
    list[3].yyyy = 2022; //EXPIRED
}

int alreadyInCart(String pname)
{
    for(auto &a : inCart)
    {
        if(pname == a.productName)
        {
            return 1;
        }
    }
    return -1;
}

int getIndex(String pname)
{
  for(int i=0; i<inStock; i++)
  {
    if(pname == list[i].productName)
    {
      return i;
    }
  }

  return -1;
}

void addProduct(String pname)
{
    for(int i=0; i<inStock; i++)
    {
        if(pname == list[i].productName)
        {
            inCart.push_back(list[i]);
            Serial.print(pname);
            Serial.println(" successfully ADDED to your cart!");
            Serial.println();
            total += list[i].price;
            break;
        }

        else if(i==(inStock-1))
        {
            Serial.println("Product not found in the database! Please contact manager");
            Serial.println();
        }
    }
}

void removeProduct(String pname)
{
    vector<ITEM>::iterator it;

    for(it=inCart.begin(); it!=inCart.end(); ++it)
    {
      //if(pname.compare(it.productName) == 0)pname == it.productName
      if(pname == (*it).productName)
      {
            total -= (*it).price;
            inCart.erase(it);
            Serial.print(pname);
            Serial.println(" successfully REMOVED from your cart!");
            Serial.println();
            return;
      }
    }
}

String getProductName(String uidName)
{
  int i;
  for(i=0; i<inStock; i++)
  {
    if(list[i].uid == uidName)
    {
      return list[i].productName;
    }
  }

  if(i==inStock)
    return "Not Available";
}

void checkExpiry(int i)
{
  // int date = tm.tm_mday;
  // int month = tm.tm_mon+1;
  // int year = tm.tm_year+1900;
  
  int date = 15; 
  int month = 6; 
  int year = 2022;

  //expiry year greater than present year
  if(list[i].yyyy> year)
  { 
    expired = false; 
    return;
  }

  //expiry year same as present year
  if(list[i].yyyy == year)
  { 
    if(list[i].mm > month)
    { 
      expired = false; 
      return;
    }
    if(list[i].mm == month)
    { 
      if(list[i].dd>date)
      { 
        expired = false; 
        return;
      }
    }
    Serial.println("Alert! Product Expired!");
  }

  expired = true; 
  digitalWrite(buzPin, HIGH);
  delay(100);
  digitalWrite(buzPin, LOW);
}

void generateBill()
{
  Serial.println("Generating Bill:");
  int count = 1;
  for(auto i=inCart.begin(); i!=inCart.end(); i++)
  {
    Serial.print(count++);
    Serial.print("\t");
    Serial.print(i->productName);
    Serial.print("\t");
    Serial.print(i->price);
    Serial.print("\t");
    Serial.println();
  }

  Serial.print("Total Amount: ");
  Serial.println(total);
  Serial.println();
}

//---------------------------------- SET UP FUNCTION ------------------------------------
void setup() 
{
    Serial.begin(9600);   // Initiate a serial communication
    SPI.begin();      // Initiate  SPI bus
    mfrc522.PCD_Init();   // Initiate MFRC522
    Serial.println("Welcome to the Store! Happy Shopping :)");
    Serial.println();
    initializeStocks();

    pinMode(billButton, INPUT);
    pinMode(buzPin, OUTPUT);

    pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
    pinMode(echoPin, INPUT); // Sets the echoPin as an Input
}

//---------------------------------- LOOP FUNCTION ---------------------------------------
void loop()
{

    // Clears the trigPin
    digitalWrite(trigPin, LOW);

    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
 
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
 
    // Calculating the dis
    dis= duration*0.034/2;
 
    safetydis = dis;
    if (safetydis <= 15)   
    {
      digitalWrite(buzPin, HIGH);
    }
    else
    {
      digitalWrite(buzPin, LOW);
    }

    // Look for new cards   
    // If card not present
    if ( ! mfrc522.PICC_IsNewCardPresent())     
    {
        return;
    }
    if ( ! mfrc522.PICC_ReadCardSerial()) 
    {
        return;
    }
    
    //-------------------------------------------------------------------------------------------
    //If card is present
    //Show UID on serial monitor
    //Serial.print("UID tag :");
    String content= "";
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++) 
    {
        //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        //Serial.print(mfrc522.uid.uidByte[i], HEX);
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.println();
    //Serial.print("Message : ");
    content.toUpperCase();
    //Serial.print("Printing content: ");
    //Serial.println(content);
    //--------------------------------------------------------------------------------------------

    String scannedProduct, scannedItemName = content.substring(1);
    scannedProduct = getProductName(scannedItemName);
    
    int index = getIndex(scannedProduct);

    if(index != -1)
      checkExpiry(index);

    //if product is already in cart
    if(alreadyInCart(scannedProduct) == 1)
        removeProduct(scannedProduct);
        
    //if product is not in cart 
    else
        addProduct(scannedProduct);

    if(digitalRead(billButton) == HIGH)
    {
      Serial.println("\nGenerating your bill...\n");
      generateBill();
      Serial.println("\n\nThank You for Shopping with us!\n\n");
    }

    delay(2000);
}
