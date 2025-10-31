
#include <WiFi.h>                   // Libreria para conexión del ESP32 al Wi-FI                 
#include <PubSubClient.h>           // Libreria para que el microcontrolador 
                                    // sea un cliente MQTT
#include <PZEM004Tv30.h>            // Libreria para funcionamiento y comunicación 
                                    // del sensor pzem004-t y esp32
                                    

// Credenciales para conexion a red Wi-Fi

const char* ssid = "XXXXXXX";    // SSID de red Wi-FI a conectarse
const char* password = "XXXXXXXX";       // Contraseña de red Wi-FI a conectarse

const char* mqtt_server = "X.X.X.X"; // Direccion IP Broker

WiFiClient espClient1;                      // Creacion de cliente MQTT
PubSubClient client(espClient1);


// Definicion de pines y serial de PZEM-004t del enchufe 1

#define pin_rx_pzem1 32                     // Define pin designado como Rx 
#define pin_tx_pzem1 33                     // Define pin designado como Tx
#define serial_pzem1 Serial1                // Define a la interfaz serial 1

// Definicion de pines y serial de PZEM-004t del enchufe 2
#define pin_rx_pzem2 26                     // Define pin designado como Rx
#define pin_tx_pzem2 27                     // Define pin designado como Tx
#define serial_pzem2 Serial2                // Define a la interfaz serial 2      

// Se define dos objetos de comunicacion serial
// junto con los pines designados como Tx y RX

PZEM004Tv30 pzem_t1(serial_pzem1, pin_rx_pzem1, pin_tx_pzem1);
PZEM004Tv30 pzem_t2(serial_pzem2, pin_rx_pzem2, pin_tx_pzem2);


// Pines y variables para el rele del enchufe 1
#define pin_t1 14                 // Designa al pin de salida para comunicar con el rele
bool estado_relay_t1 = false;     // Estado que evita el inicio activado el rele

// Pines y variables para el rele del enchufe 2
#define pin_t2 25                 // Designa al pin de salida para comunicar con el rele
bool estado_relay_t2 = false;     // Estado que evita el inicio activado el rele


///// VARIABLES DEL PZEM00T DEL ENCHUFE 1 /////
// Creacion varibles electricas
float voltaje_t1;           // Variable de lectura del voltaje
float corriente_t1;         // Variable de lectura de corriente
float potencia_t1;          // Variable de lectura de potencia
float consumo_t1;           // Variable de lectura de energía o consumo

// Varibles a publicar tipo String a Broker
char pVoltaje_t1[20];       // Variable de publicacion MQTT del voltaje
char pCorriente_t1[20];     // Variable de publicacion MQTT de corriente
char pPotencia_t1[20];      // Variable de publicacion MQTT de potencia
char pConsumo_t1[20];       // Variable de publicacion MQTT de energía o consumo

///// VARIABLES DEL PZEM00T DEL ENCHUFE 2 /////
// Creacion varibles electricas
float voltaje_t2;           // Variable de lectura del voltaje
float corriente_t2;         // Variable de lectura de corriente
float potencia_t2;          // Variable de lectura de potencia
float consumo_t2;           // Variable de lectura de energía o consump

// Varibles a publicar tipo String a Broker
char pVoltaje_t2[20];       // Variable de publicacion MQTT del voltaje
char pCorriente_t2[20];     // Variable de publicacion MQTT de corriente
char pPotencia_t2[20];      // Variable de publicacion MQTT de potencia
char pConsumo_t2[20];       // Variable de publicacion MQTT de energía o consumo


// Declaracion variables: tiempo de ultima lectura de sensores-enchufes 1 y 2

unsigned long tiempo_anterior_t1 = 0;
unsigned long tiempo_anterior_t2 = 0;

const unsigned long intervalo = 10000;        // Intervalo de tiempo para lectura de datos

// Estados de calibracion de sensores
static bool sensor_t1_calibrado = false;      // Estado de caliracion de tomacorriente 1
static bool sensor_t2_calibrado = false;      // Estado de caliracion de tomacorriente 2

