# Réseau Wi-Fi multi-sauts sur platforme ESP

*Auteur*: **Arnaud Palgen**<br />
*Directeur*: **Bruno Quoitin**<br />
*Rapporteurs*: **Alain Buys**, **Jeremy Dubrulle**

---
L'objectif du projet est de concevoir un réseau MESH multi-sauts sur l'ESP32 d'Espressif.
L'environnement de développement utilisé est [ESP-IDF v3.3.1](https://docs.espressif.com/projects/esp-idf/en/v3.3.1/) (version stable supportée jusqu'en Février 2022).

## Étapes du projet 
1. Établir un état de l'art nous permettant de choisir un environnement de développement 
et un protocole de routage MESH adapté au projet.
2. Développer un réseau MESH avec [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/v3.3.1/api-guides/mesh.html) d'IDF ([esp-mesh](./code/esp-mesh)).
3. Évaluer les performances de ce réseau.
4. Etudier ESP-NOW qui pourrait servir de base pour implémenter *AODV*. ([ESP_NOW](./code/esp-now))
5. Développer un réseau MESH avec le protocole *AODV* ([aodv](./code/aodv)).
6. Évaluer les performances de ce réseau.

Le [rapport de projet](./rapport/out/rapportProjet.pdf) décrit ces étapes en détail et les ressources utilisées sont disponibles [ici](./ressources).

## Outils utilisés
- Visual Studio Code
- Wireshark: utilisé avec l'interfac Wi-Fi Edimax pour écouter le traffic Wi-Fi du réseau MESH.<br />
Paramètrage de l'interface en mode *monitor* sous Ubuntu 18.04.4 LTS réalisé via [ce tutoriel](https://sandilands.info/sgordon/capturing-wifi-in-monitor-mode-with-iw) [consulté le 10 avril 2020].
<!--```console
user@computer:~$ iwconfig <interface_name>
<interface_name>  IEEE 802.11  ESSID:off/any  
          Mode:Managed  Access Point: Not-Associated   Tx-Power=20 dBm   
          Retry short limit:7   RTS thr=2347 B   Fragment thr:off
          Power Management:off
          
user@computer:~$ sudo ifconfig <interface_name> down;sudo iwconfig <interface_name> mode monitor
user@computer:~$ sudo ifconfig <interface_name> up
user@computer:~$ iwconfig <interface_name>
<interface_name>  IEEE 802.11  Mode:Monitor  Frequency:2.412 GHz  Tx-Power=20 dBm   
          Retry short limit:7   RTS thr=2347 B   Fragment thr:off
          Power Management:off
          
```
-->
- ...

## Matériel utilisé
- [x1] Edimax EW-7811Un 802.11n Wireless Adapter
- [x1] Raspberry Pi 2B: utilisé comme adaptateur JTAG
- [x1] Raspberry Pi Zero W: utilisé comme point d'accès Wi-Fi
- [x2] [ESP32-DevKit-LiPo](https://www.olimex.com/Products/IoT/ESP32/ESP32-DevKit-LiPo/open-source-hardware)
