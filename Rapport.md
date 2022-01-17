# Mini Projet IoT 2021-2022
# “Réseau de sirènes d’alarme LoRaWAN”
![alt text](Image/boite.png "Boite")

## Architecture globale du réseau de sirènes d’alarme

L’objectif est de développer un périphérique qui pourra être déployé en réseaux afin de récolter des informations sur plusieurs positions d’un même lieux (exemple entrepôt, pièce de vie, atelier etc…) ou sur différents lieux. Il permetra de pouvoir monitorer ces lieux, déclenche une alerte en cas de détection d'incendie ou en cas d'appuie sur un bouton. 

L'ensemble des informations sera récolté et sera visualisé sur un panneau de contrôle. 


## Sécurité de notre périphérique

Pour le développement de notre périphérique nous pouvons modifier la sécurité d'une partie. En effet, à partir de gateway nos données sont transmises par internet et donc chiffrées en équivalence. Ensuite l'accès au application tels que campus iot et mydevice la sécurité ce joue sur les mots de passe choisie.

La **partie de sécurité** sur laquelle nous pouvons intervenir est le chiffrage de la trame de donnée entre notre device et le serveur campus iot sur lequel on distribue ensuite l’information à mydevices. 

Pour la communication de notre trame nous avons choisi le mode de transmission OTAA (Over The Air Activation). 
Dans le protocole OTAA, on choisit une clé de 64 bits qui se trouve codé en dure dans notre devise et dans l’application. 
Pendant la procédure de “join”, 2 clés sont générées dynamiquement entre le périphérique et l’application, c’est 2 clés se nommant NwkSkey et AppSkey.

La NwkSkey, permet l’identification du message et empêche les attaques de type modification de message à la volé.

L'autre clé, AppSkey socupe de chiffrer point à point le message.  

## Architecture matérielle de l’objet :
![alt text](Image/RapportIOT.png "Architecture matérielle")

- fgg


