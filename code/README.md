- ### esp-mesh

Reseau MESH établit avec le protocole ESP-MESH d'IDF.
Documentation: [ESP-MESH API guide](https://docs.espressif.com/projects/esp-idf/en/v3.3.1/api-guides/mesh.html)

####Structure du projet:
```
.
├── build
│   ├── ...
⁝    ⁝
│   │
├── CMakeLists.txt
├── logs
│   ├── invalidChild
│   │   ├── logEsp0.txt
│   │   ├── logEsp1.txt
│   │   └── logWireshark.pcapng
├── main
│   ├── CMakeLists.txt
│   └── esp-mesh.c
├── Makefile
└── sdkconfig

```

`logs` Dossier contenant les logs des problèmes rencontrés lors du développement du projet.
`main` Dossier contenant le code principale


- ### wifi_scan

Scanner de points d'accès. 

- ### <span>server.py</span>
Simple serveur UDP affichant les messages reçus qui sera utile pour ce projet. 