# :orange_book: Word Count

<div id="top"></div>

[![MIT License][license-shield]][license-url]

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Tabella dei contenuti</summary>
  <ol>
    <li>
      <a href="#introduzione-al-problema">Introduzione al problema</a>
    </li>
    <li>
      <a href="#definizione-della-soluzione">Definizione della soluzione</a>
      <ul>
        <li><a href="#soluzione-ad-alto-livello">Soluzione ad alto livello</a></li>
      </ul>
    </li>
    <li>
      <a href="#struttura-del-progetto">Struttura del progetto</a>
    </li>
    <li>
      <a href="#implementazione">Implementazione</a>
      <ul>
        <li>
          <a href="#controllo-dellinput">Controllo dell'input</a>
        </li>
        <li>
          <a href="#calcolo-dei-bytes-totali">Calcolo dei bytes totali</a>
        </li>
        <li>
          <a href="divisione-delle-porzioni-di-files">Divisione delle porzioni di files</a>
        </li>
      </ul>
    </li>
  </ol>
</details>

<!-- INTRODUZIONE AL PROBLEMA -->
## Introduzione al problema

<p align="right">(<a href="#top">torna su</a>)</p>

<!-- DEFINIZIONE DELLA SOLUZIONE -->
## Definizione della soluzione

Uno degli obiettivi fondamentali del progetto è la distribuzione equa del carico di lavoro in modo tale da massimizzare il più possibile lo speed-up. Risulta quindi necessario definire una soluzione efficiente per la distribuzione dei files sui diversi processi a disposizione.

Un approccio immediato potrebbe essere quello di distribuire il numero di files per i processi a disposizione. Questa soluzione, per quanto semplice, risulta essere poco efficiente per due motivi:
* il numero di processi potrebbe essere superiore rispetto al numero di files da leggere;
* la dimensione dei files potrebbe variare anche di diversi ordini di grandezza.

Adottare questa soluzione porterebbe:
* nel primo caso ad utilizzare solo un sottoinsieme dei processi a disposizione;
* nel secondo caso ad una limitazione dello speed-up: la presenza di files molto più grandi rispetto ad altri implicherebbe tempi di esecuzione molto più lunghi per alcuni processi.

La seconda motivazione apre le porte a quella che è una soluzione molto più efficiente rispetto a quella banale: invece di dividere il numero di files per i processi a disposizione possiamo dividere il carico di lavoro in base al numero totale di bytes da leggere. In questo modo, la presenza o meno di files con dimensione diversa fra loro non comprometterebbe in nessun modo una divisione equa fra i processi.

### Divsione in base al numero totale di bytes

In questa sezione formalizziamo la strategia da utilizzare per distribuire il lavoro fra i vari processi.

La prima operazione da effettuare è quella relativa al calcolo del numero totale di bytes. Fatto ciò, dividiamolo per il numero di processi a disposizione, in modo da ottenere il numero di bytes destinato ad ogni processo. Potrebbe accadere però che il numero di processi non sia multiplo del numero totale di bytes. Questo causerebbe una perdità di distribuzione degli ultimi `r` bytes da leggere, dove `r` è proprio il resto della divisione. Banalmente si potrebbe pensare di delegare la lettura di quest'ultimi `r` bytes ad un unico processo. Una strategia più efficiente è quella di incaricare i primi `r` processi a leggere un singolo bytes in più, in modo da non sovraccaricare l'utilizzo di un processo rispetto ad un altro.

Quindi, siano p<sub>1</sub>, p<sub>2</sub>, ..., p<sub>n</sub> i processi a disposizione e siano `size` e `rest` relativamente il risultato e il resto della divisione fra il numero totale di bytes e il numero di processi,  
allora il processo p<sub>i</sub> sarà incaricato di leggere un numero di bytes pari a `size + 1` se `i` è minore uguale di `rest`, altrimenti leggerà `size` bytes. 

### Soluzione ad alto livello

La soluzione è stata implementata tramite l'utilizzo di un'architettura **master-slaves**. In base al ruolo del processo all'interno dell'architettura è possibile dividere la soluzione in diverse fasi, di seguito riportate.

Il processo **master** si occupa di:
* controllare l'input del programma; 
* calcolare il numero totale di bytes dei files;
* inviare le porzioni di files da leggere agli **slaves**;
* ricevere gli istogrammi locali dagli **slaves**;
* unire i risultati ricevuti in un unico istrogramma;
* ordinare l'istrogramma in ordine decrescente in base al numero di occorrenze;
* creare un file csv con le parole e le relative occorrenze.

I processi **slaves** si occupano di:
* ricevere le porzioni di files da leggere;
* contare le occorrenze delle diverse parole all'interno dei files;
* inviare l'istogramma locale al **master**.

<p align="right">(<a href="#top">torna su</a>)</p>

## Struttura del progetto

