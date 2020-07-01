### Contenu
- *esp-mesh*: projet permettant d'envoyer des données vers une adresse IP externe
- *esp-mesh-internal*: projet permettant d'envoyer des données vers un noeud interne au réseau ESP-MESH
- *esp-mesh-proxy*: projet permettant d'établir des communications bidirectionnelles entre un noeud du réseau (qui initie la communication ) et  adresse IP externe
- *esp-now*: projet permettant d'envoyer des paquets esp-now en broadcast
- *socket*: projet illustrant la création d'un socket tcp
- *tcpServer.py*: serveur tcp
- *espMeshWiresharkDissector.lua*: dissecteur ESP-MESH pour Wireshark

#### procédure ####
- Projets IDF
    - Installer la version 3.3.1 d'idf. (https://docs.espressif.com/projects/esp-idf/en/v3.3.1/get-started/index.html)
    - Se rendre dans le répertoire courant du projet
    - Sourcer le fichier export.sh (. $IDF_PATH/export.sh)
    - idf.py menuconfig
    - Se rendre dans le meu "ESP-MESH", "ESP-MESH configuration" ou "SOCKET" en fonction du projet
    - Configuer le SSID du point d'accès Wi-FI, mot de passe, channel
    - Sortir de la configuration
    - Compiler le projet (idf.py build)
    - Flasher le projet (idf.py -p <port> flash)
    - Communication série avec l'esp: idf.py -p (port) monitor

- Serveur TCP
    l'éxécuter via: python3 tcpServer.py <adresse IPv4> <PORT>

- Dissecteur ESP-MESH
    - dans Wireshark, se rendre dans aide -> à propos de Wireshark -> Dossiers
    - repérer le répertoire ayant pour nom "Personal Lua Plugins"
    - copier le fichier espMeshWiresharkDissector.lua dans ce répertoire
    - redémarrer Wireshark et le dissecteur sera pris en compte
