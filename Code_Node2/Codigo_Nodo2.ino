#include <WiFi.h>                   // Libreria para conexión del ESP32 al Wi-FI                 
#include <PubSubClient.h>           // Libreria para que el microcontrolador 
// sea un cliente MQTT
#include <PZEM004Tv30.h>            // Libreria para funcionamiento y comunicación 
// del sensor pzem004-t y esp32


// Credenciales para conexion a red Wi-Fi

const char* ssid = "XXXXXXX";    // SSID de red Wi-FI a conectarse
const char* password = "XXXXXXX";       // Contrasea de red Wi-FI a conectarse

const char* mqtt_server = "X.X.X.X";  // Direccion IP Broker

WiFiClient espClient3;                      // Creacion de cliente MQTT
PubSubClient client(espClient3);


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

PZEM004Tv30 pzem_p1(serial_pzem1, pin_rx_pzem1, pin_tx_pzem1);
PZEM004Tv30 pzem_p2(serial_pzem2, pin_rx_pzem2, pin_tx_pzem2);

// Variable booleana del estado del medidorinteligente
bool estado_smart_meter = false;

// Definición de variales para reles

#define pin_ssr1 19         // Variables para pines de señal de control SSR1
#define pin_ssr2 18         // Variables para pines de señal de control SSR2
#define pin_ssr3 5          // Variables para pines de señal de control SSR3
#define pin_ssr4 17         // Variables para pines de señal de control SSR4

///// VARIABLES DEL SENSOR PZEM004T N°1 /////
// Creacion varibles electricas
float voltaje_p1;           // Variable de lectura del voltaje
float corriente_p1;         // Variable de lectura de corriente
float potencia_p1;          // Variable de lectura de potencia
float consumo_p1;           // Variable de lectura de energía o consumo

// Varibles a publicar tipo String al Broker
char pVoltaje_p1[20];       // Variable de publicacion MQTT del voltaje
char pCorriente_p1[20];     // Variable de publicacion MQTT de corriente
char pPotencia_p1[20];      // Variable de publicacion MQTT de potencia
char pConsumo_p1[20];       // Variable de publicacion MQTT de energía o consumo

///// VARIABLES DEL SENSOR PZEM004T N°2 /////
// Creacion varibles electricas
float voltaje_p2;           // Variable de lectura del voltaje
float corriente_p2;         // Variable de lectura de corriente
float potencia_p2;          // Variable de lectura de potencia
float consumo_p2;           // Variable de lectura de energía o consump

// Varibles a publicar tipo String al Broker
char pVoltaje_p2[20];       // Variable de publicacion MQTT del voltaje
char pCorriente_p2[20];     // Variable de publicacion MQTT de corriente
char pPotencia_p2[20];      // Variable de publicacion MQTT de potencia
char pConsumo_p2[20];       // Variable de publicacion MQTT de energía o consumo

//// Variable de Consumo Electrico Total ////
float consumoE_total;       // Variable que almacena el consumo electrico total (suma fase1 y fase 2)
char pConsumoE_total[20];   // Variable de publicacion MQTT del consumo electrico total

// Variables de tiempo para control en la suma del consumo electrico total
unsigned tiempo_actual_sumace = 0;
unsigned tiempo_anterior_sumace = 0;

// Varibles booleandas para control de actualizacion de valores para suma de consumo electrico total
bool actualiza_cE_p1 = false;
bool actualiza_cE_p2 = false;

// Declaracion variables: tiempo de ultima lectura de sensores 1 Y 2

unsigned long tiempo_anterior_p1 = 0;
unsigned long tiempo_anterior_p2 = 0;

const unsigned long intervalo = 10000;        // Intervalo de tiempo para lectura de datos

// Estados de calibracion de sensores
static bool sensor_p1_calibrado = false;      // Estado de caliracion de pzem004t n° 1
static bool sensor_p2_calibrado = false;      // Estado de caliracion de pzem004t n° 2

// Declaracion variables: tiempo de ultima calibracion para sensor-enchufe 1 y 2
unsigned long tanterior_cali_p1 = 0;          // Tiempo última calibreacion pzem004t n° 1
unsigned long tanterior_cali_p2 = 0;          // tiempo última calibreacion pzem004t n° 2

const unsigned long intervalo_calibrar = 5000; // Intervalo de tiempo para calibreacion de sensores

// Variables para el Control del Reseteo de los Dos Sensores Pzem004-t

bool soli_resetE = false;                     // Variable para solicitud de reseteo de energia de los dos sensores pzem004-t

