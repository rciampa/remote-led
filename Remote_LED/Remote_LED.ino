/*
   Author: Richard Ciampa
   Date: 7/15/2016
   File: Remote_LED.ino

   Created for CSU Monterey Bay Summer Code Camp

   Abstract: The program sets up a network controller (WizNet 5100), then makes
   repeated HTTP GET method calls to a webservice located at:
   http://hosting.otterlabs.org/ciamparichardd/public_html/camp/switch/

   Once a switch file has been created from the Remote Switch Service the microcontroller
   sets an LED output pin based on the switch setting value. The url for the GET request
   can be found on the remote switch service user interface in "Link to File" item of the
   page.
*/

/* The digital pin connected to the anode of the LED */
#define LED_PIN 9
/* Include the ethernet library for the WizNet controller */
#include <Ethernet.h>

/* last time you connected to the server, in milliseconds */
uint32_t lastConnectionTime = 0;


void setup() {
  Serial.begin(9600); //Start serial communication for output
  while (!Serial);    //Wait for usb serial port to start

  /*
     Start the ethernet controller and obtain an ip address from
     a dhcp server on the local LAN
  */
  do {
    byte * mac = set_mac();
    Serial.println(F("Starting ethernet controller....."));

    if (Ethernet.begin(mac)) {
      /*
        Wait a second for the ethernet controller to initailize and
        obtain an ip address lease.
      */
      delay(1000);
      /*
        Print the ethernet controller ip configuration to thr serial
        port.
      */
      print_network_information();

      break;
    }else{
      free(mac);//Free the memory so we can loop again
      Serial.print(F("[ERROR]: Failed to start ethernet controller"));
    }
  } while (true);
}

void loop() {
  /*
     If ten seconds have passed since your last connection,
     then connect again and send data
  */
  if (millis() - lastConnectionTime > posting_interval(10)) {
    httpRequest();
  }
}

/*
   Prints dhcp network configuration information to the serial port.
*/
void print_network_information() {
  Serial.println(F("\n------------- DHCP Network Configuration --------------"));
  Serial.print(F("IP address: "));
  Serial.println(Ethernet.localIP());
  Serial.print(F("Subnet mask: "));
  Serial.println(Ethernet.subnetMask());
  Serial.print(F("Gateway address: "));
  Serial.println(Ethernet.gatewayIP());
  Serial.println(F("-------------------------------------------------------\n"));
}

/*
   This method makes a HTTP connection to the server
*/
void httpRequest() {
  EthernetClient client;
  char server[] = "hosting.otterlabs.org";  // Web sever hostname the runs the remote switch service

  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.print(F("\nConnecting on socket #: "));
    Serial.print((char *) client.getSocketNumber());
    //Send the HTTP GET request to the server
    client.print("GET ");
    client.print("/ciamparichardd/public_html/camp/switch/Switch/one.txt");
    client.println(" HTTP/1.1");
    client.println("Host: hosting.otterlabs.org"); //Web server fqdn hostname in request
    client.println("User-Agent: Arduino-UnoR3");
    client.println("Connection: close");
    client.println();

    /*
     * Wait for the http request response stream to be available
     */
     Serial.print(F("\nWaiting for http response "));
    do {
      Serial.print(F("."));
      if (client.available()) {
        Serial.println(F(" Done!\n"));
        set_led_state(get_switch_state_from_response(&client));
        break;
      }
    } while (!client.available());

    //Note the time that the connection was made
    lastConnectionTime = millis();
  } else {
    //If you couldn't make a connection
    Serial.println(F("connection failed"));
  }

  /*
     Close any connection before sending a new request.
     This will free the socket on the WiFi shield, the maximum is
     four (4) connections.
  */
  client.flush();
  client.stop();

  /* Renew the ip address lease from dhcp server */
  renew_dhcp_lease();
}

/*
   This method prints the response from the http request then
   extracts switch state value and returns it to the calling
   code.
*/
char get_switch_state_from_response(EthernetClient * client) {

  while (client->available() > 1) {
    Serial.write(client->read());
  }
  char remote_switch_state = client->read();
  Serial.println(remote_switch_state);
  Serial.println(F("\nRequest complete.........\n\n"));

  if (remote_switch_state == '0' || remote_switch_state == '1') {
    return remote_switch_state;
  } else {
    return NULL;
  }
}

/*
   This method sets the digital pin low(led off) or high(led on) based
   on the value (0 or 1) extracted from the http response from the
   remote switch service.
*/
void set_led_state(char value) {
  if (value != NULL) {
    Serial.println(F("Request has valid switch state value"));
    if (value == '0') {
      Serial.println(F("Switch state: OFF"));
      digitalWrite(LED_PIN, LOW); /* Use for timing example */
      //PORTB = PORTB & B101;
    } else {
      Serial.println(F("Switch state: ON"));
      digitalWrite(LED_PIN, HIGH); /* Use for timing example */
      //PORTB = PORTB | B010;
    }
    //Serial.println(PORTB, BIN); /* Use for timing example */
  } else {
    Serial.println(F("[ERROR]: Request has invalid switch state value"));
  }
}

/*
 * Renews the ip address lease from the local dhcp server
 */
void renew_dhcp_lease(){

  switch(Ethernet.maintain()){
    case 0:
    Serial.println(F("\n[MSG]: DHCP lease is valid....."));
    break;
    case 1:
    Serial.println(F("\n[ERROR]: DHCP renew lease failed....."));
    break;
    case 2:
    Serial.println(F("\n[MSG]: IP address lease renewed....."));
    break;
  }
}

/*
   We are going to use dhcp, this will allow the ip address to
   be managed by lease reservations in the local dhcp server.

   We will only need to provide the mac address, a byte array to hold
   the media access control (MAC) physical address is required.
*/
byte * set_mac(){
  byte * mac = (byte *) malloc(6);
  mac[0] = 0xDE;
  mac[1] = 0xAD;
  mac[2] = 0xBE;
  mac[3] = 0xEF;
  mac[4] = 0xFE;
  mac[5] = 0xED;
  
  return mac;
}

/*
   delay between updates, in milliseconds the "L" is
   needed to use long type numbers
*/
uint32_t posting_interval(uint32_t milis){
  return milis * 1000L;
}
