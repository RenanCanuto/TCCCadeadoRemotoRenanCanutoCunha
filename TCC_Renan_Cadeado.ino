///Bibliotecas
#include <WiFi.h> //Wifi
#include <WiFiClientSecure.h> //Wifi
#include <UniversalTelegramBot.h> //Biblioteca do bot do Telegram
#include <ArduinoJson.h> //Biblioteca para acessar sites
#include <ESP32Servo.h> //Biblioteca para controlar o servo motor pelo ESP32
#include <EEPROM.h>

#define SERVO_PIN 35 // GPIO26

Servo servoMotor;

// Dados do WiFi
#define WIFI_SSID "E948_Fibra"
#define WIFI_PASSWORD "BVGdH5e3"

// Telegram BOT Token (Botfather)
#define BOT_TOKEN "6852203315:AAEaRHtPti2rQguxuIoOrfzLKg9DiMunCS0"

//Tempo entre escaneamento de mensagens
const unsigned long BOT_MTBS = 1000;
unsigned long bot_lasttime;

// Use @myidbot (IDBot) para saber qual o seu ID
#define CHAT_ID "6698578450"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// Número de chats distintos em que o bot pode estar presente
#define MAX_CHAT_IDS 10 // Número máximo de chat_ids que você deseja armazenar
String chatIds[MAX_CHAT_IDS]; // Lista para armazenar chat_ids
int numChatIds = 0; // Contador para o número de chat_ids na lista

//Estado do cadeado
bool cadeadoAberto = true;

//Opções de tranca:
bool cadeadoRotacao = true;
bool solenoide = false;

///////COMPONENTES__
//Fim de curso
#define FIM_CURSO_PIN 34

void setup()
{

  ///Setup componentes
  pinMode(FIM_CURSO_PIN, INPUT);
  servoMotor.attach(SERVO_PIN);  // attaches the servo on ESP32 pin

  Serial.begin(115200);
  Serial.println();

  // Inicializa a EEPROM
  EEPROM.begin(512);

  // Carrega os chat_ids armazenados anteriormente
  loadChatIdsFromEEPROM();

  //conexão da rede:
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org");  // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  bot.sendMessage(CHAT_ID, "Bot iniciou", "");//envia mensagem dizendo que iniciou o BOT
}

void handleNewMessages(int numNewMessages)
{
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    
    // Adiciona chat_id à lista se não estiver presente
    bool chatIdExists = false;
    for (int j = 0; j < numChatIds; j++)
    {
      if (chatIds[j] == chat_id)
      {
        chatIdExists = true;
        break;
      }
    }

    if (!chatIdExists && numChatIds < MAX_CHAT_IDS)
    {
      chatIds[numChatIds] = chat_id;
      numChatIds++;
    }

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    ///Abrir Cadeado
    if(text == "/abrir")
    {
      if(cadeadoAberto == false)
      {
        sendMessageToAll(from_name + " está ABRINDO o cadeado");
       
        AbrirCadeado();

        sendMessageToAll("Cadeado Aberto");
      }
      else
      {
        sendMessageToAll("O cadeado está aberto");
      }
    }

    ///Fechar Cadeado
    if(text == "/fechar")
    {
      if(cadeadoAberto == true)
      {
        sendMessageToAll(from_name + " está FECHANDO o cadeado");

        FecharCadeado();

        sendMessageToAll("Cadeado Fechado");
      }
      else
      {
        sendMessageToAll("O cadeado está fechado");
      }
    }

    if (text == "/estado")
    {
      if(cadeadoAberto)
      {
        sendMessageToAll("O cadeado está ABERTO");
      }
      else
      {
        sendMessageToAll("O cadeado está FECHADO");
      }
    }

    if(text == "/comandos")
    {
      String welcome = "Bem vindo ao bot cadeado\n";
      welcome += "/abrir: Comando para abrir o cadeado\n";
      welcome += "/fechar: Comando para fechar o cadeado\n";
      welcome += "/estado: Comando para verificar o estado do cadeado";
      sendMessageToAll(welcome);
    }

    if(text == "/configuracao")
    {

    }
  }
}

void verificarFimCurso(String chat_id) {
  int estadoFimCurso = digitalRead(FIM_CURSO_PIN);
  if (estadoFimCurso == HIGH) {
    cadeadoAberto = false;
    bot.sendMessage(chat_id, "A chave fim de curso está acionada. Cadeado trancado.");
  } else {
    cadeadoAberto = true;
    bot.sendMessage(chat_id, "A chave fim de curso está desacionada. Cadeado destrancado.");
  }
}

// Função para enviar mensagem para todos os chat_ids na lista
void sendMessageToAll(String message)
{
  for (int i = 0; i < numChatIds; i++)
  {
    bot.sendMessage(chatIds[i], message);
  }

  saveChatIdsToEEPROM();
}

void AbrirCadeado()
{
  if(cadeadoRotacao == true)
  {
    // rotates from 0 degrees to 180 degrees
    for (int pos = 0; pos <= 180; pos += 1)
    {
      // in steps of 1 degree
      servoMotor.write(pos);
      delay(15); // waits 15ms to reach the position
    }
  }
  else if(solenoide == true)
  {
    digitalWrite(SOLENOIDE_PIN, LOW);
  }
  
  cadeadoAberto = true;
}

void FecharCadeado()
{
  if(cadeadoRotacao == true)
  {
    // rotates from 180 degrees to 0 degrees
    for (int pos = 180; pos >= 0; pos -= 1)
    {
      servoMotor.write(pos);
      delay(15); // waits 15ms to reach the position
    }

  }
  else if(solenoide == true)
  {
    digitalWrite(SOLENOIDE_PIN, HIGH);
  }
  else if()
  {

  }

    //verificarFimCurso();
    cadeadoAberto = false;
}

// Funções para manipulação da EEPROM
void saveChatIdsToEEPROM()
{
  for (int i = 0; i < numChatIds; i++)
  {
    EEPROM.writeString(i * 10, chatIds[i]);
  }
  EEPROM.commit();
}

void loadChatIdsFromEEPROM()
{
  for (int i = 0; i < MAX_CHAT_IDS; i++)
  {
    String chatId = EEPROM.readString(i * 10);
    if (chatId.length() > 0)
    {
      chatIds[i] = chatId;
      numChatIds++;
    }
  }
}

void loop() 
{
  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
}