unsigned long tiempo_resetE = 0;              // Variable para el tiempo transcurrido desde inicio de reseteo de Energia de los dos sensores pzem004-t

// Variables para evitar bucle en reseteo de los sensores pzem004-t

bool estado_resetE = false;                   // Variable almacena estado del proceso de reseteo de energia de los dos sensores pzem004-t

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
    message_Estado += (char)message[i];                           // Almacena en la variable message_Estado, el
  }                                                               // mensaje completo recibido
  Serial.println("");

  if (topic == "nodo2/c1/ssr1") {                                 // Comparacion con el tema o topic recibido
    if (message_Estado == "On") {                                 // Comparacion del mensaje que contiene el topic
      digitalWrite(pin_ssr1, HIGH);                               // Cambio de estado del SSR1
      Serial.println("Circuito 1  activado");
    } else if (message_Estado == "Off") {                         // Comparacion del mensaje que contiene el topic
      digitalWrite(pin_ssr1, LOW);                                // Cambio de estado del SSR1
      Serial.println("Circuito 1 desactivado");
    }
  } else if (topic == "nodo2/c2/ssr2") {
    if (message_Estado == "On") {                                 // Comparacion del mensaje que contiene el topic
      digitalWrite(pin_ssr2, HIGH);                               // Cambio de estado del SSR2
      Serial.println("Circuito 2 activado");
    } else if (message_Estado == "Off") {                         // Comparacion del mensaje que contiene el topic
      digitalWrite(pin_ssr2, LOW);                                // Cambio de estado del SSR2
      Serial.println("Circuito 2 desactivado");
    }
  } else if (topic == "nodo2/c3/ssr3") {
    if (message_Estado == "On") {                                 // Comparacion del mensaje que contiene el topic
      digitalWrite(pin_ssr3, HIGH);                               // Cambio de estado del SSR3
      Serial.println("Circuito 3 activado");
    } else if (message_Estado == "Off") {                         // Comparacion del mensaje que contiene el topic
      digitalWrite(pin_ssr3, LOW);                                // Cambio de estado del SSR3
      Serial.println("Circuito 3 desactivado");
    }
  } else if (topic == "nodo2/c4/ssr4") {
    if (message_Estado == "On") {                                 // Comparacion del mensaje que contiene el topic
      digitalWrite(pin_ssr4, HIGH);                               // Cambio de estado del SSR4
      Serial.println("Circuito 4 activado");
    } else if (message_Estado == "Off") {                         // Comparacion del mensaje que contiene el topic
      digitalWrite(pin_ssr4, LOW);                                // Cambio de estado del SSR4
      Serial.println("Circuito 4 desactivado");
    }
  } else if (topic == "nodo2/resetE") {                           // Comparacion con el tema o topic recibido
    if (message_Estado == "On" && !estado_resetE) {               // Comparacion del mensaje que contiene el topic
      // Si el mensajes es On y no existe solicitud de reset en curso
      reset_pzem();                                               // Llama a la funcion de reseteo Energia para los dos sensores pzem004-t
      estado_resetE = true;                                       // Cambia el estado de la variable del estado del proceso de reseteo
      client.publish("nodo2/resetE", "Off");                      // Publica el mensaje Off, para confirmar reseteo y evitar bucles de reset
    } else if (message_Estado == "Off") {                         // Comparacion del mensaje que contiene el topic
      estado_resetE = false;                                      // Indica que no hay proceso de reseteo en curso
    }
  }
}

void reset_pzem() {
  if (pzem_p1.resetEnergy() && pzem_p2.resetEnergy()) {           // Verifica si se realiza exitosamente el reseteo de energia de los dos sensores pzem004-t
    Serial.println("Energía de los sensores pzem004-t N°1 y N°2 reseteada exitosamente");
    soli_resetE = true;                                           // Estado de variable para indicar que existe una solicitud de reseteo pendiente
    tiempo_resetE = millis();                                     // Marca el tiempo desde que se inicia el reseteo
    consumoE_total = 0;                                           // Vuelve a cero la variable de consumo electrico total

    consumo_p1 = 0;                                               // Vuelve a cero la variable de consumo pzem1                                          
    consumo_p2 = 0;                                               // Vuelve a cero la variable de consumo pzem2

    dtostrf(consumoE_total, 10, 4, pConsumoE_total);                // Convierte a valor recolectado en una cadena de caracteres
    client.publish("nodo2/consumoe_total", pConsumoE_total);        // Publicación de energia al topic "nodo2/consumoe_total" con valor 0 
    Serial.println("Consumo Electrico Total Reseteado a 0 kWh");
  } else {
    Serial.println("Fallo al resetear la energía de los sensores pzem004-t N°1 y N°2");
  }
}