// Declaracion variables: tiempo de ultima calibracion para sensor-enchufe 1 y 2
unsigned long tanterior_cali_t1 = 0;          // Tiempo última calibreacion sensor-tomacorriente 1
unsigned long tanterior_cali_t2 = 0;          // tiempo última calibreacion sensor-tomacorriente 2

const unsigned long intervalo_calibrar = 5000; // Intervalo de tiempo para calibreacion de sensores

// Variables para el Control del Reseteo Sensores Pzem004-t

bool soli_resetE_t1 = false;                  // Variable para solicitud de reseteo de energia pzem004-t del tomacorriente 1
bool soli_resetE_t2 = false;                  // Variable para solicitud de reseteo de energia pzem004-t del tomacorriente 2

unsigned long tiempo_resetE_t1 = 0;           // Variable para el tiempo transcurrido desde inicio de reseteo de Energia tomacorriente 1
unsigned long tiempo_resetE_t2 = 0;           // Variable para el tiempo transcurrido desde inicio de reseteo de Energia tomacorriente 2

// Variables para evitar bucle en reseteo de los sensores pzem004-t

bool estado_resetE_t1 = false;                // Variable almacena estado del proceso de reseteo de energia tomacorriente 1
bool estado_resetE_t2 = false;                // Variable almacena estado del proceso de reseteo de energia tomacorriente 2


// Variables para envío de estado del Nodo

unsigned long tiempoU_estadoNodo = 0;
const unsigned long intervalo_estadoNodo = 30000;

//Funcion para conectar el ESP32 a la red Wi-Fi
void setup_wifi() {
  delay(10);                                  // Delay de 10 ms
  Serial.println("Conectando a red Wi-Fi");
  WiFi.mode(WIFI_STA);                        // Modo cliente o estacion del ESP32 con el AP
  WiFi.begin(ssid, password);                 // Metodo para establecer conexion
  while (WiFi.status() != WL_CONNECTED) {     // Bucle para reintentar conexion hasta que sea exitosa
    delay(100);
    Serial.println("Intentando conexion a red Wi-Fi");    // Mensaje sobre proceso de intento conexion Wi-Fi
  }
  Serial.println("Conectado exitosamente a red Wi-Fi");   // Mensaje sobre conexion Wi-Fi exitosa
  Serial.println("La direccion IP asignada es: ");        // Muestra la IP asignada
  Serial.println(WiFi.localIP());
}

// Creacion de funcion para procesamiento de mensajes recibidos del broker

