/*
  Este programa pretende medir los tiempos de reacción y cuantificar las mejorías en los entrenamientos de un atleta.
  Para ello utilizaremos una Placa de desarrollo NodeMCU ESP8266 y Módulo Acelerómetro CJMCU ADXL345
  Un Buzzer que hará las veces de pistoletazo de salida
  y todo controlado a través de un enlace WIFI y a unservidor Web
*/

#include <SparkFun_ADXL345.h> // Esta biblioteca le permite comunicar con el acelerómetro ADXL345
#include <ESP8266WebServer.h> // Biblioteca se utiliza para simplificar la creación de un servidor web.

ADXL345 adxl = ADXL345();
unsigned long pto1 = 0;

unsigned long resta_pto = 0;
int desviacion = 0;
unsigned long timer0 = 0;
unsigned long timer1 = 0;
unsigned long timer2 = 0;
unsigned long tiempo_desde_disparo = 0;
int sensibilidad = 35;
unsigned long resultado = 0;
String sensibilidadSTR = "<h2 style='color:orange'>Sensibilidad: MEDIA</h2>";
String color = "yellow";
int PinBUZZER = 15;                              //Definimos el pin de salida del Buzzer - GPIO15 / D8

const char ssid[] = "Club-Atletismo-Leganes";   //Definimos la SSDI de nuestro servidor WiFi -nombre de red-
const char password[] = "complejoeuropa";       //Definimos la contraseña de nuestro servidor
WiFiServer server(80);                          //TCPservidor en el puerto 80

// -------------------------------------------------------------------------------------------------------------
void setup() {

  //Serial.begin(115200);
  //Serial.println("Iniciar");
  //Serial.println();

  adxl.powerOn();                               // Power on the ADXL345
  adxl.setRangeSetting(2);                      // Definir el rango del Acelerómetro, valor en 2g

  pinMode(PinBUZZER, OUTPUT);                   // Inicializamos el GPIO2 como salida (Buzzer)
  digitalWrite(PinBUZZER, LOW);                 // Dejamos inicialmente el GPIO2 apagado

  server.begin();                               // Inicializamos el servidor
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);                  // Red con clave, en el canal 1 y visible

  //Serial.println();
  //Serial.print("Direccion IP Access Point - por defecto: ");      //Imprime la dirección IP
  //Serial.println(WiFi.softAPIP());
  //Serial.print("Direccion MAC Access Point: ");                   //Imprime la dirección MAC
  //Serial.println(WiFi.softAPmacAddress());

}

void beep() {
  digitalWrite(PinBUZZER, HIGH);
  delay(50);
  digitalWrite(PinBUZZER, LOW);
}
int medX() {                                    // Función que devuelve una muestra pònderada de 20 medidas del eje X

  int x, y, z;
  int mX = 0;

  for (int i = 0; i < 20; i++) {
    adxl.readAccel(&x, &y, &z);
    mX = mX + x;
  }
  mX = mX / 20;
  return mX;
}