void reconnect() {                                        // Funcion de reconexión con el broker
  // Reconexion a Broker MQTT
  while (!client.connected()) {                           // Inicio del bucle hasta que el cliente este conectado
    Serial.println("Intentando conexion con el Broker ");
    if (client.connect("espClient3", "nodo2/estado", 1, true, "Off")) {                   // Condicional para intento de conexion con el broker
      Serial.println("Conectado exitosamente al Broker");// si la conexión tiene éxito se realiza lo siguiente:
      client.publish("nodo2/estado", "On", true);         // Publica el estado On cundo se conecta exitosamente
      client.subscribe("nodo2/c1/ssr1");                  // Subscripción del cliente al topic "nodo2/c1/ssr1"
      client.subscribe("nodo2/c2/ssr2");                  // Subscripción del cliente al topic "nodo2/c2/ssr2"
      client.subscribe("nodo2/c3/ssr3");                  // Subscripción del cliente al topic "nodo2/c3/ssr3"
      client.subscribe("nodo2/c4/ssr4");                  // Subscripción del cliente al topic "nodo2/c4/ssr4"
      client.subscribe("nodo2/resetE");                   // Subscripción del cliente al topic "nodo2/resetE""
    } else {                                              // Condicional si la conexón al broker falla
      Serial.print("Fallo conexion con el Broker: ");
      Serial.println(client.state());
    }                                                     // Mensaje de intendo de nueva conexión al broker
    Serial.println("Intentandolo de nuevo");
    // Intentando en 5 segundos
    delay(5000);
  }
}


// LECTURAS DEL PZEM004T N°1

void lecturas_pub_nodo2_p1() {              // Funcion para lectura y publicacion de los datos obtenidos por los sensores

  unsigned long t_actual_p1 = millis();                           // Variable que contiene el tiempo transcurrido desde inicio del ESP32
  if ( (t_actual_p1 - tanterior_cali_p1) >= intervalo_calibrar) { // Condicional que verifica si transcurrio 5 seg. desde la ultima calibracion
    if (!sensor_p1_calibrado) {                                   // Condicional si el sensor aún no ha sido calibrado se calibra
      delay(5000);                                                // Espera 5 segundos en calibrar el sensor
      sensor_p1_calibrado = true;                                 // Marca el sensor como calibrado después de esperar 5 segundos
      return;                                                     // Salir del condicional sin enviar datos antes de calibracion
    }
    tanterior_cali_p1 = t_actual_p1;                              // Actualizacion del nuevo tiempo de la ultima calibracion
  }

  if (soli_resetE && (millis() - tiempo_resetE >= 8000)) {        // Compara si hay una solicitud de reseteo y si se cumple un periodo de 8 segundos inicio de reset
    soli_resetE = false;                                          // Indica que el reseteo termino y finaliza la solicitud de reset
    sensor_p1_calibrado = false;                                  // Designa al sensor pzem1 como no calibrado para volver a calibrar
    sensor_p2_calibrado = false;                                  // Designa al sensor pzem2 como no calibrado para volver a calibrar
  }

  if (!soli_resetE) {                                             // Verifica si no existe solicitud de reseteo de energia para pzem004-t N°1

    // Condicionales para lectura de datos pzem-004t luego de ser calibrado

    unsigned long tiempo_actual_p1 = millis();                   // Variable que contiene el tiempo transcurrido desde inicio del ESP32

    if ( (tiempo_actual_p1 - tiempo_anterior_p1) >= intervalo) { // Condicional que verifica si transcurrio 10 seg. desde la ultima lectura
      // Si se cumple el transcurso de 10 seg. se realizan las sig. acciones
      voltaje_p1 = pzem_p1.voltage();                           // Se realizan las lecturas de los valores de voltaje, corriente
      corriente_p1 = pzem_p1.current();                         // potencia y energía o consumo eléctrico y posterior a ello se
      potencia_p1 = pzem_p1.power();                            // almacenan en las variables designadas a cada una
      consumo_p1 = pzem_p1.energy();

      Serial.println("Mediciones del pzem00t4 N°1");           // Presentacion de los valores obtenidos
      Serial.println("Valor de voltaje: ");
      Serial.print(voltaje_p1, 4);
      Serial.println(" V");
      dtostrf(voltaje_p1, 10, 4, pVoltaje_p1);                 // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo2/p1/voltaje", pVoltaje_p1);         // Publicación de voltaje al topic "nodo2/p1/voltaje"

      Serial.println("Valor de corriente: ");                  // Presentacion de los valores obtenidos
      Serial.print(corriente_p1, 4);
      Serial.println(" A");
      dtostrf(corriente_p1, 10, 4, pCorriente_p1);             // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo2/p1/corriente", pCorriente_p1);     // Publicación de corriente al topic "nodo2/p1/corriente"

      Serial.println("Valor de potencia: ");                   // Presentacion de los valores obtenidos
      Serial.print(potencia_p1, 4);
      Serial.println(" W");
      dtostrf(potencia_p1, 10, 4, pPotencia_p1);               // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo2/p1/potencia", pPotencia_p1);       // Publicación de potencia al topic "nodo2/p1/potencia"

      Serial.println("Valor de consumo: ");                    // Presentacion de los valores obtenidos
      Serial.print(consumo_p1, 4);
      Serial.println(" kWh");
      dtostrf(consumo_p1, 10, 4, pConsumo_p1);                 // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo2/p1/consumo", pConsumo_p1);         // Publicación de energia al topic "nodo2/p1/consumo"

      tiempo_anterior_p1 = tiempo_actual_p1;                   // Actualiza el último tiempo de envío
      actualiza_cE_p1 = true;                                  // Confirma actualizacion de valor
    }
  }
}

