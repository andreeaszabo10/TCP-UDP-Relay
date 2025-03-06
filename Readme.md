Copyright Szabo Andreea 2024-2025

# TCP-UDP


Am implementat o aplicatie care sa simuleze modelul client-server, folosind multiplexarea I/O realizata cu select().

Structuri folosite:
- structura de mesaj udp pe care am folosit-o pentru a separa componentele mesajului primit, adica topicul, tipul de date si valoarea efectiva
- structura de mesaj tcp care contine lungimea mesajului si mesajul efectiv, lungimea este retinuta pentru a nu trimite bytes in plus
- structura de mesaj de comanda, care este folosita atunci cand un client tcp nou se conecteaza pentru a transmite serverului id-ul noului client, sau pentru comenzile de subscribe/unsubscribe, pentru a transmite topicul corespunzator catre server


Functionalitatea aplicatiei:
- este pornit serverul si apoi sunt deschisi 2 socketi, unul pentru tcp si unul pentru udp. Pentru a realiza acest lucru am folosit o parte din functia main din laboratorul 7, pe care am dublat-o pentru udp
- apoi in server vad dacă am primit de la tastatura comanda exit, daca am primit un mesaj de la udp, daca s a conectat un client tcp sau daca am primit o comanda de la un client tcp.
* pentru cazul in care primesc mesaj de la udp, verific care sunt clientii abonati si online si le trimit mesajul, trimitand mai intai lungimea si apoi mesajul, pentru eficienta.
* pentru cazul cand se conecteaza un nou client tcp, verific dacă mai exista alt client cu acelasi id deja online si daca e, afisez un mesaj de eroare, altfel il adaug id ul in map urile respective
* daca s-a primit o comanda de subscribe, adaug topicul in lista clientului daca nu era deja, iar la unsubscribe il scot. Pentru acest lucru, primesc un mesaj care contine topicul topicul cerut si verific daca actiunea ceruta de client se poate realiza
- pentru clientul tcp, creez un socket pe care il leg la server. Daca primesc comanda exit ies din program, daca primesc comanda de subscribe, creez un mesaj prin care anunt serverul la ce topic vrea sa se aboneze, iar la unsubscribe trimit un mesaj cu topicul de la care vrea sa se dezaboneze
- pentru wildcarduri, am facut cate un caz pentru 3 situatii: cand am doar +, doar *, sau combinate