void callback(String topic, byte* message, unsigned int length) { // Funcion con tres argumentos necesarios
  Serial.println();                                               // topic, mensaje y longitud del mensaje
  Serial.print("Mensaje recibido del topic: ");
  Serial.println(topic);                                          // Muestra informacion sbore el topic mediante
  Serial.print("Mensaje: ");                                      // Mediante la interfaz serial
  String message_Estado;                                          // Variable tipo String para almacenar mensaje
  for (int i = 0; i < length; i++)                                // Ciclor for para recorrido de cada byte del mensaje
  {
    Serial.print((char)message[i]);                               // Muestra en N caracter del mensaje
    message_Estado += (char)message[i];                           // Almacena en la variable message_Estado, tl
  }                                                               // mensaje completo recibido
  Serial.println("");

  if (topic == "nodo1_1/t1/relay") {                    // Comparacion con el tema o topic recibido
    if (message_Estado == "On") {                       // Comparacion del mensaje que contiene el topic
      estado_relay_t1 = true;                           // Cambio del estado de variable del rele tomacorriente 1
      digitalWrite(pin_t1, LOW);                        // Activación del relé del tomacorriente 1
      Serial.println("Tomacorriente 1 activado");
      Serial.println("Recoleccion y envio de datos");
    } else if (message_Estado == "Off") {               // Comparacion del mensaje que contiene el topic
      estado_relay_t1 = false;                          // Cambio del estado de variable del rele tomacorriente 1
      digitalWrite(pin_t1, HIGH);                       // Desactivación del relé del tomacorriente 1
      Serial.println("Tomacorriente 1 desactivado");

      sensor_t1_calibrado = false;                      // Reinicio de estado de calibracion

    }
  } else if (topic == "nodo1_1/t2/relay") {             // Comparacion con el tema o topic recibido
    if (message_Estado == "On") {                       // Comparacion del mensaje que contiene el topic
      estado_relay_t2 = true;                           // Cambio del estado de variable del rele tomacorriente 2
      digitalWrite(pin_t2, LOW);                        // Activación del relé del tomacorriente 2
      Serial.println("Tomacorriente 2 activado");
      Serial.println("Recoleccion y envio de datos");
    } else if (message_Estado == "Off") {                // Comparacion del mensaje que contiene el topic
      estado_relay_t2 = false;                          // Cambio del estado de variable del rele tomacorriente 2
      digitalWrite(pin_t2, HIGH);                       // Desactivación del relé del tomacorriente 2
      Serial.println("Tomacorriente 2 desactivado");

      sensor_t2_calibrado = false;                      // Reinicio de estado de calibracion
    }
  } else if (topic == "nodo1_1/resetE/t1") {            // Comparacion con el tema o topic recibido
    if (message_Estado == "On" && !estado_resetE_t1) {  // Comparacion del mensaje que contiene el topic
                                                        // Si el mensajes es On y no existe solicitud de reset en curso
      reset_pzem1();                                    // Llama a la funcion de reseteo Energia para pzem004-t N°1
      estado_resetE_t1 = true;                          // Cambia el estado de la variable del estado del proceso de reseteo tomacorriente 1
      client.publish("nodo1_1/resetE/t1", "Off");       // Publica el mensaje Off, para confirmar reseteo y evitar bucles de reset
    } else if (message_Estado == "Off") {               // Comparacion del mensaje que contiene el topic
      estado_resetE_t1 = false;                         // Indica que no hay proceso de reseteo en curso
    }
  } else if (topic == "nodo1_1/resetE/t2") {            // Comparacion con el tema o topic recibido
    if (message_Estado == "On" && !estado_resetE_t2) {  // Comparacion del mensaje que contiene el topic
                                                        // Si el mensajes es On y no existe solicitud de reset en curso
      reset_pzem2();                                    // Llama a la funcion de reseteo Energia para pzem004-t N°2
      estado_resetE_t2 = true;                          // Cambia el estado de la variable del estado del proceso de reseteo tomacorriente 2
      client.publish("nodo1_1/resetE/t2", "Off");       // Publica el mensaje Off, para confirmar reseteo y evitar bucles de reset
    } else if (message_Estado == "Off") {               // Comparacion del mensaje que contiene el topic
      estado_resetE_t2 = false;                         // Indica que no hay proceso de reseteo en curso
    }
  }
}

void reconnect() {                                        // Funcion de reconexión con el broker
  // Reconexion a Broker MQTT
  while (!client.connected()) {                           // Inicio d bucle hasta que el cliente no este conectado
    Serial.println("Intentando conexion con el Broker ");
    if (client.connect("espClient1", "nodo1_1/estado", 1, true, "Off")) {                     // Condicional para intento de conexion con el broker
      Serial.println("Conectado exitosamente al Broker");   // si la conexión tiene éxito se realiza lo siguiente:
      client.publish("nodo1_1/estado", "On", true);         // Publica el estado On cundo se conecta exitosamente
      client.subscribe("nodo1_1/t1/relay");                 // Subscripción del cliente al topic "esp32/e1/relay"
      client.subscribe("nodo1_1/t2/relay");                 // Subscripción del cliente al topic "esp32/e2/relay"
      client.subscribe("nodo1_1/resetE/t1");                // Subscripción del cliente al topic "nodo1_1/resetE/t1"
      client.subscribe("nodo1_1/resetE/t2");                // Subscripción del cliente al topic "nodo1_1/resetE/t2"
    } else {                                                // Condicional si la conexón al broker falla
      Serial.print("Fallo conexion con el Broker: ");
      Serial.println(client.state());                       // Mensaje de intendo de nueva conexión al broker
      Serial.println("Intentandolo de nuevo");
      // Intentando en 5 segundos
      delay(5000);
    }
  }
}

