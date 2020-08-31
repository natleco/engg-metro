# ENGG Metro - Carriage 2

This is the main repository for the Comms teams working on **carriage 2**, specifically: **T1_Comms_3** and **T1_Comms_5**.

The code-base has been split into two folders, one for the code to be uploaded to the UNO and another for the code to be uploaded to the MEGA.

## **Directory structure**

Explanations for directories and their containing files are detailed below:

```
.
├── .vscode                     # VSCode config files
│   ├── arduino.json            # Config file for the Arduino extension in VSCode
│   ├── c_cpp_properties        # Config file for the C/C++ extension in VSCode
│   └── launch.json             # Config file for running '.ino' files in VSCode
│
├── MEGA                        # State Machine / Motors / Sensors / Comms-Receiver code-base
│   ├── MainTrain_Testing.ino   # Code for Arduino MEGA (for testing)
│   └── MainTrain.ino           # Code for Arduino MEGA (for production)
│
├── UNO                         # Control Box / Comms-Sender code-base
│   ├── ControlBox_Testing.ino  # Code for Arduino UNO (for testing)
│   └── ControlBox.ino          # Code for Arduino UNO (for production)
└── ...
```

---
<br>
<div align="center">
    <img src="https://github.com/natleco/engg-metro/raw/master/assets/system-concept-diagram.png" alt="System Concept Diagram" width="750px">
</div>