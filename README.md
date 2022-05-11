# C2 Framework & Implant

## Authors
- Ashburn Hu
- Julian Maldonado
- Kamila Uranayeva
- Noam Metivier
- Stephan Schmidt
# Installation 


To install implant you run loader.exe with the c2 server running

### C2 Server (Windows)
```
cd .\C2-Server\
pip install -r requirements.txt
python .\make_db.py
.\run.bat
```

how do I modify the config to allow for connections to different C2s
## Modify config
Because our malware runs using a P2P network, you would need to modify the 
config to connect to a c2 that also has P2P functionality. In our inital loader we specify the URL
that it needs to connect to in order to download the PEs needed to run properly. Changing this URL to a different 
C2 that has the same .exe files needed would allow it to work properly with other C2s. 

Requirements for  Implant, client, c2...etc

#  Capabilities

what can your framework do?

- execution, file IO, supported C2 Channels, use of cryptography....etc
- For cryptography we use AES-GCM to enc/dec messages
- For file IO, we read the file where chrome passwords are stored and once this is decrpyted we send this information to the server where it is then stored in the server
- 

# Opsec

Pros:
  - two forms of persistance
Cons:
  - our persistance methods are relativaeley "loud" one shows up in a GUI and the other is boot run keys
# Utility

1) If as an attacker you what is on the computer or who the computer is, you're able to run basic commands from the server side that the C2 can then send to the implant. These include things such as whoami
2) As an attacker you know that the person you are trying to get information from doesn't use a 3rd part password manager and instead uses chrome to store their passwords. Using our stealer, you are able to not only get the specific information you're looking for but also any other passwords that they might store on chrome. From there you may be able to access the website and get more sensitive information that you didn't have access to or even know about before depending on the website. This can include things such as their address, credit card information, and more.

# Collaboration breakdown

Stephan - P2P functionality (Both on C2 & Implant) as well as login credentials
Noam - Methods of persistence
Ashburn - Looting functionality
Kamila - AES-GCM Enc/Dec
Julian - Routes on C2 and building DB



### Makefile provided for implant code