void reset_pzem1() {
  if (pzem_t1.resetEnergy()) {                              // Verifica si se realiza exitosamente el reseteo de energia de sensore pzem004-t del tomacorriente 1
    Serial.println("Energía del sensor pzem004-t N°1 o tomacorriente 1 reseteada exitosamente");
    soli_resetE_t1 = true;                                  // Estado de variable para indicar que existe una solicitud de reseteo pendiente
    tiempo_resetE_t1 = millis();                            // Marca el tiempo desde que se inicia el reseteo
  } else {
    Serial.println("Fallo al resetear la energía del sensor pzem004-t N°1 o tomacorriente 1");
  }
}

void reset_pzem2() {
  if (pzem_t2.resetEnergy()) {                              // Verifica si se realiza exitosamente el reseteo de energia de sensore pzem004-t del tomacorriente 2
    Serial.println("Energía del sensor pzem004-t N°2 o tomacorriente 2 reseteada exitosamente");
    soli_resetE_t2 = true;                                  // Estado de variable para indicar que existe una solicitud de reseteo pendiente
    tiempo_resetE_t2 = millis();                            // Marca el tiempo desde que se inicia el reseteo
  } else {
    Serial.println("Fallo al resetear la energía del sensor pzem004-t N°2 o tomacorriente 2");
  }
}


// LECTURAS DEL PZEM004T PARA EL ENCHUFE 1

void lecturas_pub_n1_1_t1() {              // Funcion para lectura y publicacion de los datos obtenidos por los sensores

  unsigned long t_actual_t1 = millis();                           // Variable que contiene el tiempo transcurrido desde inicio del ESP32
  if ( (t_actual_t1 - tanterior_cali_t1) >= intervalo_calibrar) { // Condicional que verifica si transcurrio 5 seg. desde la ultima calibracion
    if (!sensor_t1_calibrado) {                                   // Condicional si el sensor aún no ha sido calibrado se calibra
      delay(5000);                                                // Espera 5 segundos en calibrar el sensor
      sensor_t1_calibrado = true;                                 // Marca el sensor como calibrado después de esperar 5 segundos
      return;                                                     // Salir del condicional sin enviar datos antes de calibracion
    }
    tanterior_cali_t1 = t_actual_t1;                              // Actualizacion del nuevo tiempo de la ultima calibracion
  }

  if (soli_resetE_t1 && (millis() - tiempo_resetE_t1 >= 8000)) {   // Compara si hay una solicitud de reseteo y si se cumple un periodo de 8 segundos inicio de reset
    soli_resetE_t1 = false;                                        // Indica que el reseteo termino y finaliza la solicitud de reset
    sensor_t1_calibrado = false;                                   // Designa al sensor como no calibrado para volver a calibrar
  }

  if (!soli_resetE_t1) {                                           // Verifica si no existe solicitud de reseteo de energia para pzem004-t N°1 o tomacorriente 1
    // Si es no existe solicitud, ejecuta proceso de lectura y envio de datos

    // Condicionales para lectura de datos pzem-004t luego de ser calibrado

    unsigned long tiempo_actual_t1 = millis();                      // Variable que contiene el tiempo transcurrido desde inicio del ESP32

    if ( (tiempo_actual_t1 - tiempo_anterior_t1) >= intervalo) {    // Condicional que verifica si transcurrio 10 seg. desde la ultima lectura
      // Si se cumple el transcurso de 10 seg. se realizan las sig. acciones
      voltaje_t1 = pzem_t1.voltage();                               // Se realizan las lecturas de los valores de voltaje, corriente
      corriente_t1 = pzem_t1.current();                             // potencia y energía o consumo eléctrico y posterior a ello se
      potencia_t1 = pzem_t1.power();                                // almacenan en las variables designadas a cada una
      consumo_t1 = pzem_t1.energy();

      Serial.println("Mediciones del pzem00t4 N°1 o tomacorriente 1");    // Presentacion de los valores obtenidos
      Serial.println("Valor de voltaje: ");
      Serial.print(voltaje_t1, 4);
      Serial.println(" V");
      dtostrf(voltaje_t1, 10, 4, pVoltaje_t1);                        // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo1_1/t1/voltaje", pVoltaje_t1);              // Publicación de voltaje al topic "nodo1_1/t1/voltaje"

      Serial.println("Valor de corriente: ");                         // Presentacion de los valores obtenidos
      Serial.print(corriente_t1, 4);
      Serial.println(" A");
      dtostrf(corriente_t1, 10, 4, pCorriente_t1);                    // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo1_1/t1/corriente", pCorriente_t1);          // Publicación de corriente al topic "nodo1_1/t1/corriente"

      Serial.println("Valor de potencia: ");                          // Presentacion de los valores obtenidos
      Serial.print(potencia_t1, 4);
      Serial.println(" W");
      dtostrf(potencia_t1, 10, 4, pPotencia_t1);                      // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo1_1/t1/potencia", pPotencia_t1);            // Publicación de potencia al topic "nodo1_1/t1/potencia"

      Serial.println("Valor de consumo: ");                           // Presentacion de los valores obtenidos
      Serial.print(consumo_t1, 4);
      Serial.println(" kWh");
      dtostrf(consumo_t1, 10, 4, pConsumo_t1);                        // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo1_1/t1/consumo", pConsumo_t1);              // Publicación de consumo electrico al topic "nodo1_1/t1/consumo"

      tiempo_anterior_t1 = tiempo_actual_t1;                          // Actualiza el último tiempo de envío
    }
  }
}

