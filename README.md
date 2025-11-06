*** Repository Description *** 

This repository contains all the materials required for the development of an IoT-Based Residential Electricity Consumption Monitoring and Management System.

*** The system is composed of two main nodes *** 

- Node 1: Responsible for measuring electrical consumption and remotely managing household appliances.

- Node 2: Responsible for measuring the overall household electricity consumption and remotely managing electrical circuits.

*** Electronic Components ***

- ESP32 Module

- PZEM-004T V3 Sensor

- 2-Channel Relay Module

- Solid-State Relays (SSR)

* For more details on the electrical connections and circuit diagrams for each node, refer to the folder “Diagrama de Circuitos” (Circuit Diagram).

*** InfluxDB Database Files ***

The data stored in InfluxDB are divided into 10 split archive files, each identified by a numerical suffix in its folder name.
To access them:

1. Download all parts and place them in the same folder.

2. Open only the file smart_grid_home_parted.7z.001 using 7-Zip; the remaining parts will automatically merge during extraction.