// -------------------------------------------------------------------------------------------------------------
void loop()
{
  WiFiClient client = server.available();               // Comprueba si el cliente ha conectado
  if (!client) {
    return;
  }

  //Serial.println("nuevo cliente");                      // Espera hasta que el cliente envía alguna petición

  while (!client.available()) {
    delay(1);
  }

  //Serial.printf("Clientes conectados al Access Point: %dn", WiFi.softAPgetStationNum());   // Imprime el número de clientes conectados

  String peticion = client.readStringUntil('r');          // Lee la petición
  //Serial.println(peticion);
  client.flush();

  int x, y, z, x1;
  String salida = "<h2>- - - -</h2>";


  if (peticion.indexOf('/START=L') == 11) {               // Comprueba la petición de sensibilidad
    sensibilidad = 50;
    sensibilidadSTR = "<h2 style='color:green'>Sensibilidad: BAJA</h2>";
    beep();
  }
  if (peticion.indexOf('/START=M') == 11) {
    sensibilidad = 35;
    sensibilidadSTR = "<h2 style='color:orange'>Sensibilidad: MEDIA</h2>";
    beep();
  }
  if (peticion.indexOf('/START=H') == 11) {
    sensibilidad = 20;
    sensibilidadSTR = "<h2 style='color:red'>Sensibilidad: ALTA</h2>";
    beep();
  }



  //--------------------------------------------------------------------------------------------------------------------------------------------------------------






  if (peticion.indexOf('/START=O') != -1) {                // Comprueba la petición de DISPARO


    x = medX();
    for (int i = 0; i < 40; i++) {
      x1 = medX();

      if ((x1 - x) > sensibilidad || (x1 - x) < -sensibilidad) {  //  Accelearación +-50(Sensibilidad Media) en el EJE de las X  ******
        salida = "<h2 style='color:red'>* NULO - PRE-DISPARO *</h2>";
        for (int i = 0; i < 10; i++) {
          beep();
          delay(50);
          color = "red";
          resultado = 0;
        }
        goto salto;
      }
    }


    x = medX();
    digitalWrite(PinBUZZER, HIGH);                         //  Activamos el BUZZER ( DISPARO !!! )
    timer1 = millis();


    while (true)                                           //Realizar este bucle mientras NO tengamos(una mínima) Accelearación en el EJE de las X
    {
      //pto1=millis();
      x1 = medX(); //  30ms  demoras en subrutina
      // Al no saber el pto exacto de inicio de la aceleración dentro de la subrrutina medX()lo colocaremos en la mitad (30ms) dando un margen de error de +-15ms


      if ((x1 - x) > sensibilidad || (x1 - x) < -sensibilidad) {  //  Accelearación +-50(Sensibilidad Media) en el EJE de las X  ******
        timer2 = millis();
        //resta_pto = timer2-pto1;

        resultado = timer2 - timer1 + 12 + desviacion;  // Añadimos 12ms por demoras
        if (resultado < 100) {                              //Por debajo de 100ms en Atletismo se considera SALIDA NULA   ************
          salida = "<h2 style='color:red'>** SALIDA NULA **</h2>";
          delay(500);// dejamos sonar un poco el disparo antes de los pitidos de aviso por NULO
          for (int i = 0; i < 10; i++) {
            beep();
            delay(50);
            color = "red";
          }
        }
        else {
          salida = "<h2 style='color:green'>* SALIDA CORRECTA *</h2>";
          color = "yellow";

        }
        delay(750);// dejamos sonar un poco el disparo antes de dar los resultados
        digitalWrite(PinBUZZER, LOW);
        break;
      }
      else
      {
        tiempo_desde_disparo = millis();
        if ((tiempo_desde_disparo - timer1) > 3000) {  // **********  Apagamos el buzzer 3 segundos después del disparo y colocamos el mensaje "SIN SALIDA"
          digitalWrite(PinBUZZER, LOW);
          salida = "<h2 style='color:pink'>- NO HUBO SALIDA -</h2>";
          resultado = 0;
          break;
        }
      }
    }
  }

salto:




  //--------------------------------------------------------------------------------------------------------------------------------------------------------------



  if (peticion.indexOf('START=F') != -1) {  // RESET valores
    beep();
    color = "yellow";
    sensibilidadSTR = "<h2 style='color:orange'>Sensibilidad: MEDIA</h2>";
    x = 0;
    x1 = 0;
    resultado = 0;
    timer1 = 0;
    timer2 = 0;
    desviacion = 0;
    digitalWrite(PinBUZZER, LOW);

  }

  if (peticion.indexOf('START=P') == 11) {  // Suma +5 ms (correción de desviación)
    beep();
    desviacion = desviacion + 5;
  }
  if (peticion.indexOf('START=Q') == 11) {  // Suma -5 ms (correción de desviación)
    beep();
    desviacion = desviacion - 5;
  }

  // *******************************************************    Envía la página HTML de respuesta al cliente
  client.println("HTTP/1.1 200 OK");
  client.println("");                                        // No olvidar esta línea de separación
  client.println("<!DOCTYPE HTML>");
  client.println("<meta charset='UTF-8'>");
  client.println("<meta name='MobileOptimized' content='width' />");
  client.println("<html>");
  client.println("<body style='background-color:black; border: 1px solid; color: #FF7F00;'>");   // Web Page Heading
  client.println("<font color='grey'>");

  client.println("<center><h2 style='color:#FF7F00'>CLUB ATLETISMO LEGANES</h2>");
  //client.println("<center><h2 style='color:#FF7F00'></h2>");

  client.println("<h2>Tiempos de Reacción</h2>");
  client.println("<p><100ms: SALIDA NULA</p>");

  //client.println(resta_pto);
  client.println("<br><br>");

  client.print("<h1 style='color:");
  client.print(color);
  client.println("'>");

  client.print(resultado);                           // Mostramos el resultado, es decir el TR (Tiempo de reacción)
  client.println(" ms</h1>");
  client.println("<br>");
  client.println(salida);                            // Mostramos Salida NULA o CORRECTA
  client.println("<br><br>");

  //Se crean botones con estilos
  client.println("<button type='button' onClick=location.href='START=O' style='margin:auto; background-color:green; color:#A9F5A9; padding:5px; border:2px solid black; width:200;'><h2> -- GO --</h2> </button>");
  client.println("<button type='button' onClick=location.href='START=F' style='margin:auto; background-color:red; color:#F6D8CE; padding:5px; border:2px solid black; width:200;'><h2> RESET</h2> </button><br><br>");

  client.println(sensibilidadSTR);                      //Mostramos la sensibilidad del Acelerómetro

  //Se crean 3 botones para modificar la sensibilidad del Acelerómetro

  client.println("<button type='button' onClick=location.href='/START=L'> Sens BAJA </button>");
  client.println("<button type='button' onClick=location.href='/START=M'> Sens MEDIA </button>");
  client.println("<button type='button' onClick=location.href='/START=H'> Sens ALTA </button><br><br>");

  client.print(" Desviación: ");
  client.print(desviacion);
  client.print(" ms   ");
  client.print("<button type='button' onClick=location.href='/START=P'>  + 5   </button>");
  client.print("<button type='button' onClick=location.href='/START=Q'>  - 5   </button>");

  client.print("<br><br>");


  client.println("</center></font></body></html>");
  delay(1);
  //Serial.println("Petición finalizada");          // Se finaliza la petición al cliente. Se inicaliza la espera de una nueva petición.
}