// LECTURAS DEL PZEM004T PARA EL ENCHUFE 2
void lecturas_pub_n1_1_t2() {              // Funcion para lectura y publicacion de los datos obtenidos por los sensores

  unsigned long t_actual_t2 = millis();                           // Variable que contiene el tiempo transcurrido desde inicio del ESP32
  if ( (t_actual_t2 - tanterior_cali_t2) >= intervalo_calibrar) { // Condicional que verifica si transcurrio 5 seg. desde la ultima calibracion
    if (!sensor_t2_calibrado) {                                   // Condicional si el sensor aún no ha sido calibrado se calibra
      delay(5000);                                                // Espera 5 segundos en calibrar el sensor
      sensor_t2_calibrado = true;                                 // Marca el sensor como calibrado después de esperar 8 segundos
      return;                                                     // Salir de la función sin enviar datos en la primera llamada
    }
    tanterior_cali_t2 = t_actual_t2;                              // Actualizacion del nuevo tiempo de la ultima calibracion
  }

  if (soli_resetE_t2 && (millis() - tiempo_resetE_t2 >= 8000)) {   // Compara si hay una solicitud de reseteo y si se cumple un periodo de 8 segundos inicio de reset
    soli_resetE_t2 = false;                                        // Indica que el reseteo termino y finaliza la solicitud de reset
    sensor_t2_calibrado = false;                                   // Designa al sensor como no calibrado para volver a calibrar
  }

  if (!soli_resetE_t2) {
    // Si es no existe solicitud, ejecuta proceso de lectura y envio de datos

    // Condicionales para lectura de datos pzem-004t luego de ser calibrado
    
    unsigned long tiempo_actual_t2 = millis();                    // Variable que contiene el tiempo transcurrido desde inicio del ESP32

    if ( (tiempo_actual_t2 - tiempo_anterior_t2) >= intervalo) {  // Condicional que verifica si transcurrio 10 seg. desde la ultima lectura
                                                                  // Si se cumple el transcurso de 10 seg. se realizan las sig. acciones
      voltaje_t2 = pzem_t2.voltage();                             // Se realizan las lecturas de los valores de voltaje, corriente        
      corriente_t2 = pzem_t2.current();                           // potencia y energía o consumo eléctrico y posterior a ello se 
      potencia_t2 = pzem_t2.power();                              // almacenan en las variables designadas a cada una
      consumo_t2 = pzem_t2.energy();

      Serial.println("Mediciones del pzem00t4 N°2 o tomacorriente 2");  // Presentacion de los valores obtenidos
      Serial.println("Valor de voltaje: ");
      Serial.print(voltaje_t2, 4);
      Serial.println(" V");
      dtostrf(voltaje_t2, 10, 4, pVoltaje_t2);                          // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo1_1/t2/voltaje", pVoltaje_t2);                // Publicación de voltaje al topic "nodo1_1/t1/voltaje" 

      Serial.println("Valor de corriente: ");                           // Presentacion de los valores obtenidos
      Serial.print(corriente_t2, 4);
      Serial.println(" A");
      dtostrf(corriente_t2, 10, 4, pCorriente_t2);                      // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo1_1/t2/corriente", pCorriente_t2);            // Publicación de voltaje al topic "nodo1_1/t2/corriente"

      Serial.println("Valor de potencia: ");                            // Presentacion de los valores obtenidos
      Serial.print(potencia_t2, 4);
      Serial.println(" W");
      dtostrf(potencia_t2, 10, 4, pPotencia_t2);                        // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo1_1/t2/potencia", pPotencia_t2);              // Publicación de voltaje al topic "nodo1_1/t2/potencia"

      Serial.println("Valor de consumo: ");                             // Presentacion de los valores obtenidos
      Serial.print(consumo_t2, 4);  
      Serial.println(" kWh");
      dtostrf(consumo_t2, 10, 4, pConsumo_t2);                          // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo1_1/t2/consumo", pConsumo_t2);                // Publicación de voltaje al topic "nodo1_1/t2/consumo"

      tiempo_anterior_t2 = tiempo_actual_t2;                            // Actualizar el último tiempo de envío

    }
  }
}


