PROJEKTNAME: Ultraschall-Sprintzeitmesser mit dem Namen CloudEgg... der Name hat wenig bis nichts mit dem Projekt zu tun ^^

----------------------------------------

BESCHREIBUNG
------------

Sprintzeitmesser auf Basis einer Ultraschallbarriere mithilfe eines ESP32 Mikrocontrollers und einem HC-SR04 Ultraschallsensor.
Läufer können sich mit dem Smartphone per WLAN mit dem Messgerät verbinden und so ihre Messungen starten. Die gemessene Zeit wird laufend aktualisiert auf dem Smartphone und dem Display am Mikrocontroller angezeigt.
Die Messung endet, sobald der Läufer durch die Ultraschallbarriere läuft.

FUNKTIONEN
----------

- Ultraschallsensor: Der HC-SR04 Ultraschallsensor wird verwendet, um die Entfernung zwischen dem Sensor und den Objekten vor dem Sensor zu messen. 
- ESP32 Mikrocontroller: Der ESP32 steuert Ultraschallsensor und Display. Ein Webserver wird auf dem ESP32 gehostet, um eine Benutzeroberfläche für die Interaktion mit dem Sprintzeitmesser bereitzustellen.
- Browser-Interface: Benutzer können sich mit dem Sprintzeitmesser über einen Webbrowser, idealerweise auf einem Smartphone, verbinden, um Messungen zu starten und die Ergebnisse anzuzeigen.
- Echtzeitmessung: Der ESP32 führt etwa 50 Messungen pro Sekunde durch, um die Bewegung des Objekts zu überwachen und die Zeit zu stoppen, wenn sich das Objekt in einem Meter oder weniger Entfernung vor dem Sensor befindet.
- Anzeige der Zeit: Die gemessene Zeit wird sowohl auf dem Browserfenster als auch auf einem angeschlossenen Display am ESP32 angezeigt.

HARDWARE-ANFORDERUNGEN
-----------------------

- ESP32 Mikrocontroller
- HC-SR04 Ultraschallsensor
- Optional: LAFVIN 0.96 Zoll OED I2C Display

SETUP-ANLEITUNG
---------------

1. Verbinde den HC-SR04 Ultraschallsensor mit dem ESP32 Mikrocontroller: Echo auf D33 und Trigger auf D25.
2. Lade den entsprechenden Code auf den ESP32 hoch.
3. Verbinde den ESP32 mit Strom
4. Suche und Verbinde dich mit dem Smartphone/PC per WLAN mit dem Netzwerk "CloudEgg network" (Passwort: cloudegg)
6. Öffne die IP-Adresse des ESP32 (192.168.4.1) mit einem Browser, um auf die Benutzeroberfläche zuzugreifen.
7. Starte eine Messung über die Benutzeroberfläche. Der Ablauf ist selbsterklärend. Nach einer akustischen Tonabfolge startet die Messung. Sobald man durch die Ultraschallbarriere läuft, stoppt die Messung.
8. Die gemessene Zeit wird auf dem Browserfenster und dem angeschlossenen Display angezeigt.

HINWEISE
--------

- Stelle sicher, dass der Ultraschallsensor korrekt ausgerichtet ist, um genaue Messungen zu erhalten. Vor dem Sensor sollten sich möglichst keine Objekte im Abstand von 5 Metern befinden
- Öffne die Adresse 192.168.4.1/validatePos um eine automatische Positionsvalidierung durchzuführen.
- Die Genauigkeit der Messungen kann je nach Umgebung variieren. Teste die Schranke mit Testmessungen, bevor das Training beginnt.

AUTOREN
-------

- Kimio Nishiura (kimio.nishiura@me.com)

