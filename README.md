# Web_Client
Client meant to communicate with a server based off an REST API 

### Voicu Teodora-Andreea
### 323CA

### am utilizat 2 sleep days

Acest proiect consta in implementarea unui client HTTP care interactioneaza cu un server printr-o API REST pentru gestionarea unei biblioteci virtuale. Clientul permite inregistrarea si autentificarea utilizatorilor, accesarea bibliotecii, adaugarea, vizualizarea, stergerea si modificarea cartilor din biblioteca.

Programul incepe prin asteptarea comenzilor de la utilizator, procesandu-le in main-ul programului. Comenzile sunt: register, login, enter_library, get_books, get_book, add_book, delete_book, logout si exit. 
Pentru fiecare comanda, se apeleaza functiile corespunzatoare care genereaza si trimit cereri specifice serverului, apoi proceseaza raspunsurile primite.

Autentificare si Inregistrare

    process_login_or_register: gestioneaza atat inregistrarea cat si autentificarea. Construieste un obiect JSON cu numele de utilizator si parola, trimite cererea POST la server si proceseaza raspunsul.
    register_user: inregistreaza un nou utilizator folosind functia de mai sus. Verifica raspunsul pentru a determina daca inregistrarea a fost cu succes sau daca utilizatorul exista deja.
    login_user: autentifica un utilizator si, daca autentificarea este reusita, extrage si salveaza cookie-ul de sesiune necesar pentru actiunile ulterioare.

Manipularea Bibliotecii

    enter_library: solicita accesul la biblioteca. Daca utilizatorul este autentificat, functia extrage tokenul necesar pentru a efectua operatiuni asupra cartilor.
    get_books, get_book, add_book, delete_book: aceste functii permit utilizatorului sa vizualizeze cartile disponibile, sa obtina detalii despre o carte specifica, sa adauge o noua carte sau sa stearga o carte existenta. Fiecare functie construieste cererea corespunzatoare (GET pentru get_books si get_book, POST pentru add_book si DELETE pentru delete_book) si trimite aceste cereri la server.

Finalizarea Sesiunii

    logout_user: deconecteaza utilizatorul prin stergerea cookie-ului de sesiune si a tokenului.
    exit_program: curata resursele si incheie executia programului.

De asemenea am folosit functiile compute_get_delete_request si compute_post_request,
modificate fata de implementarea din laborator.

Programul foloseste biblioteca parson pentru manipularea obiectelor JSON.
S-a implementat o gestionare detaliata a erorilor pentru fiecare etapa a procesului, de la conectarea la server pana la procesarea raspunsurilor de la server.

