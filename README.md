# Réseau Wi-Fi multi-sauts sur platforme ESP
#### Projet de master 1 en sciences informatiques à l'Université de Mons.

*Auteur*: **Arnaud Palgen**<br />
*Directeur*: **Bruno Quoitin**<br />
*Rapporteurs*: **Alain Buys**, **Jeremy Dubrulle**

---
L'objectif du projet est de concevoir un réseau MESH multi-sauts sur l'ESP32 d'Espressif.
L'environnement de développement utilisé est [ESP-IDF v3.3.1](https://docs.espressif.com/projects/esp-idf/en/v3.3.1/) (version stable supportée jusqu'en Février 2022).

## Étapes du projet 
1. Établir un état de l'art nous permettant de choisir un environnement de développement et un protocole de routage MESH adapté au projet.
2. Étudier le protocole [*AODV*](https://tools.ietf.org/html/rfc3561).
3. Étudier le protocole [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/v3.3.1/api-guides/mesh.html) d'IDF.
4. Mettre en oeuvre ESP-MESH.
5. Expérimenter [ESP-NOW](https://docs.espressif.com/projects/esp-idf/en/v3.3.1/api-reference/network/esp_now.html) qui pourrait servir de base pour implémenter  un protocole tel qu'*AODV*.

Le [rapport de projet](rapport.pdf) décrit ces étapes en détail et les ressources utilisées sont disponibles [ici](./ressources).
La présentation a été réalisée pour une défense orale du projet d'une durée de 15 minutes.

## Outils utilisés
- Visual Studio Code
- Wireshark: utilisé avec une interfac Wi-Fi Edimax pour écouter le traffic Wi-Fi du réseau MESH.<br />
Paramètrage de l'interface en mode *monitor* sous Ubuntu 18.04.4 LTS réalisé via [ce tutoriel](https://sandilands.info/sgordon/capturing-wifi-in-monitor-mode-with-iw) [consulté le 10 avril 2020].

## Matériel utilisé
- [x1] Edimax EW-7811Un 802.11n Wireless Adapter
- [x1] Routeur Wi-Fi
- [x5] [ESP32-DevKit-LiPo](https://www.olimex.com/Products/IoT/ESP32/ESP32-DevKit-LiPo/open-source-hardware)