void setup() {

  Serial.begin(115200);                   // Inicio de comunicacion serial con taza de 115200 baudios
  setup_wifi();                           // Llama la funcion de conexion a red Wi-Fi
  client.setServer(mqtt_server, 1883);    // Declara al servidor MQTT y el puerto para la comunicacion MQTT
  client.setCallback(callback);           // Llama a la funcion de callback (subscricion a temas)
  pinMode(pin_t1, OUTPUT);                // Establece al pin de salida para el relé-tomacorriente 1
  digitalWrite(pin_t1, HIGH);             // Establece el estado del pin en alto

  pinMode(pin_t2, OUTPUT);                // Establece al pin de salida para el relé-tomacorriente 2
  digitalWrite(pin_t2, HIGH);             // Establece el estado del pin en alto

  
  client.publish("nodo1_1/estado","On",true); // Envio de estado general del Nodo1_1

}

void loop() {
  
  unsigned long t_actual_estadoN = millis(); // Variable que contiene el tiempo transcurrido desde inicio del ESP32
  if(t_actual_estadoN - tiempoU_estadoNodo >= intervalo_estadoNodo){ // Condicional que verifica si transcurrio 30 seg. desde la ultimo envio de estado
    tiempoU_estadoNodo = t_actual_estadoN;                           // Actualizar el último tiempo de envío
    client.publish("nodo1_1/estadoPeriodo","On",true);               // Envio del estado del Nodo periodicamente 
  }

  if (estado_relay_t1) {                  // Verifica si el relé del tomacorriente 1 esta activado
    lecturas_pub_n1_1_t1();               // Ejecuta la funcion lecturas_pub_n1_1_t1()
  }

  if (estado_relay_t2) {                  // Verifica si el relé del tomacorriente 2 esta activado
    lecturas_pub_n1_1_t2();               // Ejecuta la funcion lecturas_pub_n1_1_t2()
  }

  if (!client.connected()) {              // Verifica si el cliente no está conectado a broker MQTT
    reconnect();                          // Ejecuta la funcion de reconexion al broker
  }
  if (!client.loop())                     // Verifica si la conexion MQTT se mantiene activa
    client.connect("espClient1");         // Si la condición es falsa, se ejecuta el proceso
                                          // de reconexion con el broker mqtt
}