```
.
├── include
│   ├── file.h
│   ├── input.h
│   ├── log.h
│   ├── my-mpi.h
│   ├── sort.h
├── lib
│   ├── file.c
│   ├── input.c
│   ├── log.c
│   ├── my-mpi.c
│   ├── sort.c
├── src
│   ├── word-count.h
├── test
├── LICENSE
├── Makefile
├── README.md
├── install.sh
```

<p align="right">(<a href="#top">torna su</a>)</p>

## Implementazione

In questa sezione viene descritta l'implementazione relativa ad ogni fase della soluzione. Alcuni dei listati riportati di seguito non sono completi, in quanto l'obiettivo principale è quello di mostrare solo le parti fondamentali.

Come supporto per l'implementazione è stata utilizzata la libreria [glib.h](https://docs.gtk.org/glib/). In particolare sono state utilizzate le seguenti strutture dati:
* `GList`: definisce una struttura di tipo lista utilizzata per la memorizzazione dinamica delle informazioni relative ai files;
* `GHashTable`: implementazione di un hash table per la creazione degli istogrammi;
* `GHashTableIter`: utilizzata per poter iterare le coppie (chiave, valore) memorizzate all'interno dell'hash table.

### Controllo dell'input

L'operazione di controllo sull'input avviene tramite la funzione `check_input()` della libreria `input.h`. La funzione esegue semplicemente dei controlli sugli argomenti passati in input al programma. I possibili utilizzi del programma sono descritti nella sezione: [modalità di utilizzo](#modalità-di-utilizzo).

### Calcolo dei bytes totali

L'operazione di lettura dei files e calcolo dei bytes totali è stata implementata tramite la funzione `read_files()`, anche essa della libreria `input.h`. Quest'ultima, come mostrato nel listato successivo, prende in input:
* un riferimento ad una variabie di tipo `GList` la quale, al termine dell'esecuzione della funzione, conterrà la lista di files letti;
* un rifermo ad una variabile di tipo `off_t` che conterrà il numero totale di bytes letti.

``` c
GList *file_list;
off_t total_bytes = 0;

read_files(..., &file_list, &total_bytes);
```

All'interno della lista le informazioni relative ai files vengono memorizzate tramite il tipo `File`, definito all'interno della libreria `file.h` come segue:

``` c
#define MAX_PATH_LEN 256

typedef struct file {

    char path_file[MAX_PATH_LEN];
    off_t bytes_size;
    long start_offset;
    long end_offset;

} File;
```

La definizione di questa struttura permette la memorizzazione dei path assoluti e la dimensione di ogni file. I parametri `start_offset` e `end_offset` verranno inizializzati successivamente dal processo master per definire la parte di file da delegare ad ogni ogni processo.

### Divisione delle porzioni di files

In questa fase vengono divisi i files in porzioni diverse da inviare agli slaves. Il seguente listato mostra quali operazioni vengono effettuate dai diversi processi.

``` c
File *files;
guint n_files;          

MPI_Datatype file_type = create_file_type();

if (rank == MASTER) {

    send_files_to_slaves(size, file_list, total_bytes, file_type);

} else {

    recv_files_from_master(&n_files, &files, file_type);

}

MPI_Type_free(&file_type);
```

Le prime operazioni di questa fase comprendono la dichiarazione di un puntatore di tipo `File`, buffer utilizzato dagli slaves per ricevere le informazioni relative ai files da leggere, e di una variabile di tipo `guint` (alias `unsigned int`) che conterrà il numero di files che riceverà dal master.

Successivamente, viene eseguita la funzione `create_file_type()`, della libreria `my-mpi.h`, da parte di tutti i processi. Quest'ultima viene utilizzata per la creazione del tipo derivato `File` in modo da permettere ai processi di poter ricevere e inviare variabili di questo tipo. Il risultato di questa funzione viene memorizzato all'interno della variabile `file_type` di tipo `MPI_Datatype`.

Una volta creato il tipo `File`, i processi eseguono una funzione diversa in base al loro ruolo all'interno dell'architettura:
* master esegue la funzione `send_files_to_slaves()`;
* gli slaves eseguono la funzione `recv_files_from_master()`.

Entrambe le funzioni sono state implementate all'interno della libreria `my-mpi.h`.

#### Invio files agli slaves

La funzione `send_files_to_slaves()`, come suggerisce il nome, viene utilizzata dal master per comunicare agli slaves le diverse porizioni di files da leggere. Analizziamo ora il codice relativo a questa funzione.
Le prime operazioni sono relative al calcolo del risultato e del resto fra il numero totale di bytes e il numero di slaves.

``` c
int n_slaves = size - 1;

off_t bytes_for_each_processes = total_bytes / n_slaves;
off_t rest = total_bytes % n_slaves;
```

Successivamente viene allocato dinamicamente il buffer relativo ai files da inviare ad ogni processo. Il buffer avrà una dimensione pari al valore di `n_files` che, tramite l'utilizzo della funzione `g_list_length()`, conterrà il numero totale di files letti. Il valore di `n_files` rappresenta un upper-bound al numero reale di files che ogni processo dovrà leggere. Infatti, sarà necessario comunicare ad ogni slaves il numero effettivo di files che riceverà in modo da permettere ad ognuno di essi di allocare spazio a sufficenza per ricevere correttamente il messaggio.

``` c
guint n_files = g_list_length(file_list);
File *files = malloc((sizeof *files) * n_files);
```

Poiché le varie porzioni da destinare ad ogni processo vengono calcolate processo per volta, si è deciso di utilizzare una comunicazione non bloccante, in modo da permettere al programma di continuare con il calcolo relativo alle porzioni da destinare al processo successivo e contemporaneamente inizializzare la comunicazione verso il processo appena calcolato.  
L'utilizzo della comunicazione non-bloccante richiede che il buffer utillizzato all'interno della comunicazione non venga in nessun modo modificato fin quando la comunicazione non viene completata. Per questo motivo, dichiariamo una matrice `n_slaves` x `BUFFER_SIZE`, dove la riga `i` rappresenta un buffer di grandezza `BUFFER_SIZE` utilizzato dal master per inviare i dati all'i-esimo slaves. `BUFFER_SIZE` viene definito all'interno della libreria `my-mpi.h` ed ha un valore pari ad 8096.  
Inoltre, dichiariamo anche un array di tipo `MPI_Request`, anch'esso di dimensione `n_slaves`, utilizzato per la memorizzazione delle richieste relative ad ogni comunicazione. Quest'ultimo sarà necessario per attendere il completamento di tutte le comunicazioni effettuate.

``` c
char buffers[n_slaves][BUFFER_SIZE];
MPI_Request requests[n_slaves];
```

Il prossimo passo consiste nel calcolare, per ogni processo, il numero di bytes da inviare. Una volta fatto ciò, finché il numero di bytes da inviare è maggiore di 0, partiziono i file rimanenti in questo modo:
1. Leggo il file attuale all'interno della lista;
2. Calcolo il numero di bytes non distribuiti del file attuale;
3. Copio il file all'interno del buffer `files`;
4. Inizializzo i valori relativi a `start_offset` ed `end_offset`;
5. Se il file viene letto completamente mi sposto di una posizione in avanti all'interno della lista, altrimenti memorizzo il valore relativo all'ultimo offset letto e termino.

``` c
for (int i_slave = 0; i_slave < n_slaves; i_slave++) {

  off_t total_bytes_to_send = bytes_for_each_processes;
  if (rest > 0) {
    total_bytes_to_send++;
    rest--;
  }

  while (total_bytes_to_send > 0) { ... }
```

Terminata la partizione dei files relativa al processo corrente, tramite la funzione `MPI_Pack()`, impachettiamo all'interno del buffer `&buffers[0][i_slave]`, dove `i_slave` indica l'indice del processo corrente, il numero di files memorizzati all'interno del buffer `files` e il buffer `files` stesso. Il numero di files da inviare al processo corrente equivale al valore della variabile `i_file` utilizzata per tenere traccia della prossima cella libera all'interno del buffer `files`.

```
  guint n_files = i_file;

  int position = 0;
   
  MPI_Pack(&n_files, 1, MPI_UNSIGNED, 
    &buffers[0][i_slave], BUFFER_SIZE, 
    &position, MPI_COMM_WORLD);
    
  MPI_Pack(files, i_file, file_type, 
    &buffers[0][i_slave], BUFFER_SIZE, 
    &position, MPI_COMM_WORLD);
```

Una volta terminato l'impachettamento dei dati non ci resta che inviarli. Gli slaves, non avendo altre operazioni da effettuare prima della ricezione delle informazioni da parte del master, saranno sicuramente già in attessa del messaggio. Per questo motivo, possiamo utilizzare una comunicazione non-bloccante di tipo **ready**, tramite l'utilizzo della funzione `MPI_Irsend()`. Quest'ultima aggiunge un'informazione in più rispetto ad una send standard, permettendo ad MPI di eliminare l'operazione di hand-shake, e quindi ottenere un miglioramento delle presetazione.

```c
  MPI_Irsend(&buffers[0][i_slave], BUFFER_SIZE,
    MPI_PACKED, i_slave + 1, TAG_NUM_FILES, 
    MPI_COMM_WORLD, &requests[i_slave]);

}
```

Una volta che tutte le 

``` c
MPI_Waitall(n_slaves, requests, MPI_STATUS_IGNORE);
```

#### Ricezione dei files dal master


<p align="right">(<a href="#top">torna su</a>)</p>

# Modalità di utilizzo

<!-- MARKDOWN LINKS & IMAGES -->
[license-shield]: https://img.shields.io/github/license/othneildrew/Best-README-Template.svg?style=for-the-badge
[license-url]: https://github.com/antonio-cirillo/word-count/blob/main/LICENSE
[product-screenshot]: images/screenshot.png