// LECTURAS DEL PZEM004T N°2
void lecturas_pub_nodo2_p2() {

  unsigned long t_actual_p2 = millis();                           // Variable que contiene el tiempo transcurrido desde inicio del ESP32
  if ( (t_actual_p2 - tanterior_cali_p2) >= intervalo_calibrar) { // Condicional que verifica si transcurrio 5 seg. desde la ultima calibracion
    if (!sensor_p2_calibrado) {                                   // Condicional si el sensor aún no ha sido calibrado se calibra
      delay(5000);                                                // Espera 5 segundos para calibrar el sensor
      sensor_p2_calibrado = true;                                 // Marca el sensor como calibrado después de esperar 5 segundos
      return;                                                     // Salir del condicional sin enviar datos antes de calibracion
    }
    tanterior_cali_p2 = t_actual_p2;                              // Actualizacion del nuevo tiempo de la ultima calibracion
  }

  if (soli_resetE && (millis() - tiempo_resetE >= 8000)) {        // Compara si hay una solicitud de reseteo y si se cumple un periodo de 8 segundos inicio de reset
    soli_resetE = false;                                          // Indica que el reseteo termino y finaliza la solicitud de reset
    sensor_p1_calibrado = false;                                  // Designa al sensor pzem1 como no calibrado para volver a calibrar
    sensor_p2_calibrado = false;                                  // Designa al sensor pzem1 como no calibrado para volver a calibrar
  }

  if (!soli_resetE) {                                             // Verifica si no existe solicitud de reseteo de energia para pzem004-t N°2

    unsigned long tiempo_actual_p2 = millis();                    // Variable que contiene el tiempo transcurrido desde inicio del ESP32

    if ( (tiempo_actual_p2 - tiempo_anterior_p2) >= intervalo) {  // Condicional que verifica si transcurrio 10 seg. desde la ultima lectura

      voltaje_p2 = pzem_p2.voltage();                             // Se realizan las lecturas de los valores de voltaje, corriente
      corriente_p2 = pzem_p2.current();                           // potencia y energía o consumo eléctrico y posterior a ello se
      potencia_p2 = pzem_p2.power();                              // almacenan en las variables designadas a cada una
      consumo_p2 = pzem_p2.energy();

      Serial.println("Mediciones del pzem00t4 N°2");              // Presentacion de los valores obtenidos
      Serial.println("Valor de voltaje: ");
      Serial.print(voltaje_p2, 4);
      Serial.println(" V");
      dtostrf(voltaje_p2, 10, 4, pVoltaje_p2);                    // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo2/p2/voltaje", pVoltaje_p2);            // Publicación de energia al topic "nodo2/p1/voltaje"

      Serial.println("Valor de corriente: ");                     // Presentacion de los valores obtenidos
      Serial.print(corriente_p2, 4);
      Serial.println(" A");
      dtostrf(corriente_p2, 10, 4, pCorriente_p2);                // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo2/p2/corriente", pCorriente_p2);        // Publicación de energia al topic "nodo2/p1/corriente"

      Serial.println("Valor de potencia: ");                      // Presentacion de los valores obtenidos
      Serial.print(potencia_p2, 4);
      Serial.println(" W");
      dtostrf(potencia_p2, 10, 4, pPotencia_p2);                  // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo2/p2/potencia", pPotencia_p2);          // Publicación de energia al topic "nodo2/p1/potencia"

      Serial.println("Valor de consumo: ");                       // Presentacion de los valores obtenidos
      Serial.print(consumo_p2, 4);
      Serial.println(" kWh");
      dtostrf(consumo_p2, 10, 4, pConsumo_p2);                    // Convierte a valor recolectado en una cadena de caracteres
      client.publish("nodo2/p2/consumo", pConsumo_p2);            // Publicación de energia al topic "nodo2/p1/consumo"

      tiempo_anterior_p2 = tiempo_actual_p2;                      // Actualizar el último tiempo de envío
      actualiza_cE_p2 = true;                                     // Confirma actualizacion de valor

    }
  }
}

void calculo_consumoE_total() {                                   // Funcion calcula el consumo electrito total (fase 1 + fase 2)
  unsigned long tiempo_actual_sumace = millis();
  if ((tiempo_actual_sumace - tiempo_anterior_sumace) >= intervalo && actualiza_cE_p2 && actualiza_cE_p2) { 
                                                                    // Verifica el tiempo de espera de 10 segundos                                         
                                                                    // y asegura consumototal sea 0 hasta registro nuevos valores
    if (soli_resetE && (millis() - tiempo_resetE < 8000)){           // Verifica el tiempo de espera de 8 segundos 
      consumoE_total = 0;                                                                                                              
    }else{
      consumoE_total = consumo_p1 + consumo_p2;
    }
    // consumoE_total = consumo_p1 + consumo_p2 ;                      // Variable que almacena el consumo electrito total
    dtostrf(consumoE_total, 10, 4, pConsumoE_total);                // Convierte a valor recolectado en una cadena de caracteres
    client.publish("nodo2/consumoe_total", pConsumoE_total);        // Publicación de energia al topic "nodo2/consumoe_total"

    Serial.println("Valor de consumo electrico total: ");           // Presentacion de los valores obtenidos
    Serial.print(consumoE_total, 4);
    Serial.println(" kWh");

    actualiza_cE_p1 = false;                                        // Confirma actualizacion de valor
    actualiza_cE_p2 = false;                                        // Confirma actualizacion de valor
    tiempo_anterior_sumace = tiempo_actual_sumace;                  // Actualiz ultimo tiempo para la sumatoria de consumo por sensores
  }
}

void setup() {

  Serial.begin(115200);                             // Inicio de comunicacion serial con taza de 115200 baudios
  setup_wifi();                                     // Llama la funcion de conexion a red Wi-Fi
  client.setServer(mqtt_server, 1883);              // Declara al servidor MQTT y el puerto para la comunicacion MQTT
  client.setCallback(callback);                     // Llama a la funcion de callback (subscricion a temas)
  pinMode(pin_ssr1, OUTPUT);                        // Establece al pin de salida para el SSR1
  pinMode(pin_ssr2, OUTPUT);                        // Establece al pin de salida para el SSR2
  pinMode(pin_ssr3, OUTPUT);                        // Establece al pin de salida para el SSR3
  pinMode(pin_ssr4, OUTPUT);                        // Establece al pin de salida para el SSR4

  // Establecer todos los Relés de Estado Sólido Encendos
  digitalWrite(pin_ssr1, HIGH);
  digitalWrite(pin_ssr2, HIGH);
  digitalWrite(pin_ssr3, HIGH);
  digitalWrite(pin_ssr4, HIGH);

  client.publish("nodo2/estado","On",true); // Envio de estado general del Nodo2
}

void loop() {

  unsigned long t_actual_estadoN = millis(); // Variable que contiene el tiempo transcurrido desde inicio del ESP32
  if(t_actual_estadoN - tiempoU_estadoNodo >= intervalo_estadoNodo){ // Condicional que verifica si transcurrio 30 seg. desde la ultimo envio de estado
    tiempoU_estadoNodo = t_actual_estadoN;                           // Actualizar el último tiempo de envío
    client.publish("nodo2/estadoPeriodo","On",true);               // Envio del estado del Nodo periodicamente 
  }
  

  if (!client.connected()) {                        // Verifica si el cliente no está conectado a broker MQTT
    reconnect();                                    // Ejecuta la funcion de reconexion al broker
  }

  client.loop();
  lecturas_pub_nodo2_p1();                          // Si el cliente MQTT está conectado, llama a las funciones de lectura y publicación
  lecturas_pub_nodo2_p2();
  calculo_consumoE_total();
}